#include "Debugger.hpp"
#include "DebugContext.hpp"
#include "DebugStackTrace.hpp"
#include "Path.hpp"
#include "Error.hpp"
#include "String.hpp"

#include <Windows.h>
#include <DbgHelp.h>
#include <Psapi.h>

#include <memory>
#include <ctime>
#include <iostream>
#include <conio.h>

using namespace Hindsight::Debugger;
using namespace Hindsight::Process;
using namespace Hindsight::Exceptions;
using namespace Hindsight::Utilities;
using namespace Hindsight::Debugger::CxxExceptions;

/// <summary>
/// Construct a new Debugger instance for real-time debugging.
/// </summary>
/// <param name="process">A shared pointer to a <see cref="Hindsight::Process::Process"/> instance containing information about the process to be debugged.</param>
/// <param name="state">The hindsight program argument state.</param>
Debugger::Debugger(std::shared_ptr<Hindsight::Process::Process> process, const Cli::HindsightCli& state)
	: m_Process(process), m_State(state), m_SubState(state[state.get_chosen_subcommand_name()]) {

	// process must be running.
	if (!m_Process->Running())
		throw new ProcessNotRunningException();
}

/// <summary>
/// Construct a new Debugger instance for postmortem (JIT) debugging.
/// </summary>
/// <param name="process">A shared pointer to a <see cref="Hindsight::Process::Process"/> instance containing information about the process to be debugged.</param>
/// <param name="state">The hindsight program argument state.</param>
/// <param name="jitEvent">The event handle copied into the hindsight process, so that WER can be signaled to let the debugged process continue.</param>
/// <param name="jit">An address in the debugged process address space pointing to a <see cref="JIT_DEBUG_INFO"/> instance.</param>
Debugger::Debugger(std::shared_ptr<Hindsight::Process::Process> process, const Cli::HindsightCli& state, HANDLE jitEvent, void* jit)
	: m_Process(process), m_State(state), m_SubState(state[state.get_chosen_subcommand_name()]) {

	m_Jit = std::make_shared<JitDebuggerInfo>();
	m_Jit->JitEvent = jitEvent;

	// process must be running.
	if (!m_Process->Running())
		throw ProcessNotRunningException();

	// read the JIT_DEBUG_INFO struct from the debugged process.
	if (!m_Process->Read(jit, m_Jit->JitInfo))
		throw RemoteReadException(GetLastError());

	// add the thread ID and handle to the process instance.
	process->dwThreadId = m_Jit->JitInfo.dwThreadID;
	process->hThread    = OpenThread(THREAD_ALL_ACCESS, false, m_Jit->JitInfo.dwThreadID);
}

/// <summary>
/// The Debugger destructor, which closes the process handles and detaches the debugger from it.
/// </summary>
Debugger::~Debugger() {
	if (m_Jit != nullptr)
		m_Process->Close();

	Detach();
}

