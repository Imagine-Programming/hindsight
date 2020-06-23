#include "DebugStackTrace.hpp"
#include <Windows.h>
#include <DbgHelp.h>
#include <Psapi.h>
#include <distorm.h>

using namespace Hindsight::Debugger;

/// <summary>
/// Construct a new DebugStackTrace based on a thread context, module collection and symbol search path.
/// </summary>
/// <param name="context">A shared pointer to an instance of <see cref="::Hindsight::Debugger::DebugContext"/>, this context specifies where the trace starts.</param>
/// <param name="collection">A const reference to an instance of <see cref="::Hindsight::Debugger::ModuleCollection"/> containing all the loaded modules at the time of the trace.</param>
/// <param name="searchPath">One or multiple (separated by ';') search paths where DbgHelp can find .PDB files.</param>
/// <param name="max_recursion">The maximum number of recursive calls to show in a trace before cutting it.</param>
/// <param name="max_instruction">The maximum number of instructions to disassemble at the program count addresses of each trace frame.</param>
DebugStackTrace::DebugStackTrace(
	std::shared_ptr<const DebugContext> context, 
	const ModuleCollection& collection, 
	std::string searchPath,
	size_t max_recursion,
	size_t max_instruction)
	: m_Context(context), m_Modules(collection), m_MaxRecursion(max_recursion), m_MaxInstruction(max_instruction) {

	// Set the options for DbgHelp to determine how exactly it should resolve symbols.
	SymSetOptions(
		SYMOPT_ALLOW_ABSOLUTE_SYMBOLS | 
		SYMOPT_DEFERRED_LOADS |
		SYMOPT_INCLUDE_32BIT_MODULES |
		SYMOPT_LOAD_LINES |
		SYMOPT_UNDNAME
	);

	// If a search path is provided, pass it on to SymInitialize
	const char* path = nullptr;
	if (!searchPath.empty())
		path = searchPath.c_str();

	// Initialize DbgHelp's symbol engine.
	SymInitialize(context->GetProcess(), path, true);
	Walk(); /* walk the stack */

	// Cleanup
	SymCleanup(context->GetProcess());
}

/// <summary>
/// Construct a new DebugStackTrace based on a thread context and module collection.
/// </summary>
/// <param name="context">A shared pointer to an instance of <see cref="::Hindsight::Debugger::DebugContext"/>, this context specifies where the trace starts.</param>
/// <param name="collection">A const reference to an instance of <see cref="::Hindsight::Debugger::ModuleCollection"/> containing all the loaded modules at the time of the trace.</param>
/// <param name="max_recursion">The maximum number of recursive calls to show in a trace before cutting it.</param>
/// <param name="max_instruction">The maximum number of instructions to disassemble at the program count addresses of each trace frame.</param>
DebugStackTrace::DebugStackTrace(
	std::shared_ptr<const DebugContext> context,
	const ModuleCollection& collection,
	size_t max_recursion,
	size_t max_instruction) 
	: DebugStackTrace(context, collection, "", max_recursion, max_instruction) {

}

