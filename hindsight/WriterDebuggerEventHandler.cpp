#include "WriterDebuggerEventHandler.hpp"
#include "BinaryLogFile.hpp"
#include "String.hpp"
#include "crc32.hpp"
#include <ctime>

using namespace Hindsight::BinaryLog;
using namespace Hindsight::Debugger;
using namespace Hindsight::Debugger::EventHandler;

/// <summary>
/// Construct a new WriterDebuggerEventHandler, which will create and write to <paramref name="filepath"/>.
/// </summary>
/// <param name="filepath">The path to the file which will contain the logged events.</param>
WriterDebuggerEventHandler::WriterDebuggerEventHandler(const std::string& filepath) {
	m_Stream.open(filepath, std::ios::binary | std::ios::out);

	if (!m_Stream.is_open())
		throw std::runtime_error("cannot open file for writing: " + filepath);
}

/// <summary>
/// Write the initial data to the binary log file, such as the file header. It will also write 
/// the debugged process path, working directory (if available) and program parameters (if available).
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="p">The process being debugged.</param>
void WriterDebuggerEventHandler::OnInitialization(
	time_t time,
	const std::shared_ptr<const Hindsight::Process::Process> pi) {

	m_Header.ProcessId				= pi->dwProcessId;
	m_Header.ThreadId				= pi->dwThreadId;
	m_Header.PathLength				= pi->Path.size();
	m_Header.WorkingDirectoryLength	= pi->WorkingDirectory.size();
	m_Header.Arguments				= pi->Arguments.size();
	m_Header.StartTime				= std::time(nullptr);
	m_Header.Crc32					= 0;

	// write header, path and working directory
	Write((const char*)&m_Header, sizeof(FileHeader), false);
	Write(pi->Path);
	Write(pi->WorkingDirectory);

	// write each program argument, prepended with its length
	for (const auto& argument : pi->Arguments) 
		Write(argument, true);
}

/// <summary>
/// Write details about a breakpoint exception event, including a thread context and stack trace.
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="info">A const reference to the <see cref="EXCEPTION_DEBUG_INFO"/> struct instance with event information.</param>
/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
/// <param name="context">A shared pointer to a const <see cref="::Hindsight::Debugger::DebugContext"/> instance.</param>
/// <param name="trace">A shared pointer to a const <see cref="::Hindsight::Debugger::DebugStackTrace"/> instance.</param>
/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
void WriterDebuggerEventHandler::OnBreakpointHit(
	time_t time,
	const EXCEPTION_DEBUG_INFO& info,
	const PROCESS_INFORMATION& pi,
	std::shared_ptr<const DebugContext> context,
	std::shared_ptr<const DebugStackTrace> trace,
	const ModuleCollection& collection) {

	// Write this exception event, but only indicate if it is a breakpoint or not.
	// Both the exception and breakpoint event are exception events.
	Write(info, pi, context, trace, collection, true);
}

/// <summary>
/// Write details about an exception event, including a thread context and stack trace.
/// stack trace and context of the exception address.
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="info">A const reference to the <see cref="EXCEPTION_DEBUG_INFO"/> struct instance with event information.</param>
/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
/// <param name="firstChance">A boolean indicating that this is the first encounter with this specific exception instance, or not.</param>
/// <param name="name">A name of a known exception, or an empty string.</param>
/// <param name="context">A shared pointer to a const <see cref="::Hindsight::Debugger::DebugContext"/> instance.</param>
/// <param name="trace">A shared pointer to a const <see cref="::Hindsight::Debugger::DebugStackTrace"/> instance.</param>
/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
void WriterDebuggerEventHandler::OnException(
	time_t time,
	const EXCEPTION_DEBUG_INFO& info,
	const PROCESS_INFORMATION& pi,
	bool firstChance,
	const std::wstring& name,
	std::shared_ptr<const DebugContext> context,
	std::shared_ptr<const DebugStackTrace> trace,
	const ModuleCollection& collection) {

	// Write this exception event, but only indicate if it is a breakpoint or not.
	// Both the exception and breakpoint event are exception events.
	Write(info, pi, context, trace, collection, false);
}

