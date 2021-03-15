#pragma once

#ifndef printing_debugger_event_handler_h
#define printing_debugger_event_handler_h
	#include "IDebuggerEventHandler.hpp"
	#include "Process.hpp"

	#include <iostream>
	#include <fstream>

	namespace Hindsight {
		namespace Debugger {
			namespace EventHandler {
				/// <summary>
				/// An implementation of <see cref="::Hindsight::Debugger::EventHandler::IDebuggerEventHandler"/> that formats 
				/// debugger events and prints them to an output stream. This stream can be just std::cout, or a file stream.
				/// </summary>
				class PrintingDebuggerEventHandler : public IDebuggerEventHandler {
					public:
						using ostream   = std::ostream;
						using wostream  = std::wostream;
						using wofstream = std::wofstream;

					private:
						bool m_Colorize;						/* Should this handler use colorization in the output? (stdout only) */
						bool m_Timestamps;						/* Should this handler print timestamps of events? */
						bool m_PrintContext;					/* Should this handler print the thread contexts? */
						ostream&		m_Stream;				/* The ansi (char) stream, used for rang */
						wostream&		m_WStream;				/* The unicode (wchar_t) stream, used for output */
						wofstream		m_FStream;				/* When outputting to file, a wchar_t output file stream is required */

						std::ios_base::fmtflags	m_StreamFlags;	/* The preserved ansi stream flags, so that they can be restored */
						std::ios_base::fmtflags m_WStreamFlags;	/* The preserved unicode stream flags, so that they can be restored */

						/// <summary>
						/// Restore the flags of the output streams in this instance.
						/// </summary>
						inline void RestoreFlags() const;

					public:
						/// <summary>
						/// Construct a new PrintingDebuggerEventHandler for outputs streams with colorization support.
						/// </summary>
						/// <param name="stream">The output stream.</param>
						/// <param name="wstream">The unicode output stream.</param>
						/// <param name="colorize">When set to true, this handler will attempt colorizing the output.</param>
						/// <param name="times">When set to true, timestamps will be printed before each event indication.</param>
						/// <param name="printContext">When set to true, thread contexts will be printed before stack traces.</param>
						PrintingDebuggerEventHandler(ostream& stream, wostream& wstream, bool colorize, bool times, bool printContext);

						/// <summary>
						/// Construct a new PrintingDebuggerEventHandler for std::cout and std::wcout with colorization support. 
						/// </summary>
						/// <param name="colorize">When set to true, this handler will attempt colorizing the output.</param>
						/// <param name="times">When set to true, timestamps will be printed before each event indication.</param>
						/// <param name="printContext">When set to true, thread contexts will be printed before stack traces.</param>
						PrintingDebuggerEventHandler(bool colorize, bool times, bool printContext);

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
						PrintingDebuggerEventHandler(wostream& wstream, bool printContext);

						/// <summary>
						/// Construct a new PrintingDebuggerEventHandler for a wide-char file stream, which is will open as well. This
						/// is the constructor to use when outputting to file rather than to a console stream.
						/// </summary>
						/// <param name="file">The path to the unicode text file that will contain the full logging of the debug session.</param>
						/// <param name="printContext">When set to true, thread contexts will be printed before stack traces.</param>
						PrintingDebuggerEventHandler(const std::string& file, bool printContext);

						/// <summary>
						/// Handle the initialization event, prints the first known information.
						/// </summary>
						/// <param name="time">The time of the event.</param>
						/// <param name="p">The process being debugged.</param>
						void OnInitialization(
							time_t time,
							const std::shared_ptr<const Hindsight::Process::Process> p) override;

						/// <summary>
						/// Log a breakpoint and stacktrace of the breakpoint address, with optionally a thread context.
						/// </summary>
						/// <param name="time">The time of the event.</param>
						/// <param name="info">A const reference to the <see cref="EXCEPTION_DEBUG_INFO"/> struct instance with event information.</param>
						/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
						/// <param name="context">A shared pointer to a const <see cref="::Hindsight::Debugger::DebugContext"/> instance.</param>
						/// <param name="trace">A shared pointer to a const <see cref="::Hindsight::Debugger::DebugStackTrace"/> instance.</param>
						/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
						void OnBreakpointHit(
							time_t time,
							const EXCEPTION_DEBUG_INFO& info, 
							const PROCESS_INFORMATION& pi,
							std::shared_ptr<const DebugContext> context, 
							std::shared_ptr<const DebugStackTrace> trace,
							const ModuleCollection& collection) override;

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
						virtual void OnException(
							time_t time,
							const EXCEPTION_DEBUG_INFO& info,
							const PROCESS_INFORMATION& pi,
							bool firstChance,
							const std::wstring& name,
							std::shared_ptr<const DebugContext> context,
							std::shared_ptr<const DebugStackTrace> trace,
							const ModuleCollection& collection,
							std::shared_ptr<const CxxExceptions::ExceptionRunTimeTypeInformation> ertti) override;

						/// <summary>
						/// Log the process creation, with its PID and full path.
						/// </summary>
						/// <param name="time">The time of the event.</param>
						/// <param name="info">A const reference to the <see cref="CREATE_PROCESS_DEBUG_INFO"/> struct instance with event information.</param>
						/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
						/// <param name="path">The full path to the loaded module on file system.</param>
						/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
						void OnCreateProcess(
							time_t time,
							const CREATE_PROCESS_DEBUG_INFO& info,
							const PROCESS_INFORMATION& pi,
							const std::wstring& path,
							const ModuleCollection& collection) override;

						/// <summary>
						/// Log thread creation, with its TID, module and entrypoint offset in the module.
						/// </summary>
						/// <param name="time">The time of the event.</param>
						/// <param name="info">A const reference to the <see cref="CREATE_THREAD_DEBUG_INFO"/> struct instance with event information.</param>
						/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
						/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
						void OnCreateThread(
							time_t time,
							const CREATE_THREAD_DEBUG_INFO& info,
							const PROCESS_INFORMATION& pi,
							const ModuleCollection& collection) override;

						/// <summary>
						/// Log process termination, accompanied by exit code.
						/// </summary>
						/// <param name="time">The time of the event.</param>
						/// <param name="info">A const reference to the <see cref="EXIT_PROCESS_DEBUG_INFO"/> struct instance with event information.</param>
						/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
						/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
						void OnExitProcess(
							time_t time,
							const EXIT_PROCESS_DEBUG_INFO& info, 
							const PROCESS_INFORMATION& pi,
							const ModuleCollection& collection) override;

						/// <summary>
						/// Log thread termination, accompanied by exit code.
						/// </summary>
						/// <param name="time">The time of the event.</param>
						/// <param name="info">A const reference to the <see cref="EXIT_THREAD_DEBUG_INFO"/> struct instance with event information.</param>
						/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
						/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
						void OnExitThread(
							time_t time,
							const EXIT_THREAD_DEBUG_INFO& info,
							const PROCESS_INFORMATION& pi,
							const ModuleCollection& collection) override;

						/// <summary>
						/// Log the loading of a DLL module.
						/// </summary>
						/// <param name="time">The time of the event.</param>
						/// <param name="info">A const reference to the <see cref="EXIT_THREAD_DEBUG_INFO"/> struct instance with event information.</param>
						/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
						/// <param name="path">The full unicode path to the DLL that was loaded.</param>
						/// <param name="moduleIndex">The index in the module collection of this module, it might have been loaded before.</param>
						/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
						void OnDllLoad(
							time_t time,
							const LOAD_DLL_DEBUG_INFO& info,
							const PROCESS_INFORMATION& pi,
							const std::wstring& path,
							int moduleIndex,
							const ModuleCollection& collection) override;

						/// <summary>
						/// Log a debug string that was sent to the debugger by any module in the target process.
						/// </summary>
						/// <param name="time">The time of the event.</param>
						/// <param name="info">A const reference to the <see cref="OUTPUT_DEBUG_STRING_INFO"/> struct instance with event information.</param>
						/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
						/// <param name="string">The ANSI debug string.</param>
						void OnDebugString(
							time_t time,
							const OUTPUT_DEBUG_STRING_INFO& info,
							const PROCESS_INFORMATION& pi,
							const std::string& string) override;

						/// <summary>
						/// Log a unicode debug string that was sent to the debugger by any module in the target process.
						/// </summary>
						/// <param name="time">The time of the event.</param>
						/// <param name="info">A const reference to the <see cref="OUTPUT_DEBUG_STRING_INFO"/> struct instance with event information.</param>
						/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
						/// <param name="string">The Unicode debug string.</param>
						void OnDebugStringW(
							time_t time,
							const OUTPUT_DEBUG_STRING_INFO& info,
							const PROCESS_INFORMATION& pi,
							const std::wstring& string) override;

						/// <summary>
						/// Log a RIP error event.
						/// </summary>
						/// <param name="time">The time of the event.</param>
						/// <param name="info">A const reference to the <see cref="RIP_INFO"/> struct instance with event information.</param>
						/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
						/// <param name="errorMessage">A string describing the <see cref="RIP_INFO::dwError"/> member, or an empty string if no description is available.</param>
						void OnRip(
							time_t time,
							const RIP_INFO& info, 
							const PROCESS_INFORMATION& pi,
							const std::wstring& errorMessage) override;

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
						void OnDllUnload(
							time_t time,
							const UNLOAD_DLL_DEBUG_INFO& info, 
							const PROCESS_INFORMATION& pi, 
							const std::wstring& path, 
							int moduleIndex, 
							const ModuleCollection& collection) override;

						/// <summary>
						/// This event is benine in this handler, the logging of DLL modules happens in events, not at the end.
						/// The file is automatically closed when the shared pointer reference count to this handler reaches 0.
						/// </summary>
						/// <param name="time">The time of the event.</param>
						/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
						void OnModuleCollectionComplete(
							time_t time, 
							const ModuleCollection& collection) override;

					private:
						/// <summary>
						/// Retrieve a textual representation of an address. This method will try to find the module that 
						/// contains this address and format it as "@ (Module/Path.dll+0xOFFSET)". If it cannot find the 
						/// module that this address belongs to, it will format it simply as "@ 0xADDRESS".
						/// </summary>
						/// <param name="address">The address to format.</param>
						/// <param name="collection">The module collection, in order to attempt to resolve the address to a module.</param>
						/// <returns>A string representation of the address.</returns>
						std::wstring GetAddressDescriptor(const void* address, const ModuleCollection& collection) const;

						/// <summary>
						/// Print a full stack trace, including resolved addresses and optionally a thread CPU context.
						/// </summary>
						/// <param name="pi">A const reference to a <see cref="PROCESS_INFORMATION"/> instance of the context of the event.</param>
						/// <param name="context">A shared pointer to the thread context at the time of the event.</param>
						/// <param name="trace">A shared pointer to the stack trace, starting from the program counter of an exception or breakpoint.</param>
						/// <param name="collection">A const reference to a <see cref="::Hindsight::Debugger::ModuleCollection"/> instance.</param>
						void PrintStackTrace(const PROCESS_INFORMATION& pi,
							std::shared_ptr<const DebugContext> context,
							std::shared_ptr<const DebugStackTrace> trace,
							const ModuleCollection& collection) const;

						/// <summary>
						/// Print a thread CPU context, depending on what process is being debugged a 64-bit or a 32-bit context will be printed.
						/// </summary>
						/// <param name="context">A shared pointer to the thread context at the time of the event.</param>
						void PrintContext(std::shared_ptr<const DebugContext> context) const;

						/// <summary>
						/// Print a single RTTI class entity.
						/// </summary>
						/// <param name="classSignature">The class signature.</param>
						/// <param name="extends">True when this class extends a following class.</param>
						void PrintClass(const std::string& classSignature, bool extends) const;

						/// <summary>
						/// Print the exception run-time type information.
						/// </summary>
						/// <param name="rtti">The RTTI instance.</param>
						void PrintRtti(std::shared_ptr<const CxxExceptions::ExceptionRunTimeTypeInformation> rtti) const;
				};
			}
		}
	}

#endif 