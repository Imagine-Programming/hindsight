#include "BinaryLogPlayer.hpp"
#include "Debugger.hpp"
#include "Error.hpp"
#include "Version.hpp"
#include "crc32.hpp"

#include <filesystem>
#include <iostream>
#include <conio.h>

using namespace Hindsight::Debugger::EventHandler;
using namespace Hindsight::Debugger;
using namespace Hindsight::BinaryLog;

namespace fs = std::filesystem;

/// <summary>
/// Construct a BinaryLogPlayer instance from a path pointing to a HIND file, and a <see cref="Hindsight::State"/> instance 
/// describing the program arguments parsed by <see cref="CLI::App"/>.
/// </summary>
/// <param name="path">The path to a HIND file, the binary log format of hindsight.</param>
/// <param name="state">The state obtained through processing program arguments through <see cref="CLI::App"/>.</param>
/// <exception cref="std::runtime_error">This exception is thrown when the file cannot be opened or is not a valid binary log file.</exception>
BinaryLogPlayer::BinaryLogPlayer(const std::string& path, const Hindsight::State& state)
	: m_State(state), m_ShouldFilter(!state.ReplayEventFilter.empty()), m_Filter(state.ReplayEventFilter.begin(), state.ReplayEventFilter.end()), m_Crc32(0) {

	m_Stream.open(path, std::ios::in | std::ios::binary);
	if (!m_Stream.is_open())
		throw std::runtime_error("cannot open file for reading: " + path);
	m_StreamSize = static_cast<size_t>(fs::file_size(path));

	Read(reinterpret_cast<char*>(&m_Header), sizeof(FileHeader), false);

	uint32_t fileVersion     = m_Header.Version >> 16;
	uint32_t requiredVersion = hindsight_version_int >> 16;
	uint8_t  requiredMajor   = requiredVersion >> 8;
	uint8_t  requiredMinor   = requiredVersion & 0xf;

	if (fileVersion != requiredVersion)
		throw std::runtime_error("cannot open file, the version used to generate this log differs from the used version. Hindsight " + std::to_string(requiredMajor) + "." + std::to_string(requiredMinor) + " is required.");

	if (!state.NoSanityCheck)
		CheckSanity();
}

/// <summary>
/// Walks the binary log file and verifies that all data matches the <see cref="Hindsight::BinaryLog::FileHeader::Crc32"/>.
/// </summary>
/// <exception cref="std::runtime_error">This exception is thrown when the data in the file does not result in the <see cref="Hindsight::BinaryLog::FileHeader::Crc32"/> checksum.</exception>
void BinaryLogPlayer::CheckSanity() {
	char buf[ChecksumBufferSize] = { 0 };
	auto pos   = m_Stream.tellg();
	auto left  = m_StreamSize - pos;
	auto size  = ChecksumBufferSize;
	auto check = m_Crc32;

	while (left > 0) {
		if (left < size) size = left;
		m_Stream.read(buf, size);
		check = Hindsight::Checksum::Crc32::Update(buf, size, check);
		left -= size;
	}

	m_Stream.seekg(pos, std::ios::beg);
	if (check != m_Header.Crc32)
		throw std::runtime_error("file has been damaged, never finished writing or was appended to. Use --no-sanity-check to ignore this check.");
}

/// <summary>
/// Add an instance of any implementation of <see cref="Hindsight::Debugger::EventHandler::IDebuggerEventHandler"/> to the player.
/// </summary>
/// <param name="handler">An instance of an <see cref="Hindsight::Debugger::EventHandler::IDebuggerEventHandler"/> implementation.</param>
void BinaryLogPlayer::AddHandler(std::shared_ptr<EventHandler::IDebuggerEventHandler> handler) {
	m_Handlers.push_back(handler);
}