/// <summary>
/// Write the event that denotes the creation of a process.
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="info">A const reference to the <see cref="CREATE_PROCESS_DEBUG_INFO"/> struct instance with event information.</param>
/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
/// <param name="path">The full path to the loaded module on file system.</param>
/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
void WriterDebuggerEventHandler::OnCreateProcess(
	time_t time,
	const CREATE_PROCESS_DEBUG_INFO& info,
	const PROCESS_INFORMATION& pi,
	const std::wstring& path,
	const ModuleCollection& collection) {

	// Get the module instance for this process
	auto module = collection.GetModuleAtAddress(info.lpBaseOfImage);

	// Create an EventEntry for this event 
	CreateProcessEventEntry createProcessEventEntry(pi, path, reinterpret_cast<uint64_t>(module->Base), module->Size);

	// Write the EventEntry for this event and append the path to it.
	Write(createProcessEventEntry);
	Write(path);
}

/// <summary>
/// Write the event that denotes the creation of a thread.
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="info">A const reference to the <see cref="CREATE_THREAD_DEBUG_INFO"/> struct instance with event information.</param>
/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
void WriterDebuggerEventHandler::OnCreateThread(
	time_t time,
	const CREATE_THREAD_DEBUG_INFO& info,
	const PROCESS_INFORMATION& pi,
	const ModuleCollection& collection) {

	// Create the EventEntry for this event
	auto address = reinterpret_cast<uint64_t>(info.lpStartAddress);
	CreateThreadEventEntry createThreadEventEntry(pi, address);

	// Add module information to it, if available.
	auto module = collection.GetModuleAtAddress(info.lpStartAddress);
	if (module != nullptr) {
		createThreadEventEntry.ModuleIndex		= collection.GetIndex(module->Path);
		createThreadEventEntry.EntryPointOffset = address - reinterpret_cast<uint64_t>(module->Base);
	} else {
		createThreadEventEntry.ModuleIndex		= -1;
		createThreadEventEntry.EntryPointOffset = 0;
	}

	// Write the event
	Write(createThreadEventEntry);
}

/// <summary>
/// Write the EXIT_PROCESS event.
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="info">A const reference to the <see cref="EXIT_PROCESS_DEBUG_INFO"/> struct instance with event information.</param>
/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
void WriterDebuggerEventHandler::OnExitProcess(
	time_t time,
	const EXIT_PROCESS_DEBUG_INFO& info,
	const PROCESS_INFORMATION& pi,
	const ModuleCollection& collection) {

	// Create an EventEntry for this event and write it
	ExitProcessEventEntry exitProcessEventEntry(pi, info.dwExitCode);

	Write(exitProcessEventEntry);
}

/// <summary>
/// Write the EXIT_THREAD event.
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="info">A const reference to the <see cref="EXIT_THREAD_DEBUG_INFO"/> struct instance with event information.</param>
/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
void WriterDebuggerEventHandler::OnExitThread(
	time_t time,
	const EXIT_THREAD_DEBUG_INFO& info,
	const PROCESS_INFORMATION& pi,
	const ModuleCollection& collection) {

	// Create an EventEntry for this event and write it
	ExitThreadEventEntry exitProcessEventEntry(pi, info.dwExitCode);

	Write(exitProcessEventEntry);
}

/// <summary>
/// Write the LOAD_DLL event, accompanied by information about the loaded module.
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="info">A const reference to the <see cref="EXIT_THREAD_DEBUG_INFO"/> struct instance with event information.</param>
/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
/// <param name="path">The full unicode path to the DLL that was loaded.</param>
/// <param name="moduleIndex">The index in the module collection of this module, it might have been loaded before.</param>
/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
void WriterDebuggerEventHandler::OnDllLoad(
	time_t time,
	const LOAD_DLL_DEBUG_INFO& info,
	const PROCESS_INFORMATION& pi,
	const std::wstring& path,
	int moduleIndex,
	const ModuleCollection& collection) {

	// Obtain information about the loaded module 
	uint64_t moduleSize = 0;
	const auto module = collection.GetModuleAtAddress(info.lpBaseOfDll);
	if (module != nullptr)
		moduleSize = module->Size;

	// Create an EventEntry for this event 
	DllLoadEventEntry dllLoadEventEntry(pi, moduleIndex, reinterpret_cast<uint64_t>(info.lpBaseOfDll), moduleSize, path.size());

	// Write the EventEntry and append the module path
	Write(dllLoadEventEntry);
	Write(path);
}

