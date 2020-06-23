#include "BinaryLogFile.hpp"
#include <DbgHelp.h>
#include <chrono>

using namespace Hindsight::BinaryLog;

/// <summary>
/// Construct a new EventEntryProcessInformation struct instance from a const reference to a <see cref="PROCESS_INFORMATION"/> instance.
/// </summary>
/// <param name="pi">The process information.</param>
EventEntryProcessInformation::EventEntryProcessInformation(const PROCESS_INFORMATION& pi) 
	: hProcess(reinterpret_cast<uint64_t>(pi.hProcess)), 
	  hThread(reinterpret_cast<uint64_t>(pi.hThread)), 
	  dwProcessId(pi.dwProcessId), 
	  dwThreadId(pi.dwThreadId) {

}

/// <summary>
/// Cast this struct back to a <see cref="PROCESS_INFORMATION"/> struct instance.
/// </summary>
/// <returns>A newly created <see cref="PROCESS_INFORMATION"/> struct.</returns>
EventEntryProcessInformation::operator PROCESS_INFORMATION() const {
	return {
		reinterpret_cast<HANDLE>(hProcess),
		reinterpret_cast<HANDLE>(hThread),
		dwProcessId,
		dwThreadId
	};
}

/// <summary>
/// Default constructor, generally used when reading an existing binary log file.
/// </summary>
EventEntry::EventEntry() {}

/// <summary>
/// Construct an EventEntry instance from a <see cref="PROCESS_INFORMATION"/> instance <paramref name="pi"/>, <paramref name="eventId"/> and <paramref name="size"/>.
/// The size indicates the size of the concrete event entry.
/// </summary>
/// <param name="pi">A <see cref="PROCESS_INFORMATION"/> instance containing a handle to the process and thread of this event.</param>
/// <param name="eventId">The event ID.</param>
/// <param name="size">The size of the concrete event entry instance.</param>
EventEntry::EventEntry(const PROCESS_INFORMATION& pi, uint32_t eventId, size_t size)
	: ProcessInformation(pi), EventId(eventId), Size(size), Time(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())) {

}

/// <summary>
/// Default constructor, generally used when reading an existing binary log file.
/// </summary>
ExceptionEventEntry::ExceptionEventEntry() {}

/// <summary>
/// Construct an ExceptionEventEntry instance from parameters obtained through a debug event whilst debugging. When parameters are 
/// obtained from a JIT debug event, they are translated to a regular debug event and provided to this constructor.
/// </summary>
/// <param name="pi">A <see cref="PROCESS_INFORMATION"/> instance containing a handle to the process and thread of this event.</param>
/// <param name="addr">The exception address.</param>
/// <param name="code">The exception code.</param>
/// <param name="iswow64">A boolean indicating whether <paramref name="pi"/> refers to a process running in WOW64 mode or not.</param>
/// <param name="breakpoint">A boolean indicating whether this event is a breakpoint-exception, or a regular exception.</param>
/// <param name="firstChance">A boolean indicating whether this is a first chance exception.</param>
ExceptionEventEntry::ExceptionEventEntry(const PROCESS_INFORMATION& pi, uint64_t addr, uint32_t code, bool iswow64, bool breakpoint, bool firstChance)
	: EventEntry(pi, EXCEPTION_DEBUG_EVENT, sizeof(ExceptionEventEntry)), 
	  EventAddress(addr), EventCode(code), Wow64(iswow64), IsBreakpoint(breakpoint), IsFirstChance(firstChance) {

}

/// <summary>
/// Default constructor, generally used when reading an existing binary log file.
/// </summary>
CreateProcessEventEntry::CreateProcessEventEntry() {}

/// <summary>
/// Construct a CreateProcessEventEntry instance from parameters obtained through a debug event whilst debugging.
/// </summary>
/// <param name="pi">A <see cref="PROCESS_INFORMATION"/> instance containing a handle to the process and thread of this event.</param>
/// <param name="path">A full path to the main module of the created process.</param>
/// <param name="base">The base address for the main module of the created process.</param>
/// <param name="size">The full size in memory of the main module of the created process.</param>
CreateProcessEventEntry::CreateProcessEventEntry(const PROCESS_INFORMATION& pi, const std::wstring& path, uint64_t base, uint64_t size)
	: EventEntry(pi, CREATE_PROCESS_DEBUG_EVENT, sizeof(CreateProcessEventEntry)),
	  PathLength(path.size()), ModuleBase(base), ModuleSize(size) {

}

/// <summary>
/// Default constructor, generally used when reading an existing binary log file.
/// </summary>
CreateThreadEventEntry::CreateThreadEventEntry() {}

/// <summary>
/// Construct a CreateThreadEventEntry from parameters obtained through a debug event whilst debugging.
/// </summary>
/// <param name="pi">A <see cref="PROCESS_INFORMATION"/> instance containing a handle to the process and thread of this event.</param>
/// <param name="entryPoint">The address of the entrypoint of the thread.</param>
/// <seealso cref="LPTHREAD_START_ROUTINE"/>
CreateThreadEventEntry::CreateThreadEventEntry(const PROCESS_INFORMATION& pi, uint64_t entryPoint)
	: EventEntry(pi, CREATE_THREAD_DEBUG_EVENT, sizeof(CreateThreadEventEntry)),
      EntryPointAddress(entryPoint) {

}

/// <summary>
/// Default constructor, generally used when reading an existing binary log file.
/// </summary>
ExitProcessEventEntry::ExitProcessEventEntry() {}