/// <summary>
/// Play the binary log file and simulate the debug events that were stored in it.
/// </summary>
/// <exception cref="std::runtime_error">
///	This exception is thrown when the checksum of all data read does not match the stored <see cref="Hindsight::BinaryLog::FileHeader::Crc32"/> 
/// checksum in the file. This exception can also be thrown when an unexpected frame type is encountered in the file, or when an unexpected end 
/// is encountered in the file. 
/// </exception>
void BinaryLogPlayer::Play() {
	std::string path, workingDirectory;
	std::vector<std::string> arguments;

	PROCESS_INFORMATION pi = {
		0, 0, m_Header.ProcessId, m_Header.ThreadId
	};

	Read(path, m_Header.PathLength);
	Read(workingDirectory, m_Header.WorkingDirectoryLength);
	Read(arguments, m_Header.Arguments);

	auto process = std::make_shared<Hindsight::Process::Process>(pi, path, workingDirectory, arguments);

	for (auto handler : m_Handlers)
		handler->OnInitialization(m_Header.StartTime, process);

	while (Next()); // iterate over all events

	auto time = std::time(nullptr);
	for (auto handler : m_Handlers)
		handler->OnModuleCollectionComplete(time, m_Modules);

	if (m_Header.Crc32 != m_Crc32)
		throw std::runtime_error("not all data that was originally written has been read.");
}

/// <summary>
/// Read and process the next <see cref="Hindsight::BinaryLog::EventEntry"/> and emit it as event to the added event handlers.
/// </summary>
/// <seealso cref="Hindsight::BinaryLog::BinaryLogPlayer::AddHandler"/>
/// <returns>A boolean indicating to <see cref="Hindsight::BinaryLog::BinaryLogPlayer::Play"/> that there is more data to be read and played.</returns>
bool BinaryLogPlayer::Next() {
	// no more data, stop reading
	if (SizeLeft() < 4)
		return false;

	// read the frame signature
	char signature[4] = { 0 };
	Read(signature, 4, false);

	// verify frame signature and seek back
	if (_strnicmp(signature, "EVNT", 4))
		throw std::runtime_error("unexpected frame in binary log file, expected event entry.");
	m_Stream.seekg(-4, std::ios::cur);

	// read base frame and seek back, used to determine base event entry type.
	EventEntry e;
	Read(reinterpret_cast<char*>(&e), sizeof(EventEntry), false);
	m_Stream.seekg(-(std::streamoff)sizeof(EventEntry), std::ios::cur);

	EntryFrame		frame;
	DEBUG_EVENT		event;

	event.dwDebugEventCode	= e.EventId;
	event.dwProcessId		= e.ProcessInformation.dwProcessId;
	event.dwThreadId		= e.ProcessInformation.dwThreadId;

	// read the appripriate event entry type and emit the associated event.
	switch (e.EventId) {
		case EXCEPTION_DEBUG_EVENT:
			Read(frame.ExceptionEntry);
			EmitException(e.Time, frame.ExceptionEntry, event);
			break;
		case CREATE_PROCESS_DEBUG_EVENT:
			Read(frame.CreateProcessEntry);
			EmitCreateProcess(e.Time, frame.CreateProcessEntry, event);
			break;
		case CREATE_THREAD_DEBUG_EVENT:
			Read(frame.CreateThreadEntry);
			EmitCreateThread(e.Time, frame.CreateThreadEntry, event);
			break;
		case EXIT_PROCESS_DEBUG_EVENT:
			Read(frame.ExitProcessEntry);
			EmitExitProcess(e.Time, frame.ExitProcessEntry, event);
			break;
		case EXIT_THREAD_DEBUG_EVENT:
			Read(frame.ExitThreadEntry);
			EmitExitThread(e.Time, frame.ExitThreadEntry, event);
			break;
		case LOAD_DLL_DEBUG_EVENT:
			Read(frame.DllLoadEntry);
			EmitDllLoad(e.Time, frame.DllLoadEntry, event);
			break;
		case OUTPUT_DEBUG_STRING_EVENT:
			Read(frame.DebugStringEntry);
			EmitDebugString(e.Time, frame.DebugStringEntry, event);
			break;
		case RIP_EVENT:
			Read(frame.RipEntry);
			EmitRip(e.Time, frame.RipEntry, event);
			break;
		case UNLOAD_DLL_DEBUG_EVENT:
			Read(frame.DllUnloadEntry);
			EmitDllUnload(e.Time, frame.DllUnloadEntry, event);
			break;
		default:
			throw std::runtime_error("unexpected event frame type: " + std::to_string(e.EventId));
	}

	return true;
}