/// <summary>
/// Write an ANSI (or might be UTF-8) debug string sent by the debugged process.
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="info">A const reference to the <see cref="OUTPUT_DEBUG_STRING_INFO"/> struct instance with event information.</param>
/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
/// <param name="string">The ANSI debug string.</param>
void WriterDebuggerEventHandler::OnDebugString(
	time_t time,
	const OUTPUT_DEBUG_STRING_INFO& info,
	const PROCESS_INFORMATION& pi,
	const std::string& string) {

	// Create an EventEntry for this event with unicode set to false
	DebugStringEventEntry debugStringEventEntry(pi, false, string.size());

	// Write the EventEntry and debug string
	Write(debugStringEventEntry);
	Write(string);
}

/// <summary>
/// Write a unicode (wchar_t) debug string sent by the debugged process.
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="info">A const reference to the <see cref="OUTPUT_DEBUG_STRING_INFO"/> struct instance with event information.</param>
/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
/// <param name="string">The Unicode debug string.</param>
void WriterDebuggerEventHandler::OnDebugStringW(
	time_t time,
	const OUTPUT_DEBUG_STRING_INFO& info,
	const PROCESS_INFORMATION& pi,
	const std::wstring& string) {

	// Create an EventEntry for this event with unicode set to true
	DebugStringEventEntry debugStringEventEntry(pi, true, string.size());

	// Write the EventEntry and debug string
	Write(debugStringEventEntry);
	Write(string);
}

/// <summary>
/// Write a RIP error event.
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="info">A const reference to the <see cref="RIP_INFO"/> struct instance with event information.</param>
/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
/// <param name="errorMessage">A string describing the <see cref="RIP_INFO::dwError"/> member, or an empty string if no description is available.</param>
void WriterDebuggerEventHandler::OnRip(
	time_t time,
	const RIP_INFO& info,
	const PROCESS_INFORMATION& pi,
	const std::wstring& errorMessage) {

	// Create an EventEntry for this event and write it
	RipEventEntry ripEventEntry(pi, info.dwType, info.dwError);
	Write(ripEventEntry);
}

/// <summary>
/// Write an UNLOAD_DLL event. Binary log files don't keep a complete list of loaded modules, instead they 
/// keep the LOAD and UNLOAD events. Like that, it is known when a module was in memory and when it was not.
/// 
/// One could do an extra pass on the file extracting all of these events and build a list.
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="info">A const reference to the <see cref="RIP_INFO"/> struct instance with event information.</param>
/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
/// <param name="path">The full unicode path to the DLL that is being unloaded.</param>
/// <param name="moduleIndex">The index in the module collection of this module.</param>
/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
/// <remarks>
///		Note: At the time of invoking this event, the module still exists in <paramref name="collection"/> so that you can still work with it.
///			  It is unloaded immediately after all event handlers have been invoked.
/// </remarks>
void WriterDebuggerEventHandler::OnDllUnload(
	time_t time,
	const UNLOAD_DLL_DEBUG_INFO& info,
	const PROCESS_INFORMATION& pi,
	const std::wstring& path,
	int moduleIndex,
	const ModuleCollection& collection) {

	// Create an EventEntry for this event and write it
	DllUnloadEventEntry dllUnloadEventEntry(pi, reinterpret_cast<uint64_t>(info.lpBaseOfDll));
	Write(dllUnloadEventEntry);
}

