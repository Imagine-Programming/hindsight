#include "PrintingDebuggerEventHandler.hpp"
#include "Error.hpp"
#include "String.hpp"
#include "rang.hpp"

#include <sstream>
#include <ctime>
#include <algorithm>
#include <iterator>
#include <iomanip>
#include <iostream>
#include <ios>

/// <summary>
/// When colorization is enabled, rang_color will switch to the color defined in c
/// </summary>
#define rang_color(c) \
	if (m_Colorize) std::cout << (c)

/// <summary>
/// Invoke rang_color(c) and end the macro with m_WStream.
/// </summary>
/// <example>
/// <code>
/// rang_color_wstream(rang::fgB::red) << L"error: something or other" << std::endl;
/// </code>
/// </example>
#define rang_color_wstream(c) \
	rang_color((c)); \
	m_WStream

/// <summary>
/// Dump the current timestamp to the output if timestamps ought to be printed.
/// </summary>
#define rang_timestamp() \
	if (m_Timestamps) { \
		rang_color_wstream(rang::fg::gray) \
			<< L"(" << timestamp(time) << L") "; \
	}

/// <summary>
/// Reset rang style formatting.
/// </summary>
#define rang_reset() \
	if (m_Colorize) std::cout << rang::style::reset

using namespace Hindsight::Debugger;
using namespace Hindsight::Debugger::EventHandler;
using namespace Hindsight::Utilities;

/// <summary>
/// Add a stream operator overload for streaming <see cref="::std::string"/> instances
/// to a wchar_t stream (<see cref="::std::wostream"/>). 
/// </summary>
/// <param name="ostr">The wchar_t output stream reference.</param>
/// <param name="str">A const reference to the string to write to the stream.</param>
/// <returns>The wchar_t output stream defined in <paramref name="ostr"/>.</returns>
std::wostream& operator<< (std::wostream & ostr, std::string const& str) {
	std::copy(str.begin(), str.end(), std::ostream_iterator<char, wchar_t>(ostr));
	return (ostr);
}

/// <summary>
/// Format a <see cref="::std::time_t"/>
/// </summary>
/// <param name="t">The time to format.</param>
/// <returns>A basic formatted time.</returns>
static std::string timestamp(time_t t) {
	std::stringstream ss;
	tm tm;
	/* 
		Could've used std::localtime, but it is considered unsafe as the returned pointer
		points to memory that is going to be reused. localtime_s is safer, as you allocate
		the tm struct yourself.
	*/
	localtime_s(&tm, &t); 
	ss << std::put_time(&tm, "%d/%m/%Y %H:%M:%S");
	return ss.str();
}

/// <summary>
/// Construct a new PrintingDebuggerEventHandler for outputs streams with colorization support.
/// </summary>
/// <param name="stream">The output stream.</param>
/// <param name="wstream">The unicode output stream.</param>
/// <param name="colorize">When set to true, this handler will attempt colorizing the output.</param>
/// <param name="times">When set to true, timestamps will be printed before each event indication.</param>
/// <param name="printContext">When set to true, thread contexts will be printed before stack traces.</param>
PrintingDebuggerEventHandler::PrintingDebuggerEventHandler(ostream& stream, wostream& wstream, bool colorize, bool times, bool printContext) :
	m_Stream(stream), 
	m_WStream(wstream), 
	m_Colorize(colorize), 
	m_Timestamps(times),
	m_PrintContext(printContext), 
	m_StreamFlags(m_Stream.flags()), 
	m_WStreamFlags(m_WStream.flags()) {

}

/// <summary>
/// Construct a new PrintingDebuggerEventHandler for std::cout and std::wcout with colorization support. 
/// </summary>
/// <param name="colorize">When set to true, this handler will attempt colorizing the output.</param>
/// <param name="times">When set to true, timestamps will be printed before each event indication.</param>
/// <param name="printContext">When set to true, thread contexts will be printed before stack traces.</param>
PrintingDebuggerEventHandler::PrintingDebuggerEventHandler(bool colorize, bool times, bool printContext) :
	m_Stream(std::cout),
	m_WStream(std::wcout),
	m_Colorize(colorize), 
	m_Timestamps(times),
	m_PrintContext(printContext), 
	m_StreamFlags(m_Stream.flags()),
	m_WStreamFlags(m_WStream.flags()) {

}