/// <summary>
/// Emit an exception debug event to all the exception handlers after reading all metadata (like paths and stack traces).
/// </summary>
/// <param name="time">The recorded time of the event.</param>
/// <param name="frame">The recorded frame of the event, containing relevant information.</param>
/// <param name="event">The DEBUG_EVENT instance.</param>
void BinaryLogPlayer::EmitException(time_t time, const ExceptionEventEntry& frame, DEBUG_EVENT& event) {
	char signature[4] = { 0 };
	std::shared_ptr<DebugContext> context;
	std::shared_ptr<DebugStackTrace> trace;
	StackTraceConcrete traceConcrete;
	WOW64_CONTEXT ctx32;
	CONTEXT ctx64;

	// get a PROCESS_INFORMATION struct from the frame
	auto pi = static_cast<PROCESS_INFORMATION>(frame.ProcessInformation);

	// Read the appropriately sized CPU context
	if (frame.Wow64) {
		Read(ctx32);
		context = std::make_shared<DebugContext>(pi, ctx32);
	} else {
		Read(ctx64);
		context = std::make_shared<DebugContext>(pi, ctx64);
	}

	// Set some members on the exception event struct.
	event.u.Exception.dwFirstChance						= frame.IsFirstChance;
	event.u.Exception.ExceptionRecord.ExceptionAddress	= reinterpret_cast<PVOID>(frame.EventAddress);
	event.u.Exception.ExceptionRecord.ExceptionCode		= frame.EventCode;

	// read trace 
	Read(signature, 4, false);
	if (_strnicmp(signature, "STCK", 4))
		throw std::runtime_error("stack trace expected, binary log file damaged");
	m_Stream.seekg(-4, std::ios::cur);
		
	// read the trace header in the first part of the StackTraceConcrete instance
	Read(reinterpret_cast<char*>(&traceConcrete), sizeof(StackTrace));

	// process all trace entries
	for (size_t i = 0; i < traceConcrete.TraceEntries; ++i) {
		// construct a StackTraceEntryConcrete, to which we can read a StackTraceEntry struct
		auto& entryConcrete = traceConcrete.Entries.emplace_back();

		// read relevant data 
		Read(reinterpret_cast<char*>(&entryConcrete), sizeof(StackTraceEntry));
		Read(entryConcrete.Name, entryConcrete.NameSymbolLength);	/* symbol at frame address */
		Read(entryConcrete.Path, entryConcrete.PathLength);			/* path to source file, if PDBs were found at the time of recording */

		// process all instructions that might have been decoded 
		for (size_t j = 0; j < entryConcrete.InstructionCount; ++j) {
			// construct a StackTraceEntryInstructionConcrete, to which we can read a StackTraceEntryInstruction struct
			auto& instructionConcrete = entryConcrete.Instructions.emplace_back();

			// read relevant data 
			Read(reinterpret_cast<char*>(&instructionConcrete), sizeof(StackTraceEntryInstruction));
			Read(instructionConcrete.Hex, instructionConcrete.HexSize);				/* instruction data in hexadecimal format */
			Read(instructionConcrete.Mnemonic, instructionConcrete.MnemonicSize);	/* the instruction mnemonic */
			Read(instructionConcrete.Operands, instructionConcrete.OperandsSize);	/* the instruction operands as one string */
		}
	}

	// should this event be emitted?
	if (m_ShouldFilter && ((frame.IsBreakpoint && !m_Filter.count("breakpoint")) || (!frame.IsBreakpoint && !m_Filter.count("exception"))))
		return;

	// normalize the stack trace based on the read data
	trace = std::make_shared<DebugStackTrace>(context, m_Modules, traceConcrete);

	// invoke handlers
	if (frame.IsBreakpoint) {
		for (auto handler : m_Handlers)
			handler->OnBreakpointHit(
				time,
				event.u.Exception,
				pi,
				context,
				trace,
				m_Modules
			);
	} else {
		std::wstring name = L"";
		if (Hindsight::Debugger::Debugger::ExceptionNames.count(frame.EventCode))
			name = Hindsight::Debugger::Debugger::ExceptionNames[frame.EventCode];

		for (auto handler : m_Handlers)
			handler->OnException(
				time,
				event.u.Exception,
				pi,
				frame.IsFirstChance,
				name,
				context,
				trace,
				m_Modules
			);
	}

	// handle breaking
	if (frame.IsBreakpoint) {
		if (m_State.BreakOnBreakpoints)
			HandleBreakpointOptions();
	} else {
		if (m_State.BreakOnExceptions && (!m_State.BreakOnFirstChanceOnly || frame.IsFirstChance))
			HandleBreakpointOptions();
	}
}