/// <summary>
/// Finalize the binary logging, which will seek back to the header and overwrite it with the updated Crc32 checksum.
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
void WriterDebuggerEventHandler::OnModuleCollectionComplete(
	time_t time, 
	const ModuleCollection& collection) {

	// finalize by overwriting the header as the checksum member is now complete
	m_Stream.seekp(0, std::ios::beg);
	Write(m_Header);
	m_Stream.seekp(0, std::ios::end);
}

/// <summary>
/// Write a class or struct instance to the output stream.
/// </summary>
/// <param name="s">A reference to the class or struct instance to write, of type <typeparamref name="T"/>.</param>
/// <typeparam name="T">The type of value to write, which must be a class.</typeparam>
template <typename T, std::enable_if_t<std::is_class<T>::value, int>>
void WriterDebuggerEventHandler::Write(T s) {
	Write((const char*)&s, sizeof(T));
}

/// <summary>
/// Write a <see cref="::std::wstring"/> to the output stream, optionally preceeded by its length.
/// </summary>
/// <param name="s">A reference to the string to write.</param>
/// <param name="writeLength">When set to true, the length of the string will be written as a <see cref="uint32_t"/> before the string.</param>
void WriterDebuggerEventHandler::Write(const std::wstring& s, bool writeLength) {
	if (s.empty())
		return;

	if (writeLength) {
		auto size = static_cast<uint32_t>(s.size());
		Write(reinterpret_cast<const char*>(&size), sizeof(uint32_t));
	}

	Write((const char*)s.c_str(), s.size() * sizeof(wchar_t));
}

/// <summary>
/// Write a <see cref="::std::string"/> to the output stream, optionally preceeded by its length.
/// </summary>
/// <param name="s">A reference to the string to write.</param>
/// <param name="writeLength">When set to true, the length of the string will be written as a <see cref="uint32_t"/> before the string.</param>
void WriterDebuggerEventHandler::Write(const std::string& s, bool writeLength) {
	if (s.empty())
		return;

	if (writeLength) {
		auto size = static_cast<uint32_t>(s.size());
		Write(reinterpret_cast<const char*>(&size), sizeof(uint32_t));
	}

	Write((const char*)s.c_str(), s.size() * sizeof(char));
}

/// <summary>
/// Write an arbitrarily sized block of data to the output stream and optionally update the internal checksum of
/// all the written data.
/// </summary>
/// <param name="data">A pointer to <paramref name="size"/> bytes of memory to write.</param>
/// <param name="size">The amount of bytes to write.</param>
/// <param name="updateChecksum">When set to true, the internal checksum will be updated.</param>
void WriterDebuggerEventHandler::Write(const char* data, size_t size, bool updateChecksum) {
	m_Stream.write(data, size);
	if (updateChecksum)
		m_Header.Crc32 = Hindsight::Checksum::Crc32::Update(data, size, m_Header.Crc32);
}

/// <summary>
/// Write an exception event, this function is defined because both breakpoints and exceptions are written in the 
/// exact same format in binary log files. They are, essentially, the same debug event with only an exception code
/// indicating whether it is a breakpoint or not.
/// </summary>
/// <param name="info">A const reference to the <see cref="EXCEPTION_DEBUG_INFO"/> instance of this event.</param>
/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> instance of the thread causing the event.</param>
/// <param name="context">A shared pointer to a <see cref="::Hindsight::Debugger::DebugContext"/> instance.</param>
/// <param name="trace">A shared pointer to a <see cref="::Hindsight::Debugger::DebugStackTrace"/> instance.</param>
/// <param name="collection">A const reference to a <see cref="Hindsight::Debugger::ModuleCollection"/> instance containing information about loaded modules.</param>
/// <param name="isBreak">True when breakpoint, false when exception.</param>
void WriterDebuggerEventHandler::Write(
	const EXCEPTION_DEBUG_INFO& info,
	const PROCESS_INFORMATION& pi,
	std::shared_ptr<const DebugContext> context,
	std::shared_ptr<const DebugStackTrace> trace,
	const ModuleCollection& collection,
	bool isBreak) {

	// Create an EventEntry instance for the exception event
	ExceptionEventEntry event(
		pi,
		reinterpret_cast<uint64_t>(info.ExceptionRecord.ExceptionAddress), 
		info.ExceptionRecord.ExceptionCode,
		!context->Is64(),
		isBreak, 
		info.dwFirstChance);

	// Try to determine information about the module, if available.
	auto module = collection.GetModuleAtAddress(info.ExceptionRecord.ExceptionAddress);
	if (module != nullptr) {
		event.ModuleIndex = collection.GetIndex(module->Path);
		event.EventOffset = reinterpret_cast<uint64_t>(info.ExceptionRecord.ExceptionAddress) - reinterpret_cast<uint64_t>(module->Base);
	} else {
		event.ModuleIndex = -1;
		event.EventOffset = 0;
	}

	// Write the EventEntry, thread context and stack trace
	Write(event);
	Write(context);
	Write(trace, collection);
}

