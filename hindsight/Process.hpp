#pragma once

#ifndef process_process_h
#define process_process_h
	#include <Windows.h>
	#include <DbgHelp.h>
	#include <string>
	#include <vector>

	namespace Hindsight {
		namespace Process {
			/// <summary>
			/// The process class describes a non-dead process that can be debugged.
			/// </summary>
			class Process {
				public:
					/// <summary>
					/// The program image filepath.
					/// </summary>
					std::string Path;

					/// <summary>
					/// The starting working directory of the process.
					/// </summary>
					std::string WorkingDirectory;

					/// <summary>
					/// Program arguments (argv).
					/// </summary>
					std::vector<std::string> Arguments;

					/// <summary>
					/// The process handle.
					/// </summary>
					HANDLE hProcess;

					/// <summary>
					/// The main thread handle.
					/// </summary>
					HANDLE hThread;

					/// <summary>
					/// The PID.
					/// </summary>
					DWORD dwProcessId;

					/// <summary>
					/// The TID.
					/// </summary>
					DWORD dwThreadId;

					/// <summary>
					/// Construct a new process based on a const reference to a <see cref="PROCESS_INFORMATION"/> instance, a path, a working directory and program arguments.
					/// </summary>
					/// <param name="pi">A const reference to a <see cref="PROCESS_INFORMATION"/> struct.</param>
					/// <param name="path">The program path.</param>
					/// <param name="workingDirectory">The program starting working directory.</param>
					/// <param name="args">The program arguments.</param>
					Process(const PROCESS_INFORMATION& pi, const std::string& path, const std::string& workingDirectory, const std::vector<std::string>& args);

					/// <summary>
					/// The destructor which closes the process and thread handles.
					/// </summary>
					~Process();

					/// <summary>
					/// Create a <see cref="PROCESS_INFORMATION"/> struct about this instance.
					/// </summary>
					/// <returns>A <see cref="PROCESS_INFORMATION"/> struct.</returns>
					PROCESS_INFORMATION GetProcessInformation();

					/// <summary>
					/// Resume the main thread.
					/// </summary>
					/// <returns>When successful, true is returned.</returns>
					bool Resume();

					/// <summary>
					/// Suspend the main thread.
					/// </summary>
					/// <returns>When successful, true is returned.</returns>
					bool Suspend();

					/// <summary>
					/// Determines if the process is still running/alive.
					/// </summary>
					/// <returns>When the process is alive, true is returned.</returns>
					/// <remarks>
					/// note: when the process has an exit code STILL_ACTIVE, this function will return true.
					/// </remarks>
					bool Running();

					/// <summary>
					/// Determines if the process is a 32-bit program running in WOW64 mode.
					/// </summary>
					/// <returns>When the process is a WOW64 process, true is returned.</returns>
					bool IsWow64();

					/// <summary>
					/// Determines if the process is a 64-bit native program.
					/// </summary>
					/// <returns>When the process is not a WOW64 process, true is returned.</returns>
					/// <remarks>note: identical to !<see cref="IsWow64"/>().</remarks>
					bool Is64();

					/// <summary>
					/// Close the process and thread handles.
					/// </summary>
					void Close();

					/// <summary>
					/// Kill the process forcefully.
					/// </summary>
					/// <param name="exitCode">The exit code to terminate the process with, such as an exception code.</param>
					/// <remarks>
					/// note: This is used in post-mortem debugging to kill the process.
					/// </remarks>
					void Kill(UINT exitCode);

					/// <summary>
					/// Read a string from the memory space of the process.
					/// </summary>
					/// <param name="address">The address in the memory space of the process where the string is stored.</param>
					/// <param name="length">The length of the string, in bytes.</param>
					/// <returns>The resulting string, or "" when something went wrong.</returns>
					std::string ReadString(void* address, size_t length) const;

					/// <summary>
					/// Read a wchar_t string from the memory space of the process.
					/// </summary>
					/// <param name="address">The address in the memory space of the process where the string is stored.</param>
					/// <param name="length">The length of the string, in bytes.</param>
					/// <returns>The resulting string, or L"" when something went wrong.</returns>
					std::wstring ReadStringW(void* address, size_t length) const;
					
					/// <summary>
					/// Read an arbitrary type from the memory space of the process.
					/// </summary>
					/// <param name="address">The address in the memory space of the process where the value is stored.</param>
					/// <param name="out">A reference to a value of type <typeparamref name="TRead"/> where the result is stored.</param>
					/// <typeparam name="TRead">The type to read.</typeparam>
					/// <returns>When successful, true is returned.</returns>
					template <typename TRead>
					bool Read(const void* address, TRead& out) const {
						SIZE_T readSize;

						if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address), reinterpret_cast<LPVOID>(&out), sizeof(TRead), &readSize))
							return false;

						if (static_cast<size_t>(readSize) != sizeof(TRead))
							return false;

						return true;
					}

					/// <summary>
					/// Read an arbitrary amount of memory from the memory space of the process.
					/// </summary>
					/// <param name="address">The address in the memory space of the process where the value is stored.</param>
					/// <param name="length">The length of data to read, in bytes.</param>
					/// <param name="output">The output buffer that will contain the read data.</param>
					/// <returns>When successful, true is returned.</returns>
					bool Read(const void* address, size_t length, void* output) const;

					/// <summary>
					/// Read a (c-style) string from the memory space of the process which should terminate with a single or more \0 character(s).
					/// </summary>
					/// <param name="address">The address in the memory space of the process where the string is stored.</param>
					/// <param name="maximumLength">The length in bytes at which the scan should stop searching for NUL.</param>
					/// <returns>The resulting string, or "" when something went wrong.</returns>
					std::string ReadNulTerminatedString(const void* address, size_t maximumLength = 0) const;
			};

		}
	}

#endif 