/// <summary>
/// Attach the debugger to the process.
/// </summary>
/// <param name="killOnDetach">When set to true, the debugged process will be killed when the debugger detaches.</param>
/// <returns>When succesful, true is returned.</returns>
bool Debugger::Attach(bool killOnDetach) {
	std::shared_ptr<DebugContext>		initialContext;		/* Used for postmortem attach, nullptr by default for regular debugging */
	std::shared_ptr<DebugStackTrace>	initialStackTrace;	/* Used for postmortem attach, nullptr by default for regular debugging */
	auto time = std::time(nullptr); /* Time of initialization */

	if (m_Jit != nullptr) { /* Postmortem, dump exception + traces */
		// Initialize the event handlers.
		for (auto handler : m_Handlers)
			handler->OnInitialization(time, m_Process);

		// Enumerate all loaded modules and simulate a LOAD_DLL_DEBUG_EVENT for each of them.
		EnumerateProcessModules(m_Process->hProcess);

		// Add the process image path to the search path if specified in the flags.
		auto pdbSearchPaths = m_SubState.get<std::vector<std::string>>(Cli::Descriptors::NAME_PDBSEARCH);
		if (m_SubState.isset(Cli::Descriptors::NAME_PDBSELF))
			pdbSearchPaths.push_back(Path::GetModulePath(m_Process->hProcess, NULL));
		
		union {
			WOW64_CONTEXT	ctx32;
			CONTEXT			ctx64 = { 0 };
		};
		
		// Fetch the appropriate thread context.
		if (m_Process->IsWow64()) {
			m_Process->Read(reinterpret_cast<void*>(m_Jit->JitInfo.lpContextRecord), ctx32);
			initialContext = std::make_shared<DebugContext>(m_Process->hProcess, m_Process->hThread, ctx32);
		} else {
			m_Process->Read(reinterpret_cast<void*>(m_Jit->JitInfo.lpContextRecord), ctx64);
			initialContext = std::make_shared<DebugContext>(m_Process->hProcess, m_Process->hThread, ctx64);
		}

		// Get stack trace.
		initialStackTrace = std::make_shared<DebugStackTrace>(
			initialContext, m_LoadedModules, String::Join(pdbSearchPaths, ";"), 
			m_SubState.get<size_t>(Cli::Descriptors::NAME_MAX_RECURSION), 
			m_SubState.get<size_t>(Cli::Descriptors::NAME_MAX_INSTRUCTION));

		// Construct the exception object.
		EXCEPTION_DEBUG_INFO exception;
		exception.dwFirstChance = false;
		m_Process->Read(reinterpret_cast<void*>(m_Jit->JitInfo.lpExceptionRecord), exception.ExceptionRecord);
		exception.ExceptionRecord.ExceptionAddress = reinterpret_cast<PVOID>(m_Jit->JitInfo.lpExceptionAddress);

		// If this is a C++ EH Exception from MSVC++, we can intercept some information from it.
		std::shared_ptr<ExceptionRunTimeTypeInformation> ertti = nullptr;
		if (exception.ExceptionRecord.ExceptionCode == static_cast<DWORD>(EH_EXCEPTION_NUMBER))
		if (exception.ExceptionRecord.ExceptionInformation[0] == static_cast<ULONG_PTR>(EH_MAGIC_NUMBER1))
			ertti = std::make_shared<ExceptionRunTimeTypeInformation>(m_Process, exception.ExceptionRecord, m_LoadedModules);

		for (auto handler : m_Handlers) {
			// Emit the JIT exception to the handler.
			EmitJitException(handler, m_Jit->JitInfo, exception, initialContext, initialStackTrace, ertti);

			// ... and then finalize the handler
			handler->OnModuleCollectionComplete(time, m_LoadedModules);
		} 

		// Kill the process, it might trigger another instance of the mortem subcommand - but it will exit as the process is gone.
		m_Process->Kill(static_cast<UINT>(exception.ExceptionRecord.ExceptionCode));
		SetEvent(m_SubState.get<HANDLE>(Cli::Descriptors::NAME_JITEVENT)); /* signal WER we resolved the issue */

		return true;
	}

	// Regular debugging session, attach using DebugActiveProcess
	auto result = DebugActiveProcess(m_Process->dwProcessId);
	if (result) {
		if (!killOnDetach) /* don't kill on detach */
			DebugSetProcessKillOnExit(false);

		// Invoke initialization methods on handlers.
		for (auto handler : m_Handlers) 
				handler->OnInitialization(time, m_Process);
	} 

	return result;
}

/// <summary>
/// Detach the debugger from the process.
/// </summary>
/// <returns>When successful, true is returned.</returns>
bool Debugger::Detach() {
	return DebugActiveProcessStop(m_Process->dwProcessId);
}

/// <summary>
/// Add an implementation of <see cref="::Hindsight::Debugger::EventHandler::IDebuggerEventHandler"/> to the debugger.
/// </summary>
/// <param name="handler">The debug event handler.</param>
void Debugger::AddHandler(std::shared_ptr<EventHandler::IDebuggerEventHandler> handler) {
	m_Handlers.push_back(handler);
}