/// <summary>
/// Construct a new DebugStackTrace based on a context, module collection and a concrete stack trace read from a binary log file.
/// </summary>
/// <param name="context">A shared pointer to an instance of <see cref="::Hindsight::Debugger::DebugContext"/>, this context specifies where the trace starts.</param>
/// <param name="collection">A const reference to an instance of <see cref="::Hindsight::Debugger::ModuleCollection"/> containing all the loaded modules at the time of the trace.</param>
/// <param name="trace">A const reference to an instance of <see cref="::Hindsight::BinaryLog::StackTraceConcrete"/> containing the full stack trace that needs to be converted.</param>
DebugStackTrace::DebugStackTrace(
	std::shared_ptr<const DebugContext> context,
	const ModuleCollection& collection,
	const Hindsight::BinaryLog::StackTraceConcrete& trace)
	: m_Context(context), m_Modules(collection), m_MaxRecursion(trace.MaxRecursion), m_MaxInstruction(trace.MaxInstructions) {

	// Iterate over every concrete StackTraceEntryConcrete instance
	for (const auto& entryConcrete : trace.Entries) {
		// Create a new stack trace entry and work with its reference
		auto& entry = m_Trace.emplace_back(); 

		// Try to resolve the module containing the entry address 
		const auto module = m_Modules.GetModuleAtAddress(reinterpret_cast<void*>(entryConcrete.Address));

		if (module != nullptr)
			entry.Module = *module;
		
		// Populate the stack trace entry
		entry.ModuleBase			= reinterpret_cast<void*>(entryConcrete.ModuleBase);
		entry.Address				= reinterpret_cast<void*>(entryConcrete.Address);
		entry.AbsoluteAddress		= reinterpret_cast<void*>(entryConcrete.AbsoluteAddress);
		entry.AbsoluteLineAddress	= reinterpret_cast<void*>(entryConcrete.AbsoluteLineAddress);
		entry.LineAddress			= reinterpret_cast<void*>(entryConcrete.LineAddress);
		entry.Name					= entryConcrete.Name;
		entry.File					= entryConcrete.Path;
		entry.Line					= static_cast<uint32_t>(entryConcrete.LineNumber);
		entry.Recursion				= entryConcrete.IsRecursion;
		entry.RecursionCount		= entryConcrete.RecursionCount;

		// Iterate over all disassembled instructions, if any.
		for (const auto& instructionConcrete : entryConcrete.Instructions) {
			// Create a new instruction entry in place and work with its reference.
			auto& instruction = entry.Instructions.emplace_back();

			// Populate
			instruction.Is64BitAddress		= instructionConcrete.Is64BitAddress;
			instruction.Offset				= instructionConcrete.Offset;
			instruction.Size				= instructionConcrete.Size;
			instruction.InstructionHex		= instructionConcrete.Hex;
			instruction.InstructionMnemonic = instructionConcrete.Mnemonic;
			instruction.Operands			= instructionConcrete.Operands;
		}
	}
}

/// <summary>
/// Count the number of frames in this stack trace.
/// </summary>
/// <returns>The number of stack trace frames.</returns>
size_t DebugStackTrace::size() const {
	return m_Trace.size();
}

/// <summary>
/// Get a const reference to the vector containing all instances of <see cref="::Hindsight::Debugger::DebugStackTraceEntry"/> that make up the frames in this trace.
/// </summary>
/// <returns>A const reference to a vector containing the stack trace entries.</returns>
const std::vector<DebugStackTraceEntry>& DebugStackTrace::list() const noexcept {
	return m_Trace;
}

/// <summary>
/// Get the maximum number of recursive frames allowed before the trace of that recursion should be cut.
/// </summary>
/// <returns>The maximum number of recursive frames before the trace is cut at that point.</returns>
const size_t& DebugStackTrace::GetMaxRecursion() const {
	return m_MaxRecursion;
}

/// <summary>
/// Get the maximum number of instructions that are to be disassembled in each frame.
/// </summary>
/// <returns>The maximum number of disassembled instructions per frame.</returns>
const size_t& DebugStackTrace::GetMaxInstructions() const {
	return m_MaxInstruction;
}

