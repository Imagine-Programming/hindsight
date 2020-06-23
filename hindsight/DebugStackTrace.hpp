#pragma once

#ifndef debugger_stack_trace_h
#define debugger_stack_trace_h
	#include <Windows.h>
	#include <DbgHelp.h>

	#include "DebugContext.hpp"
	#include "ModuleCollection.hpp"
	#include "BinaryLogFile.hpp"

	#include <memory>
	#include <vector>

	namespace Hindsight {
		namespace Debugger {
			/// <summary>
			/// Describes one decoded instruction.
			/// </summary>
			struct DebugStackTraceInstruction {
				bool		Is64BitAddress;			/* True when the addressing mode is 64-bit. */
				size_t		Offset;					/* The address of this instruction. */
				size_t		Size;					/* The size of the packed instruction in bytes. */
				std::string	InstructionHex;			/* The hexadecimal representation of the instruction data. */
				std::string	InstructionMnemonic;	/* The instruction mnemonic. */
				std::string Operands;				/* The instruction operands as one string. */
			};

			/// <summary>
			/// Describes one stack trace entry.
			/// </summary>
			struct DebugStackTraceEntry {
				Module	Module;							/* The module that contains the address of this stack frame. */

				void*	ModuleBase = nullptr;			/* The module base address, if this address is nullptr then the module instance should be ignored. */
				void*	Address = nullptr;				/* The stack frame address. */
				void*	AbsoluteAddress = nullptr;		/* The stack frame absolute address. */
				void*	AbsoluteLineAddress = nullptr;	/* The stack frame absolute line address. */
				void*	LineAddress = nullptr;			/* The stack frame line address. */

				std::string  Name = "";					/* The symbol name at the stack frame address, if available. */
				std::wstring File = L"";				/* The source file of this symbol, if available. */
				uint32_t     Line = 0;					/* The line number in the source file, if available. */

				bool	Recursion = false;				/* True when this is a recursion cut frame, limiting recursion in the stack trace. */
				size_t	RecursionCount = 0;				/* Denotes the amount of frames that were skipped if this is a recursion cut frame. */

				std::vector<DebugStackTraceInstruction> Instructions; /* Decoded / disassembled instructions at the address of this frame. */
			};

			/// <summary>
			/// A stack trace built up using StackWalk64, starting from a specific thread context.
			/// This trace will contain, if available, symbol names, file paths of source files and line numbers.
			/// Aside from this, each entry in the stack trace can be resolved to the module the address comes from.
			/// </summary>
			class DebugStackTrace {
				private:
					std::shared_ptr<const DebugContext> m_Context;
					const ModuleCollection&				m_Modules;
					std::vector<DebugStackTraceEntry>	m_Trace;
					size_t								m_MaxRecursion;
					size_t								m_MaxInstruction;

				public:
					/// <summary>
					/// Construct a new DebugStackTrace based on a thread context, module collection and symbol search path.
					/// </summary>
					/// <param name="context">A shared pointer to an instance of <see cref="::Hindsight::Debugger::DebugContext"/>, this context specifies where the trace starts.</param>
					/// <param name="collection">A const reference to an instance of <see cref="::Hindsight::Debugger::ModuleCollection"/> containing all the loaded modules at the time of the trace.</param>
					/// <param name="searchPath">One or multiple (separated by ';') search paths where DbgHelp can find .PDB files.</param>
					/// <param name="max_recursion">The maximum number of recursive calls to show in a trace before cutting it.</param>
					/// <param name="max_instruction">The maximum number of instructions to disassemble at the program count addresses of each trace frame.</param>
					DebugStackTrace(
						std::shared_ptr<const DebugContext> context, 
						const ModuleCollection& collection, 
						std::string searchPath,
						size_t max_recursion = 10,
						size_t max_instruction = 0);

					/// <summary>
					/// Construct a new DebugStackTrace based on a thread context and module collection.
					/// </summary>
					/// <param name="context">A shared pointer to an instance of <see cref="::Hindsight::Debugger::DebugContext"/>, this context specifies where the trace starts.</param>
					/// <param name="collection">A const reference to an instance of <see cref="::Hindsight::Debugger::ModuleCollection"/> containing all the loaded modules at the time of the trace.</param>
					/// <param name="max_recursion">The maximum number of recursive calls to show in a trace before cutting it.</param>
					/// <param name="max_instruction">The maximum number of instructions to disassemble at the program count addresses of each trace frame.</param>
					DebugStackTrace(
						std::shared_ptr<const DebugContext> context, 
						const ModuleCollection& collection, 
						size_t max_recursion = 10,
						size_t max_instruction = 0);

					/// <summary>
					/// Construct a new DebugStackTrace based on a context, module collection and a concrete stack trace read from a binary log file.
					/// </summary>
					/// <param name="context">A shared pointer to an instance of <see cref="::Hindsight::Debugger::DebugContext"/>, this context specifies where the trace starts.</param>
					/// <param name="collection">A const reference to an instance of <see cref="::Hindsight::Debugger::ModuleCollection"/> containing all the loaded modules at the time of the trace.</param>
					/// <param name="trace">A const reference to an instance of <see cref="::Hindsight::BinaryLog::StackTraceConcrete"/> containing the full stack trace that needs to be converted.</param>
					DebugStackTrace(
						std::shared_ptr<const DebugContext> context,
						const ModuleCollection& collection,
						const Hindsight::BinaryLog::StackTraceConcrete& trace);

					/// <summary>
					/// Count the number of frames in this stack trace.
					/// </summary>
					/// <returns>The number of stack trace frames.</returns>
					size_t size() const;

					/// <summary>
					/// Get a const reference to the vector containing all instances of <see cref="::Hindsight::Debugger::DebugStackTraceEntry"/> that make up the frames in this trace.
					/// </summary>
					/// <returns>A const reference to a vector containing the stack trace entries.</returns>
					const std::vector<DebugStackTraceEntry>& list() const noexcept;

					/// <summary>
					/// Get the maximum number of recursive frames allowed before the trace of that recursion should be cut.
					/// </summary>
					/// <returns>The maximum number of recursive frames before the trace is cut at that point.</returns>
					const size_t& GetMaxRecursion() const;

					/// <summary>
					/// Get the maximum number of instructions that are to be disassembled in each frame.
					/// </summary>
					/// <returns>The maximum number of disassembled instructions per frame.</returns>
					const size_t& GetMaxInstructions() const;
				private:
					/// <summary>
					/// Walk the stack.
					/// </summary>
					void Walk();

					/// <summary>
					/// Disassemble the instructions at the PC address of a certain stack frame.
					/// </summary>
					/// <param name="frame">A const reference to the <see cref="STACKFRAME64"/> instance with information about the address to disassemble.</param>
					/// <param name="symbol">A const reference to the <see cref="SYMBOL_INFO"/> instance with information about the symbol at the address.</param>
					/// <param name="entry">A reference to the <see cref="::Hindsight::Debugger::DebugStackTraceEntry"/> to fill with information about the disassembled code.</param>
					void DisassembleFrame(const STACKFRAME64& frame, const SYMBOL_INFO& symbol, DebugStackTraceEntry& entry);

					/// <summary>
					/// Process a <see cref="StalkWalk64"/> frame for symbol names, source files and line numbers and add the results to the stack trace. 
					/// This method optionally disassembles instructions from the stack frame as well.
					/// </summary>
					/// <param name="frame">A const reference to a <see cref="STACKFRAME64"/> instance containing address information about the frame.</param>
					void AddFrame(const STACKFRAME64& frame);

					/// <summary>
					/// Add a recursion entry to the stack trace, which will add an indicator of the recursion
					/// 	with the number of frames that are skippedand the last frame.This would effectively
					/// 	allow you to generate a print out in the form :
					/// 
					/// #5: recursive call @ some address
					/// 	... recursion of X frames ...
					/// #5 + x: recursive call @ some address
					/// </summary>
					/// <param name="backlog">A const reference to a vector of <see cref="STACKFRAME64"/> instances that describes the backlog.</param>
					void AddRecursion(const std::vector<STACKFRAME64>& backlog);
			};

		}
	}

#endif