/// <summary>
/// Enumerate all the loaded modules for a specific process and simulate the LOAD_DLL debug event to all the handlers,
/// but also track the collection of loaded modules so that address resolution will work properly in those handlers.
/// </summary>
/// <param name="hProcess">The handle to the debugged process.</param>
void Debugger::EnumerateProcessModules(HANDLE hProcess) {
	DWORD cbNeeded;
	HMODULE hModsDetermine;
	MODULEINFO modInfo;

	EnumProcessModulesEx(hProcess, &hModsDetermine, sizeof(HMODULE), &cbNeeded, LIST_MODULES_ALL);

	std::vector<HMODULE> hMods(cbNeeded / sizeof(HMODULE));

	auto time = std::time(nullptr);

	if (EnumProcessModulesEx(hProcess, &hMods[0], cbNeeded, &cbNeeded, LIST_MODULES_ALL)) {
		LOAD_DLL_DEBUG_INFO di		 = { NULL, NULL, NULL, NULL, NULL, true };
		const PROCESS_INFORMATION pi = m_Process->GetProcessInformation();

		for (size_t i = 0, count = (cbNeeded / sizeof(HMODULE)); i < count; ++i) {
			std::wstring modName(MAX_PATH, ' ');

			auto length = GetModuleFileNameEx(hProcess, hMods[i], &modName[0], static_cast<DWORD>(modName.size()));
			if (!length) {
				modName = L"";
			} else {
				modName.resize(length);
			}

			// Only add the module and trigger the event when we can fetch the information we need on the module.
			if (GetModuleInformation(hProcess, hMods[i], &modInfo, sizeof(MODULEINFO))) {
				m_LoadedModules.Load(modName, reinterpret_cast<ModulePointer>(modInfo.lpBaseOfDll), modInfo.SizeOfImage);

				for (auto handler : m_Handlers) {
					di.lpBaseOfDll = modInfo.lpBaseOfDll;

					// simualte a LOAD_DLL_DEBUG_EVENT on the handlers, so that the writers actually write the loaded modules.
					handler->OnDllLoad(time, di, m_Process->GetProcessInformation(), modName, m_LoadedModules.GetIndex(modName), m_LoadedModules);
				}
			}
		}
	}
}

/// <summary>
/// Emit the postmortem/JIT exception to a handler.
/// </summary>
/// <param name="handler">The handler to trigger.</param>
/// <param name="info">A const reference to the <see cref="JIT_DEBUG_INFO"/> struct.</param>
/// <param name="exception">A const reference to the normalized <see cref="EXCEPTION_DEBUG_INFO"/> struct.</param>
/// <param name="context">A shared pointer to the thread context where the exception was raised.</param>
/// <param name="trace">A shared pointer to the stack trace starting from the program counter address where the exception originated.</param>
void Debugger::EmitJitException(std::shared_ptr<EventHandler::IDebuggerEventHandler> handler, const JIT_DEBUG_INFO& info, const EXCEPTION_DEBUG_INFO& exception, std::shared_ptr<DebugContext> context, std::shared_ptr<DebugStackTrace> trace, std::shared_ptr<CxxExceptions::ExceptionRunTimeTypeInformation> ertti) {
	std::wstring name;
	if (ExceptionNames.count(exception.ExceptionRecord.ExceptionCode))
		name = ExceptionNames.at(exception.ExceptionRecord.ExceptionCode);

	handler->OnException(std::time(nullptr), exception, m_Process->GetProcessInformation(), static_cast<bool>(exception.dwFirstChance), name, context, trace, m_LoadedModules, ertti);
}

