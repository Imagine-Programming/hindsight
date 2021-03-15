#include "Process.hpp"
using namespace Hindsight::Process;

/// <summary>
/// Construct a new process based on a const reference to a <see cref="PROCESS_INFORMATION"/> instance, a path, a working directory and program arguments.
/// </summary>
/// <param name="pi">A const reference to a <see cref="PROCESS_INFORMATION"/> struct.</param>
/// <param name="path">The program path.</param>
/// <param name="workingDirectory">The program starting working directory.</param>
/// <param name="args">The program arguments.</param>
Process::Process(
	const PROCESS_INFORMATION& pi, 
	const std::string& path, 
	const std::string& workingDirectory, 
	const std::vector<std::string>& args) : 

	hProcess(pi.hProcess),
	hThread(pi.hThread), 
	dwProcessId(pi.dwProcessId),
	dwThreadId(pi.dwThreadId),
	Path(path),
	WorkingDirectory(workingDirectory),
	Arguments(args) {

}

/// <summary>
/// The destructor which closes the process and thread handles.
/// </summary>
Process::~Process() {
	Close();
}

/// <summary>
/// Create a <see cref="PROCESS_INFORMATION"/> struct about this instance.
/// </summary>
/// <returns>A <see cref="PROCESS_INFORMATION"/> struct.</returns>
PROCESS_INFORMATION Process::GetProcessInformation() {
	return {
		hProcess,
		hThread,
		dwProcessId,
		dwThreadId
	};
}

/// <summary>
/// Resume the main thread.
/// </summary>
/// <returns>When successful, true is returned.</returns>
bool Process::Resume() {
	return ResumeThread(hThread) != (DWORD)-1;
}

/// <summary>
/// Suspend the main thread.
/// </summary>
/// <returns>When successful, true is returned.</returns>
bool Process::Suspend() {
	return SuspendThread(hThread) != (DWORD)-1;
}

/// <summary>
/// Determine if the process is still running/alive.
/// </summary>
/// <returns>When the process is alive, true is returned.</returns>
/// <remarks>
/// note: when the process has an exit code STILL_ACTIVE, this function will return true.
/// </remarks>
bool Process::Running() {
	DWORD exitCode;
	return GetExitCodeProcess(hProcess, &exitCode) && exitCode == STILL_ACTIVE;
}

/// <summary>
/// Determines if the process is a 32-bit program running in WOW64 mode.
/// </summary>
/// <returns>When the process is a WOW64 process, true is returned.</returns>
bool Process::IsWow64() {
	BOOL iswow;
	return IsWow64Process(hProcess, &iswow) && iswow;
}

/// <summary>
/// Determines if the process is a 64-bit native program.
/// </summary>
/// <returns>When the process is not a WOW64 process, true is returned.</returns>
/// <remarks>note: identical to !<see cref="IsWow64"/>().</remarks>
bool Process::Is64() {
	return !IsWow64();
}

/// <summary>
/// Close the process and thread handles.
/// </summary>
void Process::Close() {
	if (hThread)  { CloseHandle(hThread); hThread = NULL; }
	if (hProcess) {	CloseHandle(hProcess); hProcess = NULL; }
}

/// <summary>
/// Kill the process forcefully.
/// </summary>
/// <param name="exitCode">The exit code to terminate the process with, such as an exception code.</param>
/// <remarks>
/// note: This is used in post-mortem debugging to kill the process.
/// </remarks>
void Process::Kill(UINT exitCode) {
	TerminateProcess(hProcess, exitCode);
}

/// <summary>
/// Read a string from the memory space of the process.
/// </summary>
/// <param name="address">The address in the memory space of the process where the string is stored.</param>
/// <param name="length">The length of the string, in bytes.</param>
/// <returns>The resulting string, or "" when something went wrong.</returns>
std::string Process::ReadString(void* address, size_t length) const {
	if (length == 0)
		return "";

	SIZE_T read;
	std::string result(length, ' ');

	if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address), reinterpret_cast<LPVOID>(&result[0]), length, &read))
		return "";

	return result;
}

/// <summary>
/// Read a wchar_t string from the memory space of the process.
/// </summary>
/// <param name="address">The address in the memory space of the process where the string is stored.</param>
/// <param name="length">The length of the string, in bytes.</param>
/// <returns>The resulting string, or L"" when something went wrong.</returns>
std::wstring Process::ReadStringW(void* address, size_t length) const {
	if (length == 0)
		return L"";

	SIZE_T read;
	std::wstring result(length / sizeof(wchar_t), L' ');

	if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address), reinterpret_cast<LPVOID>(&result[0]), length, &read))
		return L"";

	return result;
}

/// <summary>
/// Read an arbitrary amount of memory from the memory space of the process.
/// </summary>
/// <param name="address">The address in the memory space of the process where the value is stored.</param>
/// <param name="length">The length of data to read, in bytes.</param>
/// <param name="output">The output buffer that will contain the read data.</param>
/// <returns>When successful, true is returned.</returns>
bool Process::Read(const void* address, size_t length, void* output) const {
	if (length == 0)
		return false;

	SIZE_T readSize;

	if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address), reinterpret_cast<LPVOID>(output), length, &readSize))
		return false;

	if (static_cast<size_t>(readSize) != length)
		return false;

	return true;
}

/// <summary>
/// Read a (c-style) string from the memory space of the process which should terminate with a single or more \0 character(s).
/// </summary>
/// <param name="address">The address in the memory space of the process where the string is stored.</param>
/// <param name="maximumLength">The length in bytes at which the scan should stop searching for NUL.</param>
/// <returns>The resulting string, or "" when something went wrong.</returns>
std::string Process::ReadNulTerminatedString(const void* address, size_t maximumLength) const {
	std::vector<char> data;

	char   character;
	size_t offset = 0;

	while (true) {
		if (!Read(static_cast<const void*>(static_cast<const uint8_t*>(address) + offset), character))
			return "";

		data.push_back(character);

		if (character == 0)
			break;

		offset += 1;

		if (maximumLength != 0 && offset >= maximumLength) {
			data.push_back(0);
			break;
		}
	}

	return std::string(data.begin(), data.end());
}