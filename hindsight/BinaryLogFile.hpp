#pragma once

#ifndef binary_log_file_h
#define binary_log_file_h
	#include "Version.hpp"
	#include <Windows.h>
	#include <vector>
	#include <string>
	/*
		File structure:
			- (HIND) FileHeader (always starts with this)
			  the file header has a Crc32 checksum field as well, all data of the log should be streamed to 
			  step through a crc32 checksum state. This way, the full data of the binary log can be verified 
			  to be intact (and to check if the module collection is written at the end, which is important for 
			  some event entries).
			- Frame collection
			  After the file header comes a collection of frames, each frame is a different type. One should first 
			  read the signature of 4 bytes to determine what kind of frame it is. Then the frame should be read 
			  accordingly.
			  - (EVNT) EventEntry or any of its derived classes
			    In this scenario, the EventId should be read from the EventEntry base class. One can then seek back 
				to the start of the struct and read the appropriate type (i.e. CreateProcessEventEntry)
			  - (MODS) ModuleList
			    A collection specifying the modules the process has loaded during its lifetime.
	*/
	namespace Hindsight {
		namespace BinaryLog {
			
			// All structs that are written to the binary output stream are packed.
			#pragma pack(push, 1)

			/// <summary>
			/// The HIND format file header, each hindsight debug file should start with this struct.
			/// </summary>
			struct FileHeader {
				char		Signature[4] = { 'H', 'I', 'N', 'D' };
				uint32_t	Version = hindsight_version_int;
				uint32_t	ProcessId;
				uint32_t	ThreadId;
				uint64_t	PathLength;
				uint64_t	WorkingDirectoryLength;
				uint64_t	Arguments;
				time_t		StartTime;
				uint32_t	Crc32; /* calculate afterwards, for sanity */
			};

			/// <summary>
			/// A normalized <see cref="PROCESS_INFORMATION"/>-like struct that uses constant sizes.
			/// </summary>
			struct EventEntryProcessInformation {
				uint64_t	hProcess;
				uint64_t	hThread;
				uint32_t	dwProcessId;
				uint32_t	dwThreadId;

				/// <summary>
				/// Default constructor for zero-initialization.
				/// </summary>
				EventEntryProcessInformation() = default;

				/// <summary>
				/// Construct a new EventEntryProcessInformation struct instance from a const reference to a <see cref="PROCESS_INFORMATION"/> instance.
				/// </summary>
				/// <param name="pi">The process information.</param>
				EventEntryProcessInformation(const PROCESS_INFORMATION& pi);

				/// <summary>
				/// Cast this struct back to a <see cref="PROCESS_INFORMATION"/> struct instance.
				/// </summary>
				/// <returns>A newly created <see cref="PROCESS_INFORMATION"/> struct.</returns>
				explicit operator PROCESS_INFORMATION() const;
			};

			/// <summary>
			/// Event entry base struct, each event entry inherits from this structure
			/// the binary log file should be streamed on a per entry basis, which means 
			/// that the signature should be read first. If the signature is EVNT, then 
			/// the EventId can be read. One can then seek back to the start of this struct
			/// and read the appropriate event entry structure.
			/// </summary>
			struct EventEntry {
				char						 Signature[4] = { 'E', 'V', 'N', 'T' };
				time_t						 Time		 = 0;
				uint32_t					 EventId		 = 0;
				uint64_t					 Size		 = 0;
				EventEntryProcessInformation ProcessInformation;

				/// <summary>
				/// Default constructor, generally used when reading an existing binary log file.
				/// </summary>
				EventEntry();

				/// <summary>
				/// Construct an EventEntry instance from a <see cref="PROCESS_INFORMATION"/> instance <paramref name="pi"/>, <paramref name="eventId"/> and <paramref name="size"/>.
				/// The size indicates the size of the concrete event entry.
				/// </summary>
				/// <param name="pi">A <see cref="PROCESS_INFORMATION"/> instance containing a handle to the process and thread of this event.</param>
				/// <param name="eventId">The event ID.</param>
				/// <param name="size">The size of the concrete event entry instance.</param>
				EventEntry(const PROCESS_INFORMATION& pi, uint32_t eventId, size_t size);
			};

			/// <summary>
			/// Exception event, followed by a CPU Context and stack trace
			/// </summary>
			struct ExceptionEventEntry : public EventEntry {
				uint64_t	EventAddress	= 0;
				uint64_t	EventOffset		= 0; /* offset of exception relative to the module */
				int64_t		ModuleIndex		= 0; /* the index of the module where offset it relative to */
				uint32_t	EventCode		= 0;
				uint8_t		Wow64			= 0;
				uint8_t		IsBreakpoint	= 0;
				uint8_t		IsFirstChance	= 0;

				/// <summary>
				/// Default constructor, generally used when reading an existing binary log file.
				/// </summary>
				ExceptionEventEntry(); 

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
				ExceptionEventEntry(const PROCESS_INFORMATION& pi, uint64_t addr, uint32_t code, bool iswow64, bool breakpoint, bool firstChance);
			};

			/// <summary>
			/// Create process event, followed by the path of the created processs
			/// </summary>
			struct CreateProcessEventEntry : public EventEntry {
				uint64_t	PathLength		= 0;
				uint64_t	ModuleBase		= 0;
				uint64_t	ModuleSize		= 0;

				/// <summary>
				/// Default constructor, generally used when reading an existing binary log file.
				/// </summary>
				CreateProcessEventEntry();

				/// <summary>
				/// Construct a CreateProcessEventEntry instance from parameters obtained through a debug event whilst debugging.
				/// </summary>
				/// <param name="pi">A <see cref="PROCESS_INFORMATION"/> instance containing a handle to the process and thread of this event.</param>
				/// <param name="path">A full path to the main module of the created process.</param>
				/// <param name="base">The base address for the main module of the created process.</param>
				/// <param name="size">The full size in memory of the main module of the created process.</param>
				CreateProcessEventEntry(const PROCESS_INFORMATION& pi, const std::wstring& path, uint64_t base, uint64_t size); 
			};

			/// <summary>
			/// Create thread event, containing information about the start address and which 
			/// module contained that start address.
			/// </summary>
			struct CreateThreadEventEntry : public EventEntry {
				uint64_t	EntryPointAddress = 0;
				int64_t		ModuleIndex = 0;
				uint64_t	EntryPointOffset = 0;

				/// <summary>
				/// Default constructor, generally used when reading an existing binary log file.
				/// </summary>
				CreateThreadEventEntry();

				/// <summary>
				/// Construct a CreateThreadEventEntry from parameters obtained through a debug event whilst debugging.
				/// </summary>
				/// <param name="pi">A <see cref="PROCESS_INFORMATION"/> instance containing a handle to the process and thread of this event.</param>
				/// <param name="entryPoint">The address of the entrypoint of the thread.</param>
				/// <seealso cref="LPTHREAD_START_ROUTINE"/>
				CreateThreadEventEntry(const PROCESS_INFORMATION& pi, uint64_t entryPoint);
			}; 

			/// <summary>
			/// Exit process event, contains the exit code (and pid through the ProcessInformation member).
			/// </summary>
			struct ExitProcessEventEntry : public EventEntry {
				uint32_t	ExitCode = 0;

				/// <summary>
				/// Default constructor, generally used when reading an existing binary log file.
				/// </summary>
				ExitProcessEventEntry();

				/// <summary>
				/// Construct a ExitProcessEventEntry from parameters obtained through a debug event whilst debugging.
				/// </summary>
				/// <param name="pi">A <see cref="PROCESS_INFORMATION"/> instance containing a handle to the process and thread of this event.</param>
				/// <param name="exitCode">The process exit code.</param>
				ExitProcessEventEntry(const PROCESS_INFORMATION& pi, uint32_t exitCode);
			};

			/// <summary>
			/// Exit thread event, contains the exit code (and pid through the ProcessInformation member).
			/// </summary>
			struct ExitThreadEventEntry : public EventEntry {
				uint32_t	ExitCode = 0;

				/// <summary>
				/// Default constructor, generally used when reading an existing binary log file.
				/// </summary>
				ExitThreadEventEntry();

				/// <summary>
				/// Construct a ExitThreadEventEntry from parameters obtained through a debug event whilst debugging.
				/// </summary>
				/// <param name="pi">A <see cref="PROCESS_INFORMATION"/> instance containing a handle to the process and thread of this event.</param>
				/// <param name="exitCode">The thread exit code.</param>
				ExitThreadEventEntry(const PROCESS_INFORMATION& pi, uint32_t exitCode);
			};

			/// <summary>
			/// DLL load event, contains information about module index (order), module base address, module size and path.
			/// path is written after this struct.
			/// </summary>
			struct DllLoadEventEntry : public EventEntry {
				int64_t		ModuleIndex = 0;
				uint64_t	ModuleBase = 0;
				uint64_t	ModuleSize = 0;
				uint64_t	ModulePathSize = 0;

				/// <summary>
				/// Default constructor, generally used when reading an existing binary log file.
				/// </summary>
				DllLoadEventEntry();

				/// <summary>
				/// Construct a DllLoadEventEntry from parameters obtained through a debug event whilst debugging.
				/// </summary>
				/// <param name="pi">A <see cref="PROCESS_INFORMATION"/> instance containing a handle to the process and thread of this event.</param>
				/// <param name="index">The module index in the debugger's LoadedModules collection, which can be used to lookup a module later.</param>
				/// <param name="base">The module base address.</param>
				/// <param name="size">The full module size as loaded in memory.</param>
				/// <param name="pathSize">The length of the full file path of the loaded module, which is to be written after this struct.</param>
				DllLoadEventEntry(const PROCESS_INFORMATION& pi, int64_t index, uint64_t base, uint64_t size, uint64_t pathSize);
			};

			/// <summary>
			/// Debug message event, contains information about the length and encoding of the following string
			/// </summary>
			struct DebugStringEventEntry : public EventEntry {
				uint8_t		IsUnicode = 0;
				uint64_t	Length = 0;

				/// <summary>
				/// Default constructor, generally used when reading an existing binary log file.
				/// </summary>
				DebugStringEventEntry();

				/// <summary>
				/// Construct a DllLoadEventEntry from parameters obtained through a debug event whilst debugging.
				/// </summary>
				/// <param name="pi">A <see cref="PROCESS_INFORMATION"/> instance containing a handle to the process and thread of this event.</param>
				/// <param name="unicode">An integer (non-zero or zero) indicating whether the string is wide-char unicode or not.</param>
				/// <param name="length">The length of the debug string.</param>
				DebugStringEventEntry(const PROCESS_INFORMATION& pi, uint8_t unicode, uint64_t length);
			};

			/// <summary>
			/// RIP debug event, contains type and error
			/// </summary>
			struct RipEventEntry : public EventEntry {
				uint32_t	Type = 0;
				uint32_t	Error = 0;

				/// <summary>
				/// Default constructor, generally used when reading an existing binary log file.
				/// </summary>
				RipEventEntry();

				/// <summary>
				/// Construct a RipEventEntry from parameters obtained through a debug event whilst debugging.
				/// </summary>
				/// <param name="pi">A <see cref="PROCESS_INFORMATION"/> instance containing a handle to the process and thread of this event.</param>
				/// <param name="type">The type of RIP event.</param>
				/// <param name="error">The error code.</param>
				RipEventEntry(const PROCESS_INFORMATION& pi, uint32_t type, uint32_t error);
			};

			/// <summary>
			/// DLL unload event, specifies base address of unloaded module. Module can be obtained
			/// by looking at the information from previous load events.
			/// </summary>
			struct DllUnloadEventEntry : public EventEntry {
				uint64_t	ModuleBase = 0;

				/// <summary>
				/// Default constructor, generally used when reading an existing binary log file.
				/// </summary>
				DllUnloadEventEntry();

				/// <summary>
				/// Construct a DllUnloadEventEntry from parameters obtained through a debug event whilst debugging.
				/// </summary>
				/// <param name="pi">A <see cref="PROCESS_INFORMATION"/> instance containing a handle to the process and thread of this event.</param>
				/// <param name="base">The module base address of the DLL that has been unloaded.</param>
				DllUnloadEventEntry(const PROCESS_INFORMATION& pi, uint64_t base);
			};

			/// <summary>
			/// A simple union type that allows for allocating any EventEntry implementation on the stack.
			/// </summary>
			union EntryFrame {
				ExceptionEventEntry		ExceptionEntry;
				CreateProcessEventEntry CreateProcessEntry;
				CreateThreadEventEntry	CreateThreadEntry;
				ExitProcessEventEntry	ExitProcessEntry;
				ExitThreadEventEntry	ExitThreadEntry;
				DllLoadEventEntry		DllLoadEntry;
				DebugStringEventEntry	DebugStringEntry;
				RipEventEntry			RipEntry;
				DllUnloadEventEntry		DllUnloadEntry;

				/// <summary>
				/// Default constructor, no initialization. Initialization is managed by the debugger.
				/// </summary>
				EntryFrame();
			};

			/// <summary>
			/// The stack trace header, containing information about the recursion limitation, the number 
			/// of decoded instructionsand the amount of stack trace entries.
			/// </summary>
			struct StackTrace {
				char		Signature[4]		= { 'S', 'T', 'C', 'K' };
				uint64_t	MaxRecursion		= 0;
				uint64_t	MaxInstructions		= 0;
				uint64_t	TraceEntries		= 0;

				/// <summary>
				/// Default constructor, generally used when reading an existing binary log file.
				/// </summary>
				StackTrace(); 

				/// <summary>
				/// Construct a StackTrace header.
				/// </summary>
				/// <param name="recursion">The maximum recursion for a stack trace before the entries between get cut out.</param>
				/// <param name="instructions">The maximum number of instructions to decode at the address of the start of the stack trace.</param>
				/// <param name="entries">The number of trace entries (frames).</param>
				StackTrace(uint64_t recursion, uint64_t instructions, uint64_t entries);
			};

			/// <summary>
			/// A stack trace entry, followed by the symbol name, path and decoded instructions.
			/// </summary>
			struct StackTraceEntry {
				int64_t		ModuleIndex;
				uint64_t	ModuleBase;
				uint64_t	Address;
				uint64_t	AbsoluteAddress;
				uint64_t	AbsoluteLineAddress;
				uint64_t	LineAddress;

				uint64_t	NameSymbolLength;
				uint64_t	PathLength;
				uint64_t	LineNumber;

				uint8_t		IsRecursion;
				uint64_t	RecursionCount;
				uint64_t	InstructionCount;
			};

			/// <summary>
			/// A decoded instruction at the location of a stack trace entry, effectively displaying
			/// the instructions at the address of the program counter at that point in the stack trace.
			/// </summary>
			struct StackTraceEntryInstruction {
				uint8_t		Is64BitAddress;
				uint64_t	Offset;
				uint64_t	Size;
				uint64_t	HexSize;
				uint64_t	MnemonicSize;
				uint64_t	OperandsSize;
			};
			#pragma pack(pop)

			/// <summary>
			/// A <see cref="StackTraceEntryInstruction"/> that has been read from file, i.e. for use in <see cref="Hindsight::BinaryLog::BinaryLogPlayer"/>, with 
			/// the appended strings read and placed into this struct.
			/// </summary>
			struct StackTraceEntryInstructionConcrete : public StackTraceEntryInstruction {
				std::string Hex;
				std::string Mnemonic;
				std::string Operands; 
			};

			/// <summary>
			/// A <see cref="StackTraceEntry"/> that has been read from file, i.e. for use in <see cref="Hindsight::BinaryLog::BinaryLogPlayer"/>, with 
			/// the appended strings and instructions read and placed into this struct.
			/// </summary>
			struct StackTraceEntryConcrete : public StackTraceEntry {
				std::string		Name;
				std::wstring	Path;

				std::vector<StackTraceEntryInstructionConcrete> Instructions;
			};

			/// <summary>
			/// A <see cref="StackTrace"/> that has been read from file, i.e. for use in <see cref="Hindsight::BinaryLog::BinaryLogPlayer"/>, with 
			/// the appended entries read and placed into this struct.
			/// </summary>
			struct StackTraceConcrete : public StackTrace {
				std::vector<StackTraceEntryConcrete> Entries;
			};
		}
	}

#endif 