/// <summary>
/// Wait for the next debug event and emit it to all the handlers.
/// </summary>
/// <returns>When the debug loop should stop, false is returned.</returns>
bool Debugger::Tick() {
	DEBUG_EVENT				 event;
	PROCESS_INFORMATION		 pi;

	auto stay = true;
	auto continueStatus = DBG_CONTINUE;

	// No event, continue.
	if (!WaitForDebugEventEx(&event, INFINITE))
		return stay;

	// Open the process and thread of the event.
	pi.dwProcessId = event.dwProcessId;
	pi.dwThreadId  = event.dwThreadId;

	pi.hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, pi.dwProcessId);
	if (!pi.hProcess) {
		std::cout << "error: cannot open process: 0x" << std::hex << pi.dwProcessId << std::dec << std::endl;
		return stay;
	}

	pi.hThread = OpenThread(THREAD_ALL_ACCESS, false, pi.dwThreadId);
	if (!pi.hThread) {
		std::cout << "error: cannot open thread: 0x" << std::hex << pi.dwThreadId << std::dec << std::endl;
		CloseHandle(pi.hProcess);
		return stay;
	}

	// The time of the event.
	auto time = std::time(nullptr);

	// Trigger the event handlers 
	switch (event.dwDebugEventCode) {
		// Handle exceptions.
		case EXCEPTION_DEBUG_EVENT: {
			// This debugger does not handle events, it records them.
			continueStatus = DBG_EXCEPTION_NOT_HANDLED;

			// Add the process module's path to the PDB search list if the -S option was used.
			auto pdbSearchPaths = m_SubState.get<std::vector<std::string>>(Cli::Descriptors::NAME_PDBSEARCH);
			if (m_SubState.isset(Cli::Descriptors::NAME_PDBSELF))
				pdbSearchPaths.push_back(Path::GetModulePath(m_Process->hProcess, NULL));

			// Construct a thread context for this state and construct a stack trace from that context.
			auto context = std::make_shared<DebugContext>(pi.hProcess, pi.hThread);
			auto trace   = std::make_shared<DebugStackTrace>(
				context, m_LoadedModules, String::Join(pdbSearchPaths, ";"), 
				m_SubState.get<size_t>(Cli::Descriptors::NAME_MAX_RECURSION),
				m_SubState.get<size_t>(Cli::Descriptors::NAME_MAX_INSTRUCTION));

			// Differentiate exceptions from breakpoints. Single-step exceptions are just walked over as regular exceptions.
			switch (event.u.Exception.ExceptionRecord.ExceptionCode) {
				case EXCEPTION_BREAKPOINT:
				case STATUS_WX86_BREAKPOINT: { /* We're dealing with breakpoint exceptions, handle accordingly */
					for (auto handler : m_Handlers)
						handler->OnBreakpointHit(time, event.u.Exception, pi, context, trace, m_LoadedModules);

					// Should we break?
					if (m_SubState.isset(Cli::Descriptors::NAME_BREAKB))
						HandleBreakpointOptions();

					break;
				}

				default: { /* any other exception, including single step exceptions. */
					std::wstring name;

					// Try to get a name for the exception, if it is a standard error code.
					if (ExceptionNames.count(event.u.Exception.ExceptionRecord.ExceptionCode))
						name = ExceptionNames.at(event.u.Exception.ExceptionRecord.ExceptionCode);
					
					// If this is a C++ EH Exception from MSVC++, we can intercept some information from it.
					std::shared_ptr<ExceptionRunTimeTypeInformation> ertti = nullptr;
					if (event.u.Exception.ExceptionRecord.ExceptionCode == static_cast<DWORD>(EH_EXCEPTION_NUMBER)) 
					if (event.u.Exception.ExceptionRecord.ExceptionInformation[0] == static_cast<ULONG_PTR>(EH_MAGIC_NUMBER1)) 
						ertti = std::make_shared<ExceptionRunTimeTypeInformation>(m_Process, event.u.Exception.ExceptionRecord, m_LoadedModules);
					
					for (auto handler : m_Handlers)
						handler->OnException(time, event.u.Exception, pi, static_cast<bool>(event.u.Exception.dwFirstChance), name, context, trace, m_LoadedModules, ertti);

					// Should we break?
					if (m_SubState.isset(Cli::Descriptors::NAME_BREAKE) && (!m_SubState.isset(Cli::Descriptors::NAME_BREAKF) || event.u.Exception.dwFirstChance))
						HandleBreakpointOptions();

					break;
				}
			}

			break;
		}


		// Handle process creation, fetch the path of the started module for the event handlers and mark it as loaded.
		case CREATE_PROCESS_DEBUG_EVENT: {
			auto fullPath = Path::GetPathFromFileHandleW(event.u.CreateProcessInfo.hFile);
			m_LoadedModules.Load(pi.hProcess, fullPath, event.u.CreateProcessInfo.lpBaseOfImage);

			for (auto handler : m_Handlers)
				handler->OnCreateProcess(time, event.u.CreateProcessInfo, pi, fullPath, m_LoadedModules);

			break;
		}

		// Handle the thread creation event.
		case CREATE_THREAD_DEBUG_EVENT: {
			for (auto handler : m_Handlers)
				handler->OnCreateThread(time, event.u.CreateThread, pi, m_LoadedModules);

			break;
		}

		// Handle the process termination event.
		case EXIT_PROCESS_DEBUG_EVENT: {
			stay = false; /* stop debugging */
			for (auto handler : m_Handlers)
				handler->OnExitProcess(time, event.u.ExitProcess, pi, m_LoadedModules);

			break;
		}

		// Handle the thread termination event.
		case EXIT_THREAD_DEBUG_EVENT:
			for (auto handler : m_Handlers)
				handler->OnExitThread(time, event.u.ExitThread, pi, m_LoadedModules);

			break;

		// Handle the loading of a DLL module, fetch its path and mark it as loaded before invoking the handlers.
		case LOAD_DLL_DEBUG_EVENT: {
			auto fullPath = Path::GetPathFromFileHandleW(event.u.LoadDll.hFile);
			m_LoadedModules.Load(pi.hProcess, fullPath, event.u.LoadDll.lpBaseOfDll);

			for (auto handler : m_Handlers)
				handler->OnDllLoad(time, event.u.LoadDll, pi, fullPath, m_LoadedModules.GetIndex(fullPath), m_LoadedModules);

			break;
		}

		// Handle the debug string event, the debugged process sent a string to the debugger.
		case OUTPUT_DEBUG_STRING_EVENT: {
			if (!event.u.DebugString.fUnicode) {
				auto debugString = String::Trim(m_Process->ReadString(event.u.DebugString.lpDebugStringData, event.u.DebugString.nDebugStringLength));

				for (auto handler : m_Handlers)
					handler->OnDebugString(time, event.u.DebugString, pi, debugString);
			} else {
				auto debugString = String::Trim(m_Process->ReadStringW(event.u.DebugString.lpDebugStringData, event.u.DebugString.nDebugStringLength));

				for (auto handler : m_Handlers)
					handler->OnDebugStringW(time, event.u.DebugString, pi, debugString);
			}

			break;
		}

		// Handle the RIP error event, try to fetch a formatted error message for the dwError member.
		case RIP_EVENT: {
			auto errorMessage = Error::GetErrorMessageW(event.u.RipInfo.dwError);
			for (auto handler : m_Handlers)
				handler->OnRip(time, event.u.RipInfo, pi, errorMessage);

			break;
		}

		// Handle the unloading of a DLL. Fetch the full path of the module, invoke the handlers and then mark it as unloaded.
		case UNLOAD_DLL_DEBUG_EVENT: {
			auto fullPath = m_LoadedModules.Get(event.u.UnloadDll.lpBaseOfDll);

			for (auto handler : m_Handlers)
				handler->OnDllUnload(time, event.u.UnloadDll, pi, fullPath, m_LoadedModules.GetIndex(fullPath), m_LoadedModules);

			m_LoadedModules.Unload(event.u.UnloadDll.lpBaseOfDll);

			break;
		}

		default:
			OutputDebugString(L"Uknown Event");
	}

	// Close thread and process handles.
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	// Continue
	ContinueDebugEvent(event.dwProcessId, event.dwThreadId, continueStatus);

	return stay;
}