/// <summary>
/// Emit a CREATE_PROCESS debug event to all the debug event handlers after reading all metadata (like paths).
/// </summary>
/// <param name="time">The recorded time of the event.</param>
/// <param name="frame">The recorded frame of the event, containing relevant information.</param>
/// <param name="event">The DEBUG_EVENT instance.</param>
void BinaryLogPlayer::EmitCreateProcess(time_t time, const CreateProcessEventEntry& frame, DEBUG_EVENT& event) {
	std::wstring path;
	Read(path, frame.PathLength); /* read the full path of the created process as a unicode string */

	// fill the event struct with relevant information for the handlers
	event.u.CreateProcessInfo.hProcess		= reinterpret_cast<HANDLE>(frame.ProcessInformation.hProcess);
	event.u.CreateProcessInfo.hThread		= reinterpret_cast<HANDLE>(frame.ProcessInformation.hThread);
	event.u.CreateProcessInfo.lpBaseOfImage = reinterpret_cast<LPVOID>(frame.ModuleBase);

	// simulate a module load, so that the handlers can resolve addresses to this module
	m_Modules.Load(path, reinterpret_cast<ModulePointer>(frame.ModuleBase), frame.ModuleSize);

	// should this event be emitted?
	if (m_ShouldFilter && !m_Filter.count("create_process"))
		return;

	// get a PROCESS_INFORMATION struct
	auto pi = static_cast<PROCESS_INFORMATION>(frame.ProcessInformation);

	// invoke handlers
	for (auto handler : m_Handlers)
		handler->OnCreateProcess(
			time,
			event.u.CreateProcessInfo,
			pi,
			path,
			m_Modules
		);
}

/// <summary>
/// Emit a CREATE_THREAD debug event to all the debug event handlers after reading all metadata (like paths).
/// </summary>
/// <param name="time">The recorded time of the event.</param>
/// <param name="frame">The recorded frame of the event, containing relevant information.</param>
/// <param name="event">The DEBUG_EVENT instance.</param>
void BinaryLogPlayer::EmitCreateThread(time_t time, const CreateThreadEventEntry& frame, DEBUG_EVENT& event) {
	// fill the event struct with relevant information, in this case just the entrypoint address of the thread.
	// through this entry point and the simulated module loading, the loaded module containing this address can 
	// be resolved by the handlers.
	event.u.CreateThread.lpStartAddress = reinterpret_cast<LPTHREAD_START_ROUTINE>(frame.EntryPointAddress);

	// should this event be emitted?
	if (m_ShouldFilter && !m_Filter.count("create_thread"))
		return;

	// get a PROCESS_INFORMATION struct
	auto pi = static_cast<PROCESS_INFORMATION>(frame.ProcessInformation);

	// invoke handlers
	for (auto handler : m_Handlers)
		handler->OnCreateThread(
			time,
			event.u.CreateThread,
			pi,
			m_Modules
		);
}

