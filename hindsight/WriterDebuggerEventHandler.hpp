#pragma once

#ifndef writer_debugger_event_handler_h
#define writer_debugger_event_handler_h
	#include "IDebuggerEventHandler.hpp"
	#include "BinaryLogFile.hpp"
	#include <fstream>
	#include <type_traits>

	namespace Hindsight {
		namespace Debugger {
			namespace EventHandler {
				/// <summary>
				/// An implementation of <see cref="::Hindsight::Debugger::EventHandler::IDebuggerEventHandler"/> that writes 
				/// all relevant details of each debug event to a binary log file, which can later be converted to a textual 
				/// log file or be replayed to simulate the same order of events and produce output like a regular debug 
				/// session.
				/// </summary>
				class WriterDebuggerEventHandler : public IDebuggerEventHandler {
					private:
						std::ofstream m_Stream;							/* The binary output stream */
						Hindsight::BinaryLog::FileHeader m_Header;		/* The file header, the instance is kept to update the checksum at the end */

					public:
						/// <summary>
						/// Construct a new WriterDebuggerEventHandler, which will create and write to <paramref name="filepath"/>.
						/// </summary>
						/// <param name="filepath">The path to the file which will contain the logged events.</param>
						WriterDebuggerEventHandler(const std::string& filepath);

						/// <summary>
						/// Write the initial data to the binary log file, such as the file header. It will also write 
						/// the debugged process path, working directory (if available) and program parameters (if available).
						/// </summary>
						/// <param name="time">The time of the event.</param>
						/// <param name="p">The process being debugged.</param>
						void OnInitialization(
							time_t time,
							const std::shared_ptr<const Hindsight::Process::Process> pi) override;

						/// <summary>
						/// Write details about a breakpoint exception event, including a thread context and stack trace.
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
						/// <param name="ertti">A shared pointer to a const <see cref="::Hindsight::Debugger::CxxExceptions::ExceptionRtti"/> instance.</param>
						void OnException(
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
						/// Write the event that denotes the creation of a process.
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
						/// Write the event that denotes the creation of a thread.
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
						/// Write the EXIT_PROCESS event.
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
						/// Write the EXIT_THREAD event.
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
						/// Write the LOAD_DLL event, accompanied by information about the loaded module.
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
						/// Write an ANSI (or might be UTF-8) debug string sent by the debugged process.
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
						/// Write a unicode (wchar_t) debug string sent by the debugged process.
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
						/// Write a RIP error event.
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
						void OnDllUnload(
							time_t time,
							const UNLOAD_DLL_DEBUG_INFO& info,
							const PROCESS_INFORMATION& pi,
							const std::wstring& path,
							int moduleIndex,
							const ModuleCollection& collection) override;

						/// <summary>
						/// Finalize the binary logging, which will seek back to the header and overwrite it with the updated Crc32 checksum.
						/// </summary>
						/// <param name="time">The time of the event.</param>
						/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
						void OnModuleCollectionComplete(
							time_t time, 
							const ModuleCollection& collection) override;


					private:
						/// <summary>
						/// Write a class or struct instance to the output stream.
						/// </summary>
						/// <param name="s">A reference to the class or struct instance to write, of type <typeparamref name="T"/>.</param>
						/// <typeparam name="T">The type of value to write, which must be a class.</typeparam>
						template <typename T, std::enable_if_t<std::is_class<T>::value, int> = 0>
						void Write(T s);

						/// <summary>
						/// Write a <see cref="::std::wstring"/> to the output stream, optionally preceeded by its length.
						/// </summary>
						/// <param name="s">A reference to the string to write.</param>
						/// <param name="writeLength">When set to true, the length of the string will be written as a <see cref="uint32_t"/> before the string.</param>
						void Write(const std::wstring& s, bool writeLength = false);

						/// <summary>
						/// Write a <see cref="::std::string"/> to the output stream, optionally preceeded by its length.
						/// </summary>
						/// <param name="s">A reference to the string to write.</param>
						/// <param name="writeLength">When set to true, the length of the string will be written as a <see cref="uint32_t"/> before the string.</param>
						void Write(const std::string& s, bool writeLength = false);

						/// <summary>
						/// Write an arbitrarily sized block of data to the output stream and optionally update the internal checksum of
						/// all the written data.
						/// </summary>
						/// <param name="data">A pointer to <paramref name="size"/> bytes of memory to write.</param>
						/// <param name="size">The amount of bytes to write.</param>
						/// <param name="updateChecksum">When set to true, the internal checksum will be updated.</param>
						void Write(const char* data, size_t size, bool updateChecksum = true);

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
						/// <param name="ertti">Run-time type information about the exception.</param>
						/// <param name="isBreak">True when breakpoint, false when exception.</param>
						void Write(
							const EXCEPTION_DEBUG_INFO& info,
							const PROCESS_INFORMATION& pi,
							std::shared_ptr<const DebugContext> context,
							std::shared_ptr<const DebugStackTrace> trace,
							const ModuleCollection& collection,
							std::shared_ptr<const CxxExceptions::ExceptionRunTimeTypeInformation> ertti,
							bool isBreak);

						/// <summary>
						/// Write a thread context to the output stream, which will be either a <see cref="CONTEXT"/> struct or 
						/// a <see cref="WOW64_CONTEXT"/> struct depending on the debugged process. 
						/// </summary>
						/// <param name="context">A shared pointer to a <see cref="::Hindsight::Debugger::DebugContext"/> instance.</param>
						void Write(std::shared_ptr<const DebugContext> context);

						/// <summary>
						/// Write a stack trace to the output stream, starting from an exception address.
						/// </summary>
						/// <param name="trace">A shared pointer to a <see cref="::Hindsight::Debugger::DebugStackTrace"/> instance.</param>
						/// <param name="collection">A const reference to a <see cref="Hindsight::Debugger::ModuleCollection"/> instance containing information about loaded modules.</param>
						void Write(
							std::shared_ptr<const DebugStackTrace> trace,
							const ModuleCollection& collection);
				};

			}
		}
	}

#endif 