/// <summary>
/// Walk the stack.
/// </summary>
void DebugStackTrace::Walk() {
	STACKFRAME64 frame = { 0 };						/* The StackWalk64 frame result */
	std::vector<STACKFRAME64> recursion_backlog;	/* The recursion backlog, used for detecting the maximum recursion (direct recursion). */
	LPVOID lpContext;								/* A pointer to the thread context to fetch the trace for. */
	DWORD machineType;								/* The machine type of this trace. */

	union {
		CONTEXT			x64;
		WOW64_CONTEXT	x86;
	} context;
	
#ifdef _WIN64
	if (m_Context->Is64()) { 
		// Use the 64-bit native context and machine type for the trace
		context.x64 = m_Context->Get64();
		lpContext   = (LPVOID)&context.x64;
		machineType = IMAGE_FILE_MACHINE_AMD64;

		frame.AddrPC.Offset		= context.x64.Rip; /* program counter (instruction pointer) */
		frame.AddrPC.Mode		= AddrModeFlat;
		frame.AddrFrame.Offset	= context.x64.Rbp; /* base pointer of stack frame */
		frame.AddrFrame.Mode	= AddrModeFlat;
		frame.AddrStack.Offset	= context.x64.Rsp; /* stack pointer */
		frame.AddrStack.Mode	= AddrModeFlat;
	} else {
#endif 
		// Use the 32-bit WOW64 context and machine type for the trace
		context.x86 = m_Context->Get86();
		lpContext   = (LPVOID)&context.x86;
		machineType = IMAGE_FILE_MACHINE_I386;

		frame.AddrPC.Offset		= context.x86.Eip; /* program counter (instruction pointer) */
		frame.AddrPC.Mode		= AddrModeFlat;
		frame.AddrFrame.Offset	= context.x86.Ebp; /* base pointer of stack frame */
		frame.AddrFrame.Mode	= AddrModeFlat;
		frame.AddrStack.Offset	= context.x86.Esp; /* stack pointer */
		frame.AddrStack.Mode	= AddrModeFlat;
#ifdef _WIN64
	}
#endif 

	// Walk the stack, stop only when a next frame is not available.
	for (int frameNumber = 0;; ++frameNumber) {
		// get the next stack frame
		auto result = StackWalk64(
			machineType,
			m_Context->GetProcess(),
			m_Context->GetThread(),
			&frame,
			lpContext,
			nullptr,
			SymFunctionTableAccess64,
			SymGetModuleBase64,
			nullptr);

		if (!result)
			break;

		// if recursion is not allowed, limit the call stack
		if (m_MaxRecursion != SIZE_MAX) {
			// detect recursive calls, this must be a finite stack we can unwind. Stacks are not infinite.
			if (frame.AddrPC.Offset == frame.AddrReturn.Offset) {
				// backlog this frame, we might want to show them all
				recursion_backlog.push_back(frame);
				continue;
			} 
			
			// this wasn't a recursive call, but we do have a recursion backlog.
			else if (!recursion_backlog.empty()) {
				// if the backlog is larger than the max recursion setting, add a recursion frame and the last frame.
				if (recursion_backlog.size() >= m_MaxRecursion) {
					AddRecursion(recursion_backlog);
				} 
				
				// the backlog is not too large, just add all the frames from the backlog.
				else {
					for (const auto& backlog_frame : recursion_backlog) 
						AddFrame(backlog_frame);
				}

				// clear the backlog and continue
				recursion_backlog.clear();
				continue;
			}
		}

		// add the frame regularly
		AddFrame(frame);
	}
}

/// <summary>
/// Disassemble the instructions at the PC address of a certain stack frame.
/// </summary>
/// <param name="frame">A const reference to the <see cref="STACKFRAME64"/> instance with information about the address to disassemble.</param>
/// <param name="symbol">A const reference to the <see cref="SYMBOL_INFO"/> instance with information about the symbol at the address.</param>
/// <param name="entry">A reference to the <see cref="::Hindsight::Debugger::DebugStackTraceEntry"/> to fill with information about the disassembled code.</param>
void DebugStackTrace::DisassembleFrame(const STACKFRAME64& frame, const SYMBOL_INFO& symbol, DebugStackTraceEntry& entry) {
	const size_t maxInstructions = m_MaxInstruction;
	const size_t symbolSize		 = (symbol.Size != 0 ? symbol.Size : 30);
	SIZE_T read = 0;

	#pragma warning ( push )
	#pragma warning ( disable: 26812 ) /* unscoped enum complaint, third party code, ignore warning */
	_OffsetType					offset = frame.AddrPC.Offset;
	std::vector<_DecodedInst>	instructions(maxInstructions);
	std::vector<char>			code(symbolSize);
	uint32_t					instructionCount; 
	_DecodeType					dt = (m_Context->Is64() ? _DecodeType::Decode64Bits : _DecodeType::Decode32Bits);
	#pragma warning ( pop )

	if (!ReadProcessMemory(m_Context->GetProcess(), reinterpret_cast<LPCVOID>(frame.AddrPC.Offset), reinterpret_cast<LPVOID>(&code[0]), symbolSize, &read) && read == 0)
		return;

	// Disassemble, we're going to ignore the result as it does not indicate nothing was decoded.
	static_cast<void>(distorm_decode(
		offset, 
		reinterpret_cast<const unsigned char*>(&code[0]), 
		static_cast<int>(read),
		dt, 
		&instructions[0], 
		static_cast<unsigned int>(maxInstructions), 
		&instructionCount));

	// Process disassembled instructions
	for (uint32_t i = 0; i < instructionCount; i++) {
		// Create a new DebugStackTraceInstruction instance in place and work with the reference
		auto& instruction = entry.Instructions.emplace_back();

		instruction.Is64BitAddress		= (dt == _DecodeType::Decode64Bits);
		instruction.Offset				= instructions[i].offset;
		instruction.Size				= instructions[i].size;
		instruction.InstructionHex		= reinterpret_cast<char*>(instructions[i].instructionHex.p);
		instruction.InstructionMnemonic = reinterpret_cast<char*>(instructions[i].mnemonic.p);
		instruction.Operands			= reinterpret_cast<char*>(instructions[i].operands.p);
	}
}