/// <summary>
/// Construct a new PrintingDebuggerEventHandler for a wide-char stream, without colorization support. This is 
/// because there is no ANSI stream for rang to use when setting styles.
/// </summary>
/// <param name="wstream">The wide-char output stream.</param>
/// <param name="printContext">When set to true, thread contexts will be printed before stack traces.</param>
/// <remarks>
/// By default, this constructor enabled printing timestamps. The reason is that this constructor is used 
/// internally to supply initialization to the constructor for an output file, and writing times is default 
/// for output files.
/// </remarks>
PrintingDebuggerEventHandler::PrintingDebuggerEventHandler(wostream& wstream, bool printContext) : 
	m_Stream(std::cout), 
	m_WStream(wstream), 
	m_Colorize(false),
	m_Timestamps(true),
	m_PrintContext(printContext), 
	m_StreamFlags(m_Stream.flags()),
	m_WStreamFlags(m_WStream.flags()) {

}

/// <summary>
/// Construct a new PrintingDebuggerEventHandler for a wide-char file stream, which is will open as well. This
/// is the constructor to use when outputting to file rather than to a console stream.
/// </summary>
/// <param name="file">The path to the unicode text file that will contain the full logging of the debug session.</param>
/// <param name="printContext">When set to true, thread contexts will be printed before stack traces.</param>
PrintingDebuggerEventHandler::PrintingDebuggerEventHandler(const std::string& file, bool printContext) : 
	m_Stream(std::cout), 
	m_FStream(std::wofstream(file, std::ios::trunc)), 
	m_WStream(m_FStream), 
	m_Colorize(false), 
	m_Timestamps(true),
	m_PrintContext(printContext),
	m_StreamFlags(m_Stream.flags()), 
	m_WStreamFlags(m_WStream.flags()) {

}

/// <summary>
/// Restore the flags of the output streams in this instance.
/// </summary>
inline void PrintingDebuggerEventHandler::RestoreFlags() const {
	m_Stream.flags(m_StreamFlags);
	m_WStream.flags(m_WStreamFlags);
}

/// <summary>
/// Handle the initialization event, prints the first known information.
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="p">The process being debugged.</param>
void PrintingDebuggerEventHandler::OnInitialization(
	time_t time,
	const std::shared_ptr<const Hindsight::Process::Process> p) {

	size_t		width = 12;
	char		padchar = ' ';
	std::string separator = "\" \"";

	rang_timestamp();

	rang_color_wstream(rang::fg::green)
		<< L"Attached to process 0x" << std::hex << p->dwProcessId << std::dec << std::endl;

	rang_color_wstream(rang::fg::cyan)
		<< String::PadRight("Path: ", width, padchar);

	rang_color_wstream(rang::fgB::cyan)
		<< p->Path << std::endl;

	rang_color_wstream(rang::fg::cyan)
		<< String::PadRight("WorkDir: ", width, padchar);

	rang_color_wstream(rang::fgB::cyan)
		<< p->WorkingDirectory << std::endl;

	rang_color_wstream(rang::fg::cyan)
		<< String::PadRight("Arguments: ", width, padchar);

	if (p->Arguments.empty()) {
		m_WStream << std::endl;
	} else {
		rang_color_wstream(rang::fgB::cyan)
			<< L"\"" << String::Join(p->Arguments, separator) << L"\"" << std::endl;
	}

	rang_reset();

	RestoreFlags();
}

