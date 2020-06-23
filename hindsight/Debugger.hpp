#pragma once

#ifndef debugger_h
#define debugger_h

	#include "State.hpp"
	#include "Process.hpp"
	#include "DebuggerExceptions.hpp"
	#include "ModuleCollection.hpp"
	#include "IDebuggerEventHandler.hpp"
	#include <Windows.h>
	#include <memory>
	#include <vector>
	#include <string>
	#include <map>

	#ifndef STATUS_WX86_BREAKPOINT
		/// <summary>
		/// The WOW64 breakpoint exception
		/// </summary>
		#define STATUS_WX86_BREAKPOINT 0x4000001f
	#endif 

	#ifndef STATUS_WX86_SINGLE_STEP
		/// <summary>
		/// The WOW64 single step exception
		/// </summary>
		#define STATUS_WX86_SINGLE_STEP 0x4000001e
	#endif 

	namespace Hindsight {
		namespace Debugger {
			/// <summary>
			/// A simple wrapper for the information required in postmortem debugging.
			/// </summary>
			class JitDebuggerInfo {
				public:
					JIT_DEBUG_INFO	JitInfo = { 0 };
					HANDLE			JitEvent = 0;
			};

			/// <summary>
			/// The Debugger class contains the functionality for real-time and postmortem debugging, with the focus
			/// on capturing and logging uncaught exceptions in production software. This debugger does not step through
			/// lines of code, but provides an insight (or hindsight) in where an issue might be caused.
			/// </summary>
			class Debugger {
				private:
					const Hindsight::State& m_State;					/* The hindsight program argument state. */
					std::shared_ptr<Process::Process> m_Process;		/* The debugged process. */
					std::shared_ptr<JitDebuggerInfo> m_Jit = nullptr;	/* A shared pointer to JIT debug info for the postmortem mode */

					// A collection of debugger event handlers.
					std::vector<std::shared_ptr<EventHandler::IDebuggerEventHandler>> m_Handlers;

					// The currently loaded modules in the debugged process.
					ModuleCollection m_LoadedModules;

				public:
					/// <summary>
					/// Construct a new Debugger instance for real-time debugging.
					/// </summary>
					/// <param name="process">A shared pointer to a <see cref="Hindsight::Process::Process"/> instance containing information about the process to be debugged.</param>
					/// <param name="state">The hindsight program argument state.</param>
					Debugger(std::shared_ptr<Process::Process> process, const Hindsight::State& state);

					/// <summary>
					/// Construct a new Debugger instance for postmortem (JIT) debugging.
					/// </summary>
					/// <param name="process">A shared pointer to a <see cref="Hindsight::Process::Process"/> instance containing information about the process to be debugged.</param>
					/// <param name="state">The hindsight program argument state.</param>
					/// <param name="jitEvent">The event handle copied into the hindsight process, so that WER can be signaled to let the debugged process continue.</param>
					/// <param name="jit">An address in the debugged process address space pointing to a <see cref="JIT_DEBUG_INFO"/> instance.</param>
					Debugger(std::shared_ptr<Process::Process> process, const Hindsight::State& state, HANDLE jitEvent, void* jit);

					/// <summary>
					/// The Debugger destructor, which closes the process handles and detaches the debugger from it.
					/// </summary>
					~Debugger();

					/// <summary>
					/// Attach the debugger to the process.
					/// </summary>
					/// <param name="killOnDetach">When set to true, the debugged process will be killed when the debugger detaches.</param>
					/// <returns>When succesful, true is returned.</returns>
					bool Attach(bool killOnDetach = false);

					/// <summary>
					/// Detach the debugger from the process.
					/// </summary>
					/// <returns>When successful, true is returned.</returns>
					bool Detach();

					/// <summary>
					/// Add an implementation of <see cref="::Hindsight::Debugger::EventHandler::IDebuggerEventHandler"/> to the debugger.
					/// </summary>
					/// <param name="handler">The debug event handler.</param>
					void AddHandler(std::shared_ptr<EventHandler::IDebuggerEventHandler> handler);
				private:
					/// <summary>
					/// Enumerate all the loaded modules for a specific process and simulate the LOAD_DLL debug event to all the handlers,
					/// but also track the collection of loaded modules so that address resolution will work properly in those handlers.
					/// </summary>
					/// <param name="hProcess">The handle to the debugged process.</param>
					void EnumerateProcessModules(HANDLE hProcess);

					/// <summary>
					/// Emit the postmortem/JIT exception to a handler.
					/// </summary>
					/// <param name="handler">The handler to trigger.</param>
					/// <param name="info">A const reference to the <see cref="JIT_DEBUG_INFO"/> struct.</param>
					/// <param name="exception">A const reference to the normalized <see cref="EXCEPTION_DEBUG_INFO"/> struct.</param>
					/// <param name="context">A shared pointer to the thread context where the exception was raised.</param>
					/// <param name="trace">A shared pointer to the stack trace starting from the program counter address where the exception originated.</param>
					void EmitJitException(std::shared_ptr<EventHandler::IDebuggerEventHandler> handler, const JIT_DEBUG_INFO& info, const EXCEPTION_DEBUG_INFO& exception, std::shared_ptr<DebugContext> context, std::shared_ptr<DebugStackTrace> trace);

					/// <summary>
					/// Wait for the next debug event and emit it to all the handlers.
					/// </summary>
					/// <returns>When the debug loop should stop, false is returned.</returns>
					bool Tick();

					/// <summary>
					/// Will break the output, until the user presses a key.
					/// </summary>
					void HandleBreakpointOptions();
				public:
					/// <summary>
					/// Start the debugger main loop.
					/// </summary>
					void Start();

					/// <summary>
					/// Names of common error codes that exceptions can have.
					/// </summary>
					static std::map<DWORD, std::wstring> ExceptionNames;
			};
		}
	}

#endif