/// <summary>
/// Emit an LOAD_DLL debug event to all the debug event handlers after reading all metadata (like paths).
/// </summary>
/// <param name="time">The recorded time of the event.</param>
/// <param name="frame">The recorded frame of the event, containing relevant information.</param>
/// <param name="event">The DEBUG_EVENT instance.</param>
void BinaryLogPlayer::EmitDllLoad(time_t time, const DllLoadEventEntry& frame, DEBUG_EVENT& event) {
	std::wstring path;
	Read(path, frame.ModulePathSize); /* read the full path of the created process as a unicode string */

	// set the base address of the module that was loaded
	event.u.LoadDll.lpBaseOfDll = reinterpret_cast<LPVOID>(frame.ModuleBase);

	// simulate a module load, see EmitCreateProcess why
	m_Modules.Load(path, event.u.LoadDll.lpBaseOfDll, frame.ModuleSize);

	// should this event be emitted?
	if (m_ShouldFilter && !m_Filter.count("load_dll"))
		return;

	// get a PROCESS_INFORMATION struct
	auto pi = static_cast<PROCESS_INFORMATION>(frame.ProcessInformation);

	// invoke handlers
	for (auto handler : m_Handlers)
		handler->OnDllLoad(
			time,
			event.u.LoadDll,
			pi,
			path,
			m_Modules.GetIndex(path),
			m_Modules
		);
}

/// <summary>
/// Emit an EXIT_PROCESS debug event to all the debug event handlers after reading all metadata (like paths).
/// </summary>
/// <param name="time">The recorded time of the event.</param>
/// <param name="frame">The recorded frame of the event, containing relevant information.</param>
/// <param name="event">The DEBUG_EVENT instance.</param>
void BinaryLogPlayer::EmitExitProcess(time_t time, const ExitProcessEventEntry& frame, DEBUG_EVENT& event) {
	// fill the event struct with relevant information, in this case merely the exit code.
	event.u.ExitProcess.dwExitCode = frame.ExitCode;

	// should this event be emitted?
	if (m_ShouldFilter && !m_Filter.count("exit_process"))
		return;

	// get a PROCESS_INFORMATION struct
	auto pi = static_cast<PROCESS_INFORMATION>(frame.ProcessInformation);

	// invoke handlers
	for (auto handler : m_Handlers)
		handler->OnExitProcess(
			time,
			event.u.ExitProcess,
			pi,
			m_Modules
		);
}

/// <summary>
/// Emit an EXIT_THREAD debug event to all the debug event handlers after reading all metadata (like paths).
/// </summary>
/// <param name="time">The recorded time of the event.</param>
/// <param name="frame">The recorded frame of the event, containing relevant information.</param>
/// <param name="event">The DEBUG_EVENT instance.</param>
void BinaryLogPlayer::EmitExitThread(time_t time, const ExitThreadEventEntry& frame, DEBUG_EVENT& event) {
	// fill the event struct with relevant information, in this case merely the exit code.
	event.u.ExitProcess.dwExitCode = frame.ExitCode;

	// should this event be emitted?
	if (m_ShouldFilter && !m_Filter.count("exit_thread"))
		return;

	// get a PROCESS_INFORMATION struct
	auto pi = static_cast<PROCESS_INFORMATION>(frame.ProcessInformation);

	// invoke handlers
	for (auto handler : m_Handlers)
		handler->OnExitThread(
			time,
			event.u.ExitThread,
			pi,
			m_Modules
		);
}

/// <summary>
/// Emit an OUTPUT_DEBUG_STRING debug event to all the debug event handlers after reading all metadata (like paths).
/// </summary>
/// <param name="time">The recorded time of the event.</param>
/// <param name="frame">The recorded frame of the event, containing relevant information.</param>
/// <param name="event">The DEBUG_EVENT instance.</param>
void BinaryLogPlayer::EmitDebugString(time_t time, const DebugStringEventEntry& frame, DEBUG_EVENT& event) {
	// fill the event struct with relevant information
	event.u.DebugString.fUnicode = frame.IsUnicode;
	event.u.DebugString.nDebugStringLength = static_cast<WORD>(frame.Length);

	// get a PROCESS_INFORMATION struct
	auto pi = static_cast<PROCESS_INFORMATION>(frame.ProcessInformation);

	if (frame.IsUnicode) { /* decode as an MS wchar_t string */
		std::wstring message;
		Read(message, frame.Length);

		// should this event be emitted?
		if (m_ShouldFilter && !m_Filter.count("debug"))
			return;

		// invoke handlers
		for (auto handler : m_Handlers)
			handler->OnDebugStringW(time, event.u.DebugString, pi, message);
	} else { /* regular ANSI */
		std::string message;
		Read(message, frame.Length);

		// should this event be emitted?
		if (m_ShouldFilter && !m_Filter.count("debug"))
			return;

		// invoke handlers
		for (auto handler : m_Handlers)
			handler->OnDebugString(time, event.u.DebugString, pi, message);
	}
}