/// <summary>
/// Log a breakpoint and stacktrace of the breakpoint address, with optionally a thread context.
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="info">A const reference to the <see cref="EXCEPTION_DEBUG_INFO"/> struct instance with event information.</param>
/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
/// <param name="context">A shared pointer to a const <see cref="::Hindsight::Debugger::DebugContext"/> instance.</param>
/// <param name="trace">A shared pointer to a const <see cref="::Hindsight::Debugger::DebugStackTrace"/> instance.</param>
/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
void PrintingDebuggerEventHandler::OnBreakpointHit(
	time_t time,
	const EXCEPTION_DEBUG_INFO& info, 
	const PROCESS_INFORMATION& pi,
	std::shared_ptr<const DebugContext> context, 
	std::shared_ptr<const DebugStackTrace> trace, 
	const ModuleCollection& collection) {

	rang_timestamp();

	rang_color_wstream(rang::fg::green) 
		<< L"[BREAK] ";

	rang_color_wstream(rang::fgB::green)
		<< L"(" << std::hex << L"0x" << info.ExceptionRecord.ExceptionCode << L")";

	rang_color_wstream(rang::fg::yellow) 
		<< GetAddressDescriptor(info.ExceptionRecord.ExceptionAddress, collection) << std::endl;

	rang_reset();

	if (m_PrintContext)
		PrintContext(context);

	PrintStackTrace(pi, context, trace, collection);
}

/// <summary>
/// Log an exception and stacktrace of the exception address, with optionally a thread context. 
/// This method also receives an exception name, if it is a known system error.
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="info">A const reference to the <see cref="EXCEPTION_DEBUG_INFO"/> struct instance with event information.</param>
/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
/// <param name="firstChance">A boolean indicating that this is the first encounter with this specific exception instance, or not.</param>
/// <param name="name">A name of a known exception, or an empty string.</param>
/// <param name="context">A shared pointer to a const <see cref="::Hindsight::Debugger::DebugContext"/> instance.</param>
/// <param name="trace">A shared pointer to a const <see cref="::Hindsight::Debugger::DebugStackTrace"/> instance.</param>
/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
/// <param name="ertti">A shared pointer to a const <see cref="::Hindsight::Debugger::CxxExceptions::ExceptionRtti"/> instance.</param>
void PrintingDebuggerEventHandler::OnException(
	time_t time,
	const EXCEPTION_DEBUG_INFO& info, 
	const PROCESS_INFORMATION& pi,
	bool firstChance, 
	const std::wstring& name,
	std::shared_ptr<const DebugContext> context, 
	std::shared_ptr<const DebugStackTrace> trace, 
	const ModuleCollection& collection,
	std::shared_ptr<const CxxExceptions::ExceptionRunTimeTypeInformation> ertti) {

	rang_timestamp();

	rang_color_wstream(rang::fgB::red)
		<< L"[EXCEPT] ";

	rang_color_wstream(rang::fg::red)
		<< L"(0x" << std::hex << info.ExceptionRecord.ExceptionCode << std::dec << L")";

	rang_color_wstream(rang::fg::yellow)
		<< GetAddressDescriptor(info.ExceptionRecord.ExceptionAddress, collection);

	if (firstChance) {
		rang_color_wstream(rang::fg::magenta)
			<< L", first chance";
	}

	if (!name.empty()) {
		m_WStream << L": ";
		rang_color_wstream(rang::fgB::red)
			<< name;
	} 

	m_WStream << std::endl;

	rang_reset();

	if (ertti != nullptr)
		PrintRtti(ertti);

	if (m_PrintContext)
		PrintContext(context);

	PrintStackTrace(pi, context, trace, collection);
}

/// <summary>
/// Log the process creation, with its PID and full path.
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="info">A const reference to the <see cref="CREATE_PROCESS_DEBUG_INFO"/> struct instance with event information.</param>
/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
/// <param name="path">The full path to the loaded module on file system.</param>
/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
void PrintingDebuggerEventHandler::OnCreateProcess(
	time_t time,
	const CREATE_PROCESS_DEBUG_INFO& info, 
	const PROCESS_INFORMATION& pi,
	const std::wstring& path,
	const ModuleCollection& collection) {

	rang_timestamp();

	rang_color_wstream(rang::fgB::green)
		<< L"[CREATE PROCESS] ";

	rang_color_wstream(rang::fgB::cyan)
		<< L"0x" << std::hex << pi.dwProcessId;

	rang_reset();

	m_WStream << L" " << path << std::endl;
}