/// <summary>
/// Write a thread context to the output stream, which will be either a <see cref="CONTEXT"/> struct or 
/// a <see cref="WOW64_CONTEXT"/> struct depending on the debugged process. 
/// </summary>
/// <param name="context">A shared pointer to a <see cref="::Hindsight::Debugger::DebugContext"/> instance.</param>
void WriterDebuggerEventHandler::Write(std::shared_ptr<const DebugContext> context) {
	if (context->Is64()) {
		Write(context->Get64());
	} else {
		Write(context->Get86());
	}
}

/// <summary>
/// Write a stack trace to the output stream, starting from an exception address.
/// </summary>
/// <param name="trace">A shared pointer to a <see cref="::Hindsight::Debugger::DebugStackTrace"/> instance.</param>
/// <param name="collection">A const reference to a <see cref="Hindsight::Debugger::ModuleCollection"/> instance containing information about loaded modules.</param>
void WriterDebuggerEventHandler::Write(
	std::shared_ptr<const DebugStackTrace> trace,
	const ModuleCollection& collection) {

	// Create a StackTrace instance which will serve as the header for that frame, and write it.
	StackTrace stackTrace(trace->GetMaxRecursion(), trace->GetMaxInstructions(), trace->size());

	Write(stackTrace);

	// Write each stack trace entry
	for (const auto& entry : trace->list()) {
		// Create a StackTraceEntry instance with all the relevant information on one stack trace frame.
		StackTraceEntry stackTraceEntry = {
			entry.Module.Base != 0 && !entry.Module.Path.empty() ? collection.GetIndex(entry.Module.Path) : 0,
			reinterpret_cast<uint64_t>(entry.Module.Base),
			reinterpret_cast<uint64_t>(entry.Address),
			reinterpret_cast<uint64_t>(entry.AbsoluteAddress),
			reinterpret_cast<uint64_t>(entry.AbsoluteLineAddress),
			reinterpret_cast<uint64_t>(entry.LineAddress),
			entry.Name.size(),
			entry.File.size(),
			entry.Line,
			entry.Recursion,
			entry.RecursionCount,
			entry.Instructions.size()
		};

		// Write the StackTraceEntry, the resolved symbol name (or nothing) and source code filepath (or nothing)
		Write(stackTraceEntry);
		Write(entry.Name); /* might write nothing, as the length of these strings can be 0 */
		Write(entry.File); /* might write nothing, as the length of these strings can be 0 */

		// Write all the decoded instructions, if any
		for (const auto& instr : entry.Instructions) {
			// Create a StackTraceEntryInstruction instance describing this instruction
			StackTraceEntryInstruction stackTraceInstr = {
				instr.Is64BitAddress,
				instr.Offset,
				instr.Size,
				instr.InstructionHex.size(),
				instr.InstructionMnemonic.size(),
				instr.Operands.size()
			};

			// Write the StackTraceEntryInstruction instance, the hexadecimal representation of the decoded instruction, 
			// the instruction mnemonic and instruction operands as strings.
			Write(stackTraceInstr);
			Write(instr.InstructionHex);
			Write(instr.InstructionMnemonic);
			Write(instr.Operands);
		}
	}
}