#pragma once

#ifndef binary_log_player_h
#define binary_log_player_h
	#include "BinaryLogFile.hpp"
	#include "IDebuggerEventHandler.hpp"
	#include "ModuleCollection.hpp"
	#include "DynaCli.hpp"
	#include "ArgumentNames.hpp"

	#include <memory>
	#include <vector>
	#include <fstream>
	#include <ctime>
	#include <set>

	namespace Hindsight {
		namespace BinaryLog {
			using namespace Hindsight::Debugger;

			/// <summary>
			/// A class that mimics the <see cref="Hindsight::Debugger::Debugger"/> by walking a previously written log file in binary format (HIND)
			/// for <see cref="Hindsight::BinaryLog::EventEntry"/> elements and invoking the event handlers using that data. Like this, a recorded 
			/// collection of events can be replayed/viewed at a later moment by a developer for further analysis.
			/// </summary>
			class BinaryLogPlayer {
				private:
					std::ifstream				m_Stream;
					size_t						m_StreamSize;
					const Cli::HindsightCli&	m_State;
					const Cli::HindsightCli&	m_SubState;
					bool						m_ShouldFilter;
					std::set<std::string>		m_Filter;

					FileHeader					m_Header;
					uint32_t					m_Crc32;

					std::vector<std::shared_ptr<EventHandler::IDebuggerEventHandler>> m_Handlers;

					ModuleCollection m_Modules;

					static const size_t ChecksumBufferSize = 2048;
				public:
					/// <summary>
					/// Construct a BinaryLogPlayer instance from a path pointing to a HIND file, and a <see cref="Hindsight::State"/> instance 
					/// describing the program arguments parsed by <see cref="CLI::App"/>.
					/// </summary>
					/// <param name="path">The path to a HIND file, the binary log format of hindsight.</param>
					/// <param name="state">The state obtained through processing program arguments through <see cref="CLI::App"/>.</param>
					/// <exception cref="std::runtime_error">This exception is thrown when the file cannot be opened or is not a valid binary log file.</exception>
					BinaryLogPlayer(const std::string& path, const Cli::HindsightCli& state);

					/// <summary>
					/// Walks the binary log file and verifies that all data matches the <see cref="Hindsight::BinaryLog::FileHeader::Crc32"/>.
					/// </summary>
					/// <exception cref="std::runtime_error">This exception is thrown when the data in the file does not result in the <see cref="Hindsight::BinaryLog::FileHeader::Crc32"/> checksum.</exception>
					void CheckSanity();

					/// <summary>
					/// Add an instance of any implementation of <see cref="Hindsight::Debugger::EventHandler::IDebuggerEventHandler"/> to the player.
					/// </summary>
					/// <param name="handler">An instance of an <see cref="Hindsight::Debugger::EventHandler::IDebuggerEventHandler"/> implementation.</param>
					void AddHandler(std::shared_ptr<EventHandler::IDebuggerEventHandler> handler);

					/// <summary>
					/// Play the binary log file and simulate the debug events that were stored in it.
					/// </summary>
					/// <exception cref="std::runtime_error">
					///	This exception is thrown when the checksum of all data read does not match the stored <see cref="Hindsight::BinaryLog::FileHeader::Crc32"/> 
					/// checksum in the file. This exception can also be thrown when an unexpected frame type is encountered in the file, or when an unexpected end 
					/// is encountered in the file. 
					/// </exception>
					void Play();

				private:
					/// <summary>
					/// Read and process the next <see cref="Hindsight::BinaryLog::EventEntry"/> and emit it as event to the added event handlers.
					/// </summary>
					/// <seealso cref="Hindsight::BinaryLog::BinaryLogPlayer::AddHandler"/>
					/// <returns>A boolean indicating to <see cref="Hindsight::BinaryLog::BinaryLogPlayer::Play"/> that there is more data to be read and played.</returns>
					bool Next();

					/// <summary>
					/// Emit an EXCEPTION debug event to all the debug event handlers after reading all metadata (like paths and stack traces).
					/// This emitter can also emit a BREAKPOINT event, considering that it is an exception too.
					/// </summary>
					/// <param name="time">The recorded time of the event.</param>
					/// <param name="frame">The recorded frame of the event, containing relevant information.</param>
					/// <param name="event">The DEBUG_EVENT instance.</param>
					void EmitException(time_t time, const ExceptionEventEntry& frame, DEBUG_EVENT& event);

					/// <summary>
					/// Emit a CREATE_PROCESS debug event to all the debug event handlers after reading all metadata (like paths).
					/// </summary>
					/// <param name="time">The recorded time of the event.</param>
					/// <param name="frame">The recorded frame of the event, containing relevant information.</param>
					/// <param name="event">The DEBUG_EVENT instance.</param>
					void EmitCreateProcess(time_t time, const CreateProcessEventEntry& frame, DEBUG_EVENT& event);

					/// <summary>
					/// Emit a CREATE_THREAD debug event to all the debug event handlers after reading all metadata (like paths).
					/// </summary>
					/// <param name="time">The recorded time of the event.</param>
					/// <param name="frame">The recorded frame of the event, containing relevant information.</param>
					/// <param name="event">The DEBUG_EVENT instance.</param>
					void EmitCreateThread(time_t time, const CreateThreadEventEntry& frame, DEBUG_EVENT& event);

					/// <summary>
					/// Emit an EXIT_PROCESS debug event to all the debug event handlers after reading all metadata (like paths).
					/// </summary>
					/// <param name="time">The recorded time of the event.</param>
					/// <param name="frame">The recorded frame of the event, containing relevant information.</param>
					/// <param name="event">The DEBUG_EVENT instance.</param>
					void EmitExitProcess(time_t time, const ExitProcessEventEntry& frame, DEBUG_EVENT& event);

					/// <summary>
					/// Emit an EXIT_THREAD debug event to all the debug event handlers after reading all metadata (like paths).
					/// </summary>
					/// <param name="time">The recorded time of the event.</param>
					/// <param name="frame">The recorded frame of the event, containing relevant information.</param>
					/// <param name="event">The DEBUG_EVENT instance.</param>
					void EmitExitThread(time_t time, const ExitThreadEventEntry& frame, DEBUG_EVENT& event);

					/// <summary>
					/// Emit an LOAD_DLL debug event to all the debug event handlers after reading all metadata (like paths).
					/// </summary>
					/// <param name="time">The recorded time of the event.</param>
					/// <param name="frame">The recorded frame of the event, containing relevant information.</param>
					/// <param name="event">The DEBUG_EVENT instance.</param>
					void EmitDllLoad(time_t time, const DllLoadEventEntry& frame, DEBUG_EVENT& event);

					/// <summary>
					/// Emit an OUTPUT_DEBUG_STRING debug event to all the debug event handlers after reading all metadata (like paths).
					/// </summary>
					/// <param name="time">The recorded time of the event.</param>
					/// <param name="frame">The recorded frame of the event, containing relevant information.</param>
					/// <param name="event">The DEBUG_EVENT instance.</param>
					void EmitDebugString(time_t time, const DebugStringEventEntry& frame, DEBUG_EVENT& event);

					/// <summary>
					/// Emit an RIP debug event to all the debug event handlers after reading all metadata (like paths).
					/// </summary>
					/// <param name="time">The recorded time of the event.</param>
					/// <param name="frame">The recorded frame of the event, containing relevant information.</param>
					/// <param name="event">The DEBUG_EVENT instance.</param>
					void EmitRip(time_t time, const RipEventEntry& frame, DEBUG_EVENT& event);

					/// <summary>
					/// Emit an UNLOAD_DLL debug event to all the debug event handlers after reading all metadata (like paths).
					/// </summary>
					/// <param name="time">The recorded time of the event.</param>
					/// <param name="frame">The recorded frame of the event, containing relevant information.</param>
					/// <param name="event">The DEBUG_EVENT instance.</param>
					void EmitDllUnload(time_t time, const DllUnloadEventEntry& frame, DEBUG_EVENT& event);

					/// <summary>
					/// Determines the size of the binary log stream, thus the filesize.
					/// </summary>
					/// <returns>An integer representing the filesize of this binary log file.</returns>
					inline size_t Size();

					/// <summary>
					/// Determines the current location the player is reading from in the stream.
					/// </summary>
					/// <returns>An integer representing the current location in the binary log file stream.</returns>
					inline size_t Pos();

					/// <summary>
					/// Determines the number of bytes that are remaining in the stream to be read.
					/// </summary>
					/// <returns>An integer representing the remaining size in the stream.</returns>
					inline size_t SizeLeft();

					/// <summary>
					/// Determines the number of bytes that are remaining in the stream to be read, and 
					/// verifies that it is enough to meet the value specified in <paramref name="required"/>.
					/// </summary>
					/// <param name="required">The number of bytes required to be available in the stream.</param>
					inline void AssertSizeLeft(size_t required);

					/// <summary>
					/// Read a value T from the stream directly to the value referenced by <paramref name="result"/>.
					/// </summary>
					/// <param name="result">A reference to a <typeparamref name="T"/> value to read to, which must be a class or struct.</param>
					/// <typeparam name="T">The type of value to read from the stream.</typeparam>
					template <typename T, std::enable_if_t<std::is_class<T>::value, int> = 0>
					void Read(T& result);

					/// <summary>
					/// Read a value T from the stream directly to the value referenced by <paramref name="result"/>.
					/// </summary>
					/// <param name="result">A reference to a <typeparamref name="T"/> value to read to, which must be an integer.</param>
					/// <typeparam name="T">The type of value to read from the stream.</typeparam>
					template <typename T, std::enable_if_t<std::is_integral<T>::value, int> = 0>
					void Read(T& result);

					/// <summary>
					/// Read <paramref name="size"/> bytes from the stream into <paramref name="buffer"/> and optionally update the checksum.
					/// </summary>
					/// <param name="buffer">The buffer to read to, it must point to enough memory to hold <paramref name="size"/> bytes.</param>
					/// <param name="size">The number of bytes to read.</param>
					/// <param name="updateChecksum">Whether to update the internal checksum or not, used in verifying data integrity.</param>
					void Read(char* buffer, size_t size, bool updateChecksum = true);

					/// <summary>
					/// Read an ANSI string from the stream to <paramref name="result"/>. When <paramref name="size"/> is specified, it will read 
					/// exactly that many characters (not bytes). If it is not specified, it will read the length first as a <see cref="uint32_t"/>.
					/// </summary>
					/// <param name="result">A reference to a <see cref="::std::string" /> instance that will hold the result.</param>
					/// <param name="size">The number of characters to read, or -1 to read the length from the stream.</param>
					void Read(std::string& result, int64_t size = -1);

					/// <summary>
					/// Read a unicode string from the stream to <paramref name="result"/>. When <paramref name="size"/> is specified, it will read 
					/// exactly that many characters (not bytes). If it is not specified, it will read the length first as a <see cref="uint32_t"/>.
					/// </summary>
					/// <param name="result">A reference to a <see cref="::std::wstring" /> instance that will hold the result.</param>
					/// <param name="size">The number of characters to read, or -1 to read the length from the stream.</param>
					void Read(std::wstring& result, int64_t size = -1);

					/// <summary>
					/// Read a vector of <paramref name="count"/> ANSI strings from the stream to <paramref name="result"/>.
					/// </summary>
					/// <param name="result">A reference to a <see cref="::std::vector<::std::string>"/> instance that will hold the result.</param>
					/// <param name="count">The number of strings to read.</param>
					/// <remarks>note: the length of each string in the vector is read before reading the string, like in <see cref="::Hindsight::BinaryLog::BinaryLogPlayer::Read(std::string& result, int64_t size)"/></remarks>
					void Read(std::vector<std::string>& result, size_t count);

					/// <summary>
					/// A breakpoint or exception was emitted and the user specified either or both the --break-breakpoint or --break-exception flags, 
					/// this method handles the actual breaking.
					/// </summary>
					void HandleBreakpointOptions();

			};
		}
	}

#endif 