/// <summary>
/// Log thread creation, with its TID, module and entrypoint offset in the module.
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="info">A const reference to the <see cref="CREATE_THREAD_DEBUG_INFO"/> struct instance with event information.</param>
/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
void PrintingDebuggerEventHandler::OnCreateThread(
	time_t time,
	const CREATE_THREAD_DEBUG_INFO& info, 
	const PROCESS_INFORMATION& pi,
	const ModuleCollection& collection) {

	rang_timestamp();

	rang_color_wstream(rang::fgB::green)
		<< L"[CREATE THREAD] ";

	rang_color_wstream(rang::fgB::cyan)
		<< L"0x" << std::hex << pi.dwThreadId;

	rang_color_wstream(rang::fg::yellow)
		<< GetAddressDescriptor(info.lpStartAddress, collection) << std::endl;

	rang_reset();
}

/// <summary>
/// Log process termination, accompanied by exit code.
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="info">A const reference to the <see cref="EXIT_PROCESS_DEBUG_INFO"/> struct instance with event information.</param>
/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
void PrintingDebuggerEventHandler::OnExitProcess(
	time_t time,
	const EXIT_PROCESS_DEBUG_INFO& info, 
	const PROCESS_INFORMATION& pi,
	const ModuleCollection& collection) {

	rang_timestamp();

	rang_color_wstream(rang::fg::red)
		<< L"[EXIT PROCESS] ";

	rang_color_wstream(rang::fgB::cyan)
		<< L"0x" << std::hex << pi.dwProcessId;

	rang_color_wstream(info.dwExitCode == 0 ? rang::fgB::green : rang::fgB::red)
		<< L", exit code 0x" << info.dwExitCode << std::dec << std::endl;

	rang_reset();
}

/// <summary>
/// Log thread termination, accompanied by exit code.
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="info">A const reference to the <see cref="EXIT_THREAD_DEBUG_INFO"/> struct instance with event information.</param>
/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
void PrintingDebuggerEventHandler::OnExitThread(
	time_t time,
	const EXIT_THREAD_DEBUG_INFO& info, 
	const PROCESS_INFORMATION& pi,
	const ModuleCollection& collection) {

	rang_timestamp();

	rang_color_wstream(rang::fg::red)
		<< L"[EXIT THREAD] ";

	rang_color_wstream(rang::fgB::cyan)
		<< L"0x" << std::hex << pi.dwThreadId;

	rang_color_wstream(info.dwExitCode == 0 ? rang::fgB::green : rang::fgB::red)
		<< L", exit code 0x" << info.dwExitCode << std::dec << std::endl;

	rang_reset();
}

/// <summary>
/// Log the loading of a DLL module.
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="info">A const reference to the <see cref="EXIT_THREAD_DEBUG_INFO"/> struct instance with event information.</param>
/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
/// <param name="path">The full unicode path to the DLL that was loaded.</param>
/// <param name="moduleIndex">The index in the module collection of this module, it might have been loaded before.</param>
/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
void PrintingDebuggerEventHandler::OnDllLoad(
	time_t time,
	const LOAD_DLL_DEBUG_INFO& info, 
	const PROCESS_INFORMATION& pi,
	const std::wstring& path, 
	int moduleIndex, 
	const ModuleCollection& collection) {

	rang_timestamp();

	rang_color_wstream(rang::fg::cyan)
		<< L"[DLL LOAD] ";

	rang_color_wstream(rang::fgB::cyan)
		<< L"0x" << std::hex << info.lpBaseOfDll << std::dec << ": ";

	rang_reset();

	m_WStream << path << std::endl;
}

/// <summary>
/// Log a debug string that was sent to the debugger by any module in the target process.
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="info">A const reference to the <see cref="OUTPUT_DEBUG_STRING_INFO"/> struct instance with event information.</param>
/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
/// <param name="string">The ANSI debug string.</param>
void PrintingDebuggerEventHandler::OnDebugString(
	time_t time, 
	const OUTPUT_DEBUG_STRING_INFO& info,
	const PROCESS_INFORMATION& pi, 
	const std::string& string) {

	rang_timestamp();

	rang_color_wstream(rang::fg::yellow)
		<< L"[DEBUGA] ";

	rang_color_wstream(rang::fgB::yellow)
		<< string;

	if (string.back() != '\n')
		m_WStream << std::endl;

	rang_reset();
}