/// <summary>
/// Emit an RIP debug event to all the debug event handlers after reading all metadata (like paths).
/// </summary>
/// <param name="time">The recorded time of the event.</param>
/// <param name="frame">The recorded frame of the event, containing relevant information.</param>
/// <param name="event">The DEBUG_EVENT instance.</param>
void BinaryLogPlayer::EmitRip(time_t time, const RipEventEntry& frame, DEBUG_EVENT& event) {
	// fill the event struct with relevant information
	event.u.RipInfo.dwError = frame.Error;
	event.u.RipInfo.dwType  = frame.Type;

	// should this event be emitted?
	if (m_ShouldFilter && !m_Filter.count("rip"))
		return;

	// get a PROCESS_INFORMATION struct
	auto pi = static_cast<PROCESS_INFORMATION>(frame.ProcessInformation);

	// try formatting the error code and invoke handlers.
	auto message = Hindsight::Utilities::Error::GetErrorMessageW(event.u.RipInfo.dwError);
	for (auto handler : m_Handlers)
		handler->OnRip(time, event.u.RipInfo, pi, message);
}

/// <summary>
/// Emit an UNLOAD_DLL debug event to all the debug event handlers after reading all metadata (like paths).
/// </summary>
/// <param name="time">The recorded time of the event.</param>
/// <param name="frame">The recorded frame of the event, containing relevant information.</param>
/// <param name="event">The DEBUG_EVENT instance.</param>
void BinaryLogPlayer::EmitDllUnload(time_t time, const DllUnloadEventEntry& frame, DEBUG_EVENT& event) {
	// set the module base address to the event struct
	event.u.UnloadDll.lpBaseOfDll = reinterpret_cast<LPVOID>(frame.ModuleBase);

	// get a PROCESS_INFORMATION struct
	auto pi = static_cast<PROCESS_INFORMATION>(frame.ProcessInformation);

	// only when not filtering or when unload_dll is included in the filter
	if (!m_ShouldFilter || (m_ShouldFilter && m_Filter.count("unload_dll"))) {
		auto path = m_Modules.Get(event.u.UnloadDll.lpBaseOfDll);
		for (auto handler : m_Handlers)
			handler->OnDllUnload(
				time,
				event.u.UnloadDll, 
				pi, 
				path, 
				m_Modules.GetIndex(path),
				m_Modules);
	}

	// simulate the unload in our internal collection too, keep track of which modules 
	// are still loaded so that address resolution is correct. We do this after the 
	// handlers are invoked, so that handlers can still resolve the module name.
	m_Modules.Unload(event.u.UnloadDll.lpBaseOfDll);
}

/// <summary>
/// Determines the size of the binary log stream, thus the filesize.
/// </summary>
/// <returns>An integer representing the filesize of this binary log file.</returns>
inline size_t BinaryLogPlayer::Size() {
	return m_StreamSize;
}

/// <summary>
/// Determines the current location the player is reading from in the stream.
/// </summary>
/// <returns>An integer representing the current location in the binary log file stream.</returns>
inline size_t BinaryLogPlayer::Pos() {
	return static_cast<size_t>(m_Stream.tellg());
}

/// <summary>
/// Determines the number of bytes that are remaining in the stream to be read.
/// </summary>
/// <returns>An integer representing the remaining size in the stream.</returns>
inline size_t BinaryLogPlayer::SizeLeft() {
	return Size() - Pos();
}

/// <summary>
/// Determines the number of bytes that are remaining in the stream to be read, and 
/// verifies that it is enough to meet the value specified in <paramref name="required"/>.
/// </summary>
/// <param name="required">The number of bytes required to be available in the stream.</param>
inline void BinaryLogPlayer::AssertSizeLeft(size_t required) {
	if (SizeLeft() < required)
		throw std::runtime_error("unexpected end of binary log file, expected more data.");
}