/// <summary>
/// Construct a ExitProcessEventEntry from parameters obtained through a debug event whilst debugging.
/// </summary>
/// <param name="pi">A <see cref="PROCESS_INFORMATION"/> instance containing a handle to the process and thread of this event.</param>
/// <param name="exitCode">The process exit code.</param>
ExitProcessEventEntry::ExitProcessEventEntry(const PROCESS_INFORMATION& pi, uint32_t exitCode)
	: EventEntry(pi, EXIT_PROCESS_DEBUG_EVENT, sizeof(ExitProcessEventEntry)),
      ExitCode(exitCode) {

}

/// <summary>
/// Default constructor, generally used when reading an existing binary log file.
/// </summary>
ExitThreadEventEntry::ExitThreadEventEntry() {}

/// <summary>
/// Construct a ExitThreadEventEntry from parameters obtained through a debug event whilst debugging.
/// </summary>
/// <param name="pi">A <see cref="PROCESS_INFORMATION"/> instance containing a handle to the process and thread of this event.</param>
/// <param name="exitCode">The thread exit code.</param>
ExitThreadEventEntry::ExitThreadEventEntry(const PROCESS_INFORMATION& pi, uint32_t exitCode) 
	: EventEntry(pi, EXIT_THREAD_DEBUG_EVENT, sizeof(ExitThreadEventEntry)),
	  ExitCode(exitCode) {

}

/// <summary>
/// Default constructor, generally used when reading an existing binary log file.
/// </summary>
DllLoadEventEntry::DllLoadEventEntry() {}

/// <summary>
/// Construct a DllLoadEventEntry from parameters obtained through a debug event whilst debugging.
/// </summary>
/// <param name="pi">A <see cref="PROCESS_INFORMATION"/> instance containing a handle to the process and thread of this event.</param>
/// <param name="index">The module index in the debugger's LoadedModules collection, which can be used to lookup a module later.</param>
/// <param name="base">The module base address.</param>
/// <param name="size">The full module size as loaded in memory.</param>
/// <param name="pathSize">The length of the full file path of the loaded module, which is to be written after this struct.</param>
DllLoadEventEntry::DllLoadEventEntry(const PROCESS_INFORMATION& pi, int64_t index, uint64_t base, uint64_t size, uint64_t pathSize) 
	: EventEntry(pi, LOAD_DLL_DEBUG_EVENT, sizeof(DllLoadEventEntry)),
	  ModuleIndex(index), ModuleBase(base), ModuleSize(size), ModulePathSize(pathSize) {

}

/// <summary>
/// Default constructor, generally used when reading an existing binary log file.
/// </summary>
DebugStringEventEntry::DebugStringEventEntry() {}

/// <summary>
/// Construct a DllLoadEventEntry from parameters obtained through a debug event whilst debugging.
/// </summary>
/// <param name="pi">A <see cref="PROCESS_INFORMATION"/> instance containing a handle to the process and thread of this event.</param>
/// <param name="unicode">An integer (non-zero or zero) indicating whether the string is wide-char unicode or not.</param>
/// <param name="length">The length of the debug string.</param>
DebugStringEventEntry::DebugStringEventEntry(const PROCESS_INFORMATION& pi, uint8_t unicode, uint64_t length)
	: EventEntry(pi, OUTPUT_DEBUG_STRING_EVENT, sizeof(DebugStringEventEntry)),
	  IsUnicode(unicode), Length(length) {

}

/// <summary>
/// Default constructor, generally used when reading an existing binary log file.
/// </summary>
RipEventEntry::RipEventEntry() {}

/// <summary>
/// Construct a RipEventEntry from parameters obtained through a debug event whilst debugging.
/// </summary>
/// <param name="pi">A <see cref="PROCESS_INFORMATION"/> instance containing a handle to the process and thread of this event.</param>
/// <param name="type">The type of RIP event.</param>
/// <param name="error">The error code.</param>
RipEventEntry::RipEventEntry(const PROCESS_INFORMATION& pi, uint32_t type, uint32_t error)
	: EventEntry(pi, RIP_EVENT, sizeof(RipEventEntry)),
	  Type(type), Error(error) {

}

/// <summary>
/// Default constructor, generally used when reading an existing binary log file.
/// </summary>
DllUnloadEventEntry::DllUnloadEventEntry() {}

/// <summary>
/// Construct a DllUnloadEventEntry from parameters obtained through a debug event whilst debugging.
/// </summary>
/// <param name="pi">A <see cref="PROCESS_INFORMATION"/> instance containing a handle to the process and thread of this event.</param>
/// <param name="base">The module base address of the DLL that has been unloaded.</param>
DllUnloadEventEntry::DllUnloadEventEntry(const PROCESS_INFORMATION& pi, uint64_t base) 
	: EventEntry(pi, UNLOAD_DLL_DEBUG_EVENT, sizeof(DllUnloadEventEntry)),
	  ModuleBase(base) {

}

#pragma warning( push )
#pragma warning( disable : 26495 ) /* no initialization required, done by debugger writing to this union */
/// <summary>
/// Default constructor, no initialization. Initialization is managed by the debugger.
/// </summary>
EntryFrame::EntryFrame() {

}
#pragma warning( pop )

/// <summary>
/// Default constructor, generally used when reading an existing binary log file.
/// </summary>
StackTrace::StackTrace() {}

/// <summary>
/// Construct a StackTrace header.
/// </summary>
/// <param name="recursion">The maximum recursion for a stack trace before the entries between get cut out.</param>
/// <param name="instructions">The maximum number of instructions to decode at the address of the start of the stack trace.</param>
/// <param name="entries">The number of trace entries (frames).</param>
StackTrace::StackTrace(uint64_t recursion, uint64_t instructions, uint64_t entries)
	: MaxRecursion(recursion), MaxInstructions(instructions), TraceEntries(entries) {

}