/// <summary>
/// Log a unicode debug string that was sent to the debugger by any module in the target process.
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="info">A const reference to the <see cref="OUTPUT_DEBUG_STRING_INFO"/> struct instance with event information.</param>
/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
/// <param name="string">The Unicode debug string.</param>
void PrintingDebuggerEventHandler::OnDebugStringW(
	time_t time, 
	const OUTPUT_DEBUG_STRING_INFO& info,
	const PROCESS_INFORMATION& pi, 
	const std::wstring& string) {

	rang_timestamp();

	rang_color_wstream(rang::fg::yellow)
		<< L"[DEBUGW] ";

	rang_color_wstream(rang::fgB::yellow)
		<< string;

	if (string.back() != L'\n')
		m_WStream << std::endl;

	rang_reset();
}

/// <summary>
/// Log a RIP error event.
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="info">A const reference to the <see cref="RIP_INFO"/> struct instance with event information.</param>
/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
/// <param name="errorMessage">A string describing the <see cref="RIP_INFO::dwError"/> member, or an empty string if no description is available.</param>
void PrintingDebuggerEventHandler::OnRip(
	time_t time,
	const RIP_INFO& info,
	const PROCESS_INFORMATION& pi,
	const std::wstring& errorMessage) {

	rang_timestamp();

	rang_color_wstream(rang::fgB::red)
		<< L"[RIP] ";

	switch (info.dwType) {
		case SLE_ERROR:
			rang_color_wstream(rang::fg::red)
				<< L"(SLE_ERROR, program fail) ";
			break;

		case SLE_MINORERROR:
			rang_color_wstream(rang::fg::yellow)
				<< L"(SLE_MINORERROR, might fail) ";
			break;

		case SLE_WARNING:
			rang_color_wstream(rang::fg::green)
				<< L"(SLE_WARNING, will not fail) ";
			break;
	}

	rang_reset();
	if (!errorMessage.empty()) {
		m_WStream << errorMessage;
		if (errorMessage.back() != L'\n')
			m_WStream << std::endl;
	} else {
		m_WStream << std::endl;
	}
}

/// <summary>
/// Log the unloading of a DLL module.
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
void PrintingDebuggerEventHandler::OnDllUnload(
	time_t time,
	const UNLOAD_DLL_DEBUG_INFO& info, 
	const PROCESS_INFORMATION& pi,
	const std::wstring& path, 
	int moduleIndex, 
	const ModuleCollection& collection) {

	rang_timestamp();

	rang_color_wstream(rang::fg::red)
		<< L"[DLL UNLOAD] ";

	rang_color_wstream(rang::fgB::cyan)
		<< L"0x" << std::hex << info.lpBaseOfDll << std::dec << ": ";

	rang_reset();

	m_WStream << path << std::endl;
}

/// <summary>
/// This event is benine in this handler, the logging of DLL modules happens in events, not at the end.
/// The file is automatically closed when the shared pointer reference count to this handler reaches 0.
/// </summary>
/// <param name="time">The time of the event.</param>
/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
void PrintingDebuggerEventHandler::OnModuleCollectionComplete(
	time_t time, 
	const ModuleCollection& collection) {


}

/// <summary>
/// Retrieve a textual representation of an address. This method will try to find the module that 
/// contains this address and format it as "@ (Module/Path.dll+0xOFFSET)". If it cannot find the 
/// module that this address belongs to, it will format it simply as "@ 0xADDRESS".
/// </summary>
/// <param name="address">The address to format.</param>
/// <param name="collection">The module collection, in order to attempt to resolve the address to a module.</param>
/// <returns>A string representation of the address.</returns>
std::wstring PrintingDebuggerEventHandler::GetAddressDescriptor(const void* address, const ModuleCollection& collection) const {
	std::wstringstream as;

	auto module = collection.GetModuleAtAddress(address);
	if (module != nullptr) {
		as << L" @ " << module->Path << "+0x" << std::hex << (uint64_t)((uint8_t*)address - reinterpret_cast<size_t>(module->Base)) << std::dec;
	} else {
		as << L" @ 0x" << std::hex << address << std::dec;
	}

	return as.str();
}