/// <summary>
/// Read a value T from the stream directly to the value referenced by <paramref name="result"/>.
/// </summary>
/// <param name="result">A reference to a <typeparamref name="T"/> value to read to, which must be a class or struct.</param>
/// <typeparam name="T">The type of value to read from the stream.</typeparam>
template <typename T, std::enable_if_t<std::is_class<T>::value, int>>
void BinaryLogPlayer::Read(T& result) {
	Read(reinterpret_cast<char*>(&result), sizeof(T));
}

/// <summary>
/// Read a value T from the stream directly to the value referenced by <paramref name="result"/>.
/// </summary>
/// <param name="result">A reference to a <typeparamref name="T"/> value to read to, which must be an integer.</param>
/// <typeparam name="T">The type of value to read from the stream.</typeparam>
template <typename T, std::enable_if_t<std::is_integral<T>::value, int>>
void BinaryLogPlayer::Read(T& result) {
	Read(reinterpret_cast<char*>(&result), sizeof(T));
}

/// <summary>
/// Read <paramref name="size"/> bytes from the stream into <paramref name="buffer"/> and optionally update the checksum.
/// </summary>
/// <param name="buffer">The buffer to read to, it must point to enough memory to hold <paramref name="size"/> bytes.</param>
/// <param name="size">The number of bytes to read.</param>
/// <param name="updateChecksum">Whether to update the internal checksum or not, used in verifying data integrity.</param>
void BinaryLogPlayer::Read(char* buffer, size_t size, bool updateChecksum) {
	AssertSizeLeft(size);
	m_Stream.read(buffer, size);

	if (updateChecksum)
		m_Crc32 = Hindsight::Checksum::Crc32::Update(buffer, size, m_Crc32);
}

/// <summary>
/// Read an ANSI string from the stream to <paramref name="result"/>. When <paramref name="size"/> is specified, it will read 
/// exactly that many characters (not bytes). If it is not specified, it will read the length first as a <see cref="uint32_t"/>.
/// </summary>
/// <param name="result"></param>
/// <param name="size"></param>
void BinaryLogPlayer::Read(std::string& result, int64_t size) {
	if (size == -1) {
		uint32_t readSize;
		Read(readSize);
		size = readSize;
	}

	result.resize(size);
	Read(&result[0], size * sizeof(char));
}

/// <summary>
/// Read a unicode string from the stream to <paramref name="result"/>. When <paramref name="size"/> is specified, it will read 
/// exactly that many characters (not bytes). If it is not specified, it will read the length first as a <see cref="uint32_t"/>.
/// </summary>
/// <param name="result">A reference to a <see cref="::std::wstring" /> instance that will hold the result.</param>
/// <param name="size">The number of characters to read, or -1 to read the length from the stream.</param>
void BinaryLogPlayer::Read(std::wstring& result, int64_t size) {
	if (size == -1) {
		uint32_t readSize;
		Read(readSize);
		size = readSize;
	}

	result.resize(size);
	Read(reinterpret_cast<char*>(&result[0]), size * sizeof(wchar_t));
}

/// <summary>
/// Read a vector of <paramref name="count"/> ANSI strings from the stream to <paramref name="result"/>.
/// </summary>
/// <param name="result">A reference to a <see cref="::std::vector<::std::string>"/> instance that will hold the result.</param>
/// <param name="count">The number of strings to read.</param>
/// <remarks>note: the length of each string in the vector is read before reading the string, like in <see cref="::Hindsight::BinaryLog::BinaryLogPlayer::Read(std::string& result, int64_t size)"/></remarks>
void BinaryLogPlayer::Read(std::vector<std::string>& result, size_t count) {
	if (count == 0)
		return;

	for (size_t i = 0; i < count; ++i) {
		result.emplace_back();
		Read(result.back());
	}
}

/// <summary>
/// A breakpoint or exception was emitted and the user specified either or both the --break-breakpoint or --break-exception flags, 
/// this method handles the actual breaking.
/// </summary>
void BinaryLogPlayer::HandleBreakpointOptions() {
	char choice;

	do {
		std::wcout << L"[c]ontinue or [a]bort?" << std::endl;
		choice = std::tolower(_getch());
	} while (choice != 'c' && choice != 'a');

	if (choice == 'a') 
		exit(0);
}