/// <summary>
/// Process a <see cref="StalkWalk64"/> frame for symbol names, source files and line numbers and add the results to the stack trace. 
/// This method optionally disassembles instructions from the stack frame as well.
/// </summary>
/// <param name="frame">A const reference to a <see cref="STACKFRAME64"/> instance containing address information about the frame.</param>
void DebugStackTrace::AddFrame(const STACKFRAME64& frame) {
	DWORD64 dwSymbolDisplacement;
	DWORD   dwLineDisplacement;

	// Reserve enough bytes of memory to hold the SYMBOL_INFO struct and the symbol name
	uint8_t symbol[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};

	auto pSymbol = reinterpret_cast<PSYMBOL_INFO>(symbol);
	auto address = frame.AddrPC.Offset;
	auto& entry  = m_Trace.emplace_back(); /* create new stack trace entry and work with its reference */

	pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbol->MaxNameLen   = MAX_SYM_NAME;
	entry.Address         = reinterpret_cast<void*>(address);

	// Get symbol information (name, which module it came from, addresses)
	if (SymFromAddr(m_Context->GetProcess(), address, &dwSymbolDisplacement, pSymbol)) {
		// Try to find the loaded module this address belongs to.
		auto module = m_Modules.GetModuleAtAddress(reinterpret_cast<const void*>(pSymbol->Address)); 

		if (module != nullptr) {
			entry.Module = *module;

			if (!pSymbol->ModBase) {
				entry.ModuleBase = reinterpret_cast<void*>(module->Base);
			} else {
				entry.ModuleBase = reinterpret_cast<void*>(pSymbol->ModBase);
			}
		} else {
			entry.ModuleBase = reinterpret_cast<void*>(pSymbol->ModBase);
		}

		entry.AbsoluteAddress = reinterpret_cast<void*>(address + dwSymbolDisplacement);
		entry.Name			  = "";

		if (pSymbol->NameLen) /* convert the name to an std::string */
			entry.Name = std::string((const char*)&pSymbol->Name[0], pSymbol->NameLen);
	}

	if (m_MaxInstruction != 0) /* Disassemble the instructions at the address of this symbol */
		DisassembleFrame(frame, *pSymbol, entry);

	// Get the symbol line association, which is an approximation
	IMAGEHLP_LINEW64 line = { sizeof(IMAGEHLP_LINEW64), 0, 0, 0, 0 };
	if (SymGetLineFromAddrW64(m_Context->GetProcess(), frame.AddrPC.Offset, &dwLineDisplacement, &line)) {
		entry.AbsoluteLineAddress = reinterpret_cast<void*>(frame.AddrPC.Offset + dwLineDisplacement);
		entry.LineAddress		  = reinterpret_cast<void*>(line.Address);
		entry.File				  = line.FileName;
		entry.Line				  = line.LineNumber;
	}
}

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
void DebugStackTrace::AddRecursion(const std::vector<STACKFRAME64>& backlog) {
	auto& entry = m_Trace.emplace_back();
	entry.Recursion = true;
	entry.RecursionCount = backlog.size();
	AddFrame(backlog.back()); 
}