/// <summary>
/// Will break the output, until the user presses a key.
/// </summary>
void Debugger::HandleBreakpointOptions() {
	char choice;

	do {
		std::wcout << L"[c]ontinue or [a]bort?" << std::endl;
		choice = std::tolower(_getch());
	} while (choice != 'c' && choice != 'a');

	if (choice == 'a') {
		DebugSetProcessKillOnExit(true);
		exit(0);
	}
}

/// <summary>
/// Start the debugger main loop.
/// </summary>
void Debugger::Start() {
	// While events can be processed, continue.
	while (Tick());

	// Finalize handlers.
	auto time = std::time(nullptr);
	for (auto handler : m_Handlers)
		handler->OnModuleCollectionComplete(time, m_LoadedModules);
}

/// <summary>
/// Names of common error codes that exceptions can have.
/// </summary>
std::map<DWORD, std::wstring> Debugger::ExceptionNames = {
	{ EXCEPTION_ACCESS_VIOLATION,			L"EXCEPTION_ACCESS_VIOLATION" },
	{ EXCEPTION_ARRAY_BOUNDS_EXCEEDED,		L"EXCEPTION_ARRAY_BOUNDS_EXCEEDED" },
	{ EXCEPTION_BREAKPOINT,					L"EXCEPTION_BREAKPOINT" },
	{ EXCEPTION_DATATYPE_MISALIGNMENT,		L"EXCEPTION_DATATYPE_MISALIGNMENT" },
	{ EXCEPTION_FLT_DENORMAL_OPERAND,		L"EXCEPTION_FLT_DENORMAL_OPERAND" },
	{ EXCEPTION_FLT_DIVIDE_BY_ZERO,			L"EXCEPTION_FLT_DIVIDE_BY_ZERO" },
	{ EXCEPTION_FLT_INEXACT_RESULT,			L"EXCEPTION_FLT_INEXACT_RESULT" },
	{ EXCEPTION_FLT_INVALID_OPERATION,		L"EXCEPTION_FLT_INVALID_OPERATION" },
	{ EXCEPTION_FLT_OVERFLOW,				L"EXCEPTION_FLT_OVERFLOW" },
	{ EXCEPTION_FLT_STACK_CHECK,			L"EXCEPTION_FLT_STACK_CHECK" },
	{ EXCEPTION_FLT_UNDERFLOW,				L"EXCEPTION_FLT_UNDERFLOW" },
	{ EXCEPTION_ILLEGAL_INSTRUCTION,		L"EXCEPTION_ILLEGAL_INSTRUCTION" },
	{ EXCEPTION_IN_PAGE_ERROR,				L"EXCEPTION_IN_PAGE_ERROR" },
	{ EXCEPTION_INT_DIVIDE_BY_ZERO,			L"EXCEPTION_INT_DIVIDE_BY_ZERO" },
	{ EXCEPTION_INT_OVERFLOW,				L"EXCEPTION_INT_OVERFLOW" },
	{ EXCEPTION_INVALID_DISPOSITION,		L"EXCEPTION_INVALID_DISPOSITION" },
	{ EXCEPTION_NONCONTINUABLE_EXCEPTION,	L"EXCEPTION_NONCONTINUABLE_EXCEPTION" },
	{ EXCEPTION_PRIV_INSTRUCTION,			L"EXCEPTION_PRIV_INSTRUCTION" },
	{ EXCEPTION_SINGLE_STEP,				L"EXCEPTION_SINGLE_STEP" },
	{ EXCEPTION_STACK_OVERFLOW,				L"EXCEPTION_STACK_OVERFLOW" },
	//{ EXCEPTION_POSSIBLE_DEADLOCK,			L"EXCEPTION_POSSIBLE_DEADLOCK" },
	{ EXCEPTION_INVALID_HANDLE,				L"EXCEPTION_INVALID_HANDLE" },
	{ STATUS_WX86_BREAKPOINT,				L"STATUS_WX86_BREAKPOINT" },
	{ STATUS_WX86_SINGLE_STEP,				L"STATUS_WX86_SINGLE_STEP" },
	{ EH_EXCEPTION_THREAD_NAME,				L"THREAD_NAMING" },
	{ EH_EXCEPTION_NUMBER,					L"CXX_VCPP_EH_EXCEPTION" }
};