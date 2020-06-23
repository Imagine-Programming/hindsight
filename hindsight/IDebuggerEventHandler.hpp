#pragma once

#ifndef debugger_event_handler_h
#define debugger_event_handler_h
	#include <Windows.h>
	#include <DbgHelp.h>

	#include <string>
	#include <memory>
	#include <vector>

	#include "Process.hpp"
	#include "DebugContext.hpp"
	#include "DebugStackTrace.hpp"
	#include "ModuleCollection.hpp"

	namespace Hindsight {
		namespace Debugger {
			namespace EventHandler {
				/// <summary>
				/// The abstract class, used as interface, for all debugger event handlers. Each event handler should 
				/// implement each virtual abstract method in this definition.
				/// </summary>
				class IDebuggerEventHandler {
					public:
						/// <summary>
						/// Virtual dtor
						/// </summary>
						virtual ~IDebuggerEventHandler() {}

						/// <summary>
						/// The method that will handle the initialization event.
						/// </summary>
						/// <param name="time">The time of the event.</param>
						/// <param name="p">The process being debugged.</param>
						virtual void OnInitialization(
							time_t time,
							const std::shared_ptr<const Hindsight::Process::Process> p) = 0;

						/// <summary>
						/// The method that will handle the breakpoint exception event. This method will receive the full 
						/// stack trace and context of the exception address.
						/// </summary>
						/// <param name="time">The time of the event.</param>
						/// <param name="info">A const reference to the <see cref="EXCEPTION_DEBUG_INFO"/> struct instance with event information.</param>
						/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
						/// <param name="context">A shared pointer to a const <see cref="::Hindsight::Debugger::DebugContext"/> instance.</param>
						/// <param name="trace">A shared pointer to a const <see cref="::Hindsight::Debugger::DebugStackTrace"/> instance.</param>
						/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
						virtual void OnBreakpointHit(
							time_t time,
							const EXCEPTION_DEBUG_INFO& info, 
							const PROCESS_INFORMATION& pi,
							std::shared_ptr<const DebugContext> context, 
							std::shared_ptr<const DebugStackTrace> trace,
							const ModuleCollection& collection) = 0;

						/// <summary>
						/// The method that will handle the exception event. This method will receive the full 
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
						virtual void OnException(
							time_t time,
							const EXCEPTION_DEBUG_INFO& info, 
							const PROCESS_INFORMATION& pi,
							bool firstChance, 
							const std::wstring& name,
							std::shared_ptr<const DebugContext> context, 
							std::shared_ptr<const DebugStackTrace> trace,
							const ModuleCollection& collection) = 0;

						/// <summary>
						/// The method that will handle the CREATE_PROCESS event. 
						/// </summary>
						/// <param name="time">The time of the event.</param>
						/// <param name="info">A const reference to the <see cref="CREATE_PROCESS_DEBUG_INFO"/> struct instance with event information.</param>
						/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
						/// <param name="path">The full path to the loaded module on file system.</param>
						/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
						virtual void OnCreateProcess(
							time_t time,
							const CREATE_PROCESS_DEBUG_INFO& info,
							const PROCESS_INFORMATION& pi,
							const std::wstring& path,
							const ModuleCollection& collection) = 0;

						/// <summary>
						/// The method that will handle the CREATE_THREAD event. 
						/// </summary>
						/// <param name="time">The time of the event.</param>
						/// <param name="info">A const reference to the <see cref="CREATE_THREAD_DEBUG_INFO"/> struct instance with event information.</param>
						/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
						/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
						virtual void OnCreateThread(
							time_t time,
							const CREATE_THREAD_DEBUG_INFO& info,
							const PROCESS_INFORMATION& pi,
							const ModuleCollection& collection) = 0;

						/// <summary>
						/// The method that will handle the EXIT_PROCESS event. 
						/// </summary>
						/// <param name="time">The time of the event.</param>
						/// <param name="info">A const reference to the <see cref="EXIT_PROCESS_DEBUG_INFO"/> struct instance with event information.</param>
						/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
						/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
						virtual void OnExitProcess(
							time_t time,
							const EXIT_PROCESS_DEBUG_INFO& info,
							const PROCESS_INFORMATION& pi,
							const ModuleCollection& collection) = 0;

						/// <summary>
						/// The method that will handle the EXIT_THREAD event. 
						/// </summary>
						/// <param name="time">The time of the event.</param>
						/// <param name="info">A const reference to the <see cref="EXIT_THREAD_DEBUG_INFO"/> struct instance with event information.</param>
						/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
						/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
						virtual void OnExitThread(
							time_t time,
							const EXIT_THREAD_DEBUG_INFO& info, 
							const PROCESS_INFORMATION& pi,
							const ModuleCollection& collection) = 0;

						/// <summary>
						/// The method that will handle the LOAD_DLL event. 
						/// </summary>
						/// <param name="time">The time of the event.</param>
						/// <param name="info">A const reference to the <see cref="EXIT_THREAD_DEBUG_INFO"/> struct instance with event information.</param>
						/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
						/// <param name="path">The full unicode path to the DLL that was loaded.</param>
						/// <param name="moduleIndex">The index in the module collection of this module, it might have been loaded before.</param>
						/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
						virtual void OnDllLoad(
							time_t time,
							const LOAD_DLL_DEBUG_INFO& info,
							const PROCESS_INFORMATION& pi,
							const std::wstring& path,
							int moduleIndex,
							const ModuleCollection& collection) = 0;

						/// <summary>
						/// The method that will handle the OUTPUT_DEBUG_STRING event (for ANSI strings). 
						/// </summary>
						/// <param name="time">The time of the event.</param>
						/// <param name="info">A const reference to the <see cref="OUTPUT_DEBUG_STRING_INFO"/> struct instance with event information.</param>
						/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
						/// <param name="string">The ANSI debug string.</param>
						virtual void OnDebugString(
							time_t time,
							const OUTPUT_DEBUG_STRING_INFO& info,
							const PROCESS_INFORMATION& pi,
							const std::string& string) = 0;

						/// <summary>
						/// The method that will handle the OUTPUT_DEBUG_STRING event (for Unicode strings). 
						/// </summary>
						/// <param name="time">The time of the event.</param>
						/// <param name="info">A const reference to the <see cref="OUTPUT_DEBUG_STRING_INFO"/> struct instance with event information.</param>
						/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
						/// <param name="string">The Unicode debug string.</param>
						virtual void OnDebugStringW(
							time_t time,
							const OUTPUT_DEBUG_STRING_INFO& info,
							const PROCESS_INFORMATION& pi,
							const std::wstring& string) = 0;

						/// <summary>
						/// The method that will handle the RIP event.
						/// </summary>
						/// <param name="time">The time of the event.</param>
						/// <param name="info">A const reference to the <see cref="RIP_INFO"/> struct instance with event information.</param>
						/// <param name="pi">A const reference to the <see cref="PROCESS_INFORMATION"/> struct of the process and thread triggering the event.</param>
						/// <param name="errorMessage">A string describing the <see cref="RIP_INFO::dwError"/> member, or an empty string if no description is available.</param>
						virtual void OnRip(
							time_t time,
							const RIP_INFO& info, 
							const PROCESS_INFORMATION& pi, 
							const std::wstring& errorMessage) = 0;

						/// <summary>
						/// The method that will handle the UNLOAD_DLL event.
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
						virtual void OnDllUnload(
							time_t time,
							const UNLOAD_DLL_DEBUG_INFO& info, 
							const PROCESS_INFORMATION& pi, 
							const std::wstring& path, 
							int moduleIndex, 
							const ModuleCollection& collection) = 0;

						/// <summary>
						/// The method that will handle the finalization of the debugger. At this point, the debugging has 
						/// been completed and the module collection is in its final state. It will contain a list of all 
						/// modules that have been loaded during the lifetime of the debugger, so it can be used to log or 
						/// write a list of this collection. 
						/// 
						/// This is also a nice moment to finalize any i/o operations in a handler, instead of in a destructor.
						/// </summary>
						/// <param name="time">The time of the event.</param>
						/// <param name="collection">A const reference to the <see cref="::Hindsight::Debugger::ModuleCollection"/> of currently loaded modules at the time of the event.</param>
						virtual void OnModuleCollectionComplete(
							time_t time, 
							const ModuleCollection& collection) = 0;
				};
			}
		}
	}

#endif 