/// <summary>
/// Print a full stack trace, including resolved addresses and optionally a thread CPU context.
/// </summary>
/// <param name="pi">A const reference to a <see cref="PROCESS_INFORMATION"/> instance of the context of the event.</param>
/// <param name="context">A shared pointer to the thread context at the time of the event.</param>
/// <param name="trace">A shared pointer to the stack trace, starting from the program counter of an exception or breakpoint.</param>
/// <param name="collection">A const reference to a <see cref="::Hindsight::Debugger::ModuleCollection"/> instance.</param>
void PrintingDebuggerEventHandler::PrintStackTrace(
	const PROCESS_INFORMATION& pi,
	std::shared_ptr<const DebugContext> context,
	std::shared_ptr<const DebugStackTrace> trace,
	const ModuleCollection& collection) const {

	if (trace->size() == 0) {
		rang_color_wstream(rang::fgB::red)
			<< L"no stack trace available" << std::endl;
		rang_reset();

		return;
	}

	rang_color_wstream(rang::fgB::magenta)
		<< L"[STACK]" << std::endl;

	size_t frameIndex = 0;
	for (const auto& frame : trace->list()) {
		if (frame.Recursion) {

			rang_color_wstream(rang::fgB::yellow)
				<< L"\t... recursion " << frame.RecursionCount << L" frames ..." << std::endl;
			rang_reset();

			frameIndex += frame.RecursionCount;
			continue;
		}

		std::wstring start(L"\t#" + std::to_wstring(frameIndex) + L": ");

		rang_color_wstream(rang::fg::cyan)
			<< start;

		rang_color_wstream(rang::fgB::cyan)
			<< (frame.Name.empty() ? std::string("<unknown>") : frame.Name);

		rang_color_wstream(rang::fg::yellow)
			<< GetAddressDescriptor(frame.Address, collection) << std::endl;

		if (!frame.Instructions.empty()) {
			for (const auto& instruction : frame.Instructions) {
				m_WStream << L"\t" << std::wstring(start.size() - 1, L' ');

				rang_color_wstream(rang::fgB::yellow)
					<< std::hex << std::setw(instruction.Is64BitAddress ? 16 : 8) << std::setfill(L'0') << instruction.Offset;
				
				rang_color_wstream(rang::fg::yellow)
					<< L" " << L"(" << std::dec << std::setw(2) << std::setfill(L'0') << instruction.Size << L")";

				rang_color_wstream(rang::fg::green)
					<< L" " << String::PadRight(instruction.InstructionHex, 24, ' ');

				rang_color_wstream(rang::fgB::green)
					<< L" " << instruction.InstructionMnemonic;

				if (!instruction.Operands.empty()) {
					rang_color_wstream(rang::fgB::magenta)
						<< L" " << instruction.Operands;
				}

				rang_reset();

				m_WStream << std::endl;
			}	
		}

		if (!frame.File.empty()) {
			m_WStream << L"\t" << std::wstring(start.size() - 1, L' ');

			rang_color_wstream(rang::fg::green) 
				<< frame.File << L": ";
			rang_color_wstream(rang::fgB::green)
				<< L"line " << frame.Line << std::endl; 
		}

		rang_reset();

		++frameIndex;
	}

	RestoreFlags();
}

/// <summary>
/// Print a thread CPU context, depending on what process is being debugged a 64-bit or a 32-bit context will be printed.
/// </summary>
/// <param name="context">A shared pointer to the thread context at the time of the event.</param>
void PrintingDebuggerEventHandler::PrintContext(std::shared_ptr<const DebugContext> context) const {
	rang_color_wstream(rang::fgB::magenta)
		<< L"[CPUCTX]" << std::endl;

	std::vector<std::pair<std::string, uint64_t>> registers;

#ifdef _WIN64
	auto is64 = context->Is64();
	if (is64) {
		const auto& ctx = context->Get64();

		registers = {
			{ "RIP", ctx.Rip },
			{ "RSP", ctx.Rsp },
			{ "RBP", ctx.Rbp },
			{ "RAX", ctx.Rax },
			{ "RBX", ctx.Rbx },
			{ "RCX", ctx.Rcx },
			{ "RDX", ctx.Rdx },
			{ "RSI", ctx.Rsi },
			{ "RDI", ctx.Rdi },
			{ "R8",  ctx.R8 },
			{ "R9",  ctx.R9 },
			{ "R10", ctx.R10 },
			{ "R11", ctx.R11 },
			{ "R12", ctx.R12 },
			{ "R13", ctx.R13 },
			{ "R14", ctx.R14 },
			{ "R15", ctx.R15 }
		};
	} else {
#else 
	auto is64 = false;
#endif 
		const auto& ctx = context->Get86();

		registers = {
			{ "EIP", ctx.Eip },
			{ "ESP", ctx.Esp },
			{ "EBP", ctx.Ebp },
			{ "EAX", ctx.Eax },
			{ "EBX", ctx.Ebx },
			{ "ECX", ctx.Ecx },
			{ "EDX", ctx.Edx },
			{ "ESI", ctx.Esi },
			{ "EDI", ctx.Edi }
		};
#ifdef _WIN64
	}
#endif 

	m_WStream << "\t";
	for (size_t current = 0, in_line = 0, count = registers.size(); current < count; ++current) {
		rang_color_wstream(rang::fg::green)
			<< String::PadLeft(registers[current].first, 3, ' ') << L" = ";
		rang_color_wstream(rang::fgB::green)
			<< std::hex << std::setw(is64 ? 16 : 8) << std::setfill(L'0') << registers[current].second << std::dec << std::setw(0);
		rang_reset();

		if (in_line == 2 || current == count - 1) {
			m_WStream << std::endl;
			in_line = 0;

			if (current != count - 1)
				m_WStream << "\t";
		} else {
			m_WStream << "  ";
			++in_line;
		}
	}

	RestoreFlags();
	m_WStream << std::endl;
}

/// <summary>
/// Print a single RTTI class entity.
/// </summary>
/// <param name="classSignature">The class signature.</param>
/// <param name="extends">True when this class extends a following class.</param>
void PrintingDebuggerEventHandler::PrintClass(const std::string& classSignature, bool extends) const {
	m_Stream << "\t";
	for (size_t i = 0, l = classSignature.size(); i < l;) {
		if (classSignature.substr(i, 6) == "class ") { // '[class ]a::b::c<char>::d'
			rang_color(rang::fg::cyan) << "class ";
			i += 6;
		} else if (classSignature.substr(i, 7) == "struct ") { // '[struct ]a::b::c<char>::d'
			rang_color(rang::fg::cyan) << "struct ";
			i += 7;
		} else if (classSignature.substr(i, 2) == "::") { // 'struct a[::]b[::]c<char>[::]d'
			rang_color(rang::fgB::gray) << "::";
			i += 2;
		} else if (classSignature[i] == '<' || classSignature[i] == '>' || classSignature[i] == ',' || classSignature[i] == '.') { // 'struct a::b::c[<]char[,] byte[>]::d'
			rang_color(rang::fgB::red) << classSignature[i];
			if (classSignature[i] == ',') m_Stream << " ";
			i += 1; 
		} else {
			rang_color(rang::fgB::cyan) << classSignature[i];
			i += 1;
		}
	}

	rang_color(rang::fg::cyan) << (extends ? " extends: " : ".") << "\n";
}

/// <summary>
/// Print the exception run-time type information.
/// </summary>
/// <param name="rtti">The RTTI instance.</param>
void PrintingDebuggerEventHandler::PrintRtti(std::shared_ptr<const CxxExceptions::ExceptionRunTimeTypeInformation> rtti) const {
	rang_color_wstream(rang::fgB::magenta)
		<< L"[RTTI]\n";

	// Print each catchable type.
	const auto& names = rtti->exception_type_names();
	for (size_t i = 0, limit = names.size(), last = limit - 1; i < limit; ++i) 
		PrintClass(names[i], i != last);

	// Print the module path of the ThrowInfo* source.
	if (rtti->exception_module_path().has_value()) {
		rang_color(rang::fg::yellow)
			<< "\tthrow info source(): ";
		rang_color_wstream(rang::fgB::yellow)
			<< rtti->exception_module_path().value();
		m_Stream << "\n";
	}

	// And the exception message, if we managed to find it.
	if (rtti->exception_message().has_value()) {
		rang_color(rang::fg::yellow)
			<< "\twhat(): ";
		rang_color(rang::fgB::yellow)
			<< rtti->exception_message().value();
		m_Stream << "\n";
	}
}