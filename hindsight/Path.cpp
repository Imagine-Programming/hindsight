#include "Path.hpp"
#include <Windows.h>
#include <Psapi.h>
#include <filesystem>

namespace fs = std::filesystem;

using namespace Hindsight::Utilities;

/// <summary>
/// Determines the absolute path for a certain input path.
/// </summary>
/// <param name="path">The input path.</param>
/// <returns>The absolute path.</returns>
std::string Path::Absolute(const std::string& path) {
	fs::path p(path);

	if (p.is_absolute())
		return p.string();

	return fs::absolute(p).string();
}

/// <summary>
/// Get the full path to a certain file denoted by a file handle.
/// </summary>
/// <param name="file">The handle to the open file to determine the path for.</param>
/// <returns>The full filepath, or an empty string when something went wrong.</returns>
std::string Path::GetPathFromFileHandleA(HANDLE file) {
	char c;
	auto size = (size_t)GetFinalPathNameByHandleA(file, &c, 1, FILE_NAME_NORMALIZED);

	if (size == 0)
		return "";

	// Reserve some space for the result in a vector, something we can easily convert to a string.
	std::vector<char> buff(size);
	if ((size_t)GetFinalPathNameByHandleA(file, &buff[0], (DWORD)size, FILE_NAME_NORMALIZED) != size - 1)
		return "";

	// Convert the result to a string, cutting off the NUL-character from the end.
	std::string resultString(buff.begin(), buff.begin() + size - 1);

	// Normalize the path.
	if (resultString.substr(0, 4) == "\\\\?\\")
		resultString = resultString.substr(4);

	if (resultString.substr(0, 3) == "UNC")
		resultString = resultString.substr(3);

	// Make sure the path is an absolute path.
	return fs::absolute(resultString).string();
}

/// <summary>
/// Get the full unicode path to a certain file denoted by a file handle.
/// </summary>
/// <param name="file">The handle to the open file to determine the path for.</param>
/// <returns>The full unicode filepath, or an empty string when something went wrong.</returns>
std::wstring Path::GetPathFromFileHandleW(HANDLE file) {
	wchar_t c;
	auto size = (size_t)GetFinalPathNameByHandleW(file, &c, 1, FILE_NAME_NORMALIZED);

	if (size == 0)
		return L"";

	// Reserve some space for the result in a vector, something we can easily convert to a string.
	std::vector<wchar_t> buff(size);
	if ((size_t)GetFinalPathNameByHandleW(file, &buff[0], (DWORD)size, FILE_NAME_NORMALIZED) != size - 1)
		return L"";

	// Convert the result to a string, cutting off the NUL-character from the end.
	std::wstring resultString(buff.begin(), buff.begin() + size - 1);

	// Normalize the path.
	if (resultString.substr(0, 4) == L"\\\\?\\")
		resultString = resultString.substr(4);

	if (resultString.substr(0, 3) == L"UNC")
		resultString = resultString.substr(3);

	// Make sure the path is an absolute path.
	return fs::absolute(resultString).wstring();
}

/// <summary>
/// Get the full path to a module <paramref name="hModule"/> loaded into <paramref name="hProcess"/>. When NULL 
/// is provided as <paramref name="hModule"/>, the path for the executing module is returned.
/// </summary>
/// <param name="hProcess">The handle to the process that loaded the module.</param>
/// <param name="hModule">The module base address, or NULL when the path of the main module is to be returned.</param>
/// <returns>The full module path.</returns>
std::string Path::GetModulePath(HANDLE hProcess, HMODULE hModule) {
	char path[MAX_PATH + 1] = { 0 };
	GetModuleFileNameExA(hProcess, hModule, path, MAX_PATH);
	return fs::path(path).parent_path().string();
}

/// <summary>
/// Ensures that all the directories in <paramref name="path"/> exist and creates them otherwise.
/// </summary>
/// <param name="path">The path to verify.</param>
/// <returns>When successful, true is returned.</returns>
bool Path::EnsureDirectoryExists(const std::string& path) {
	if (!fs::is_directory(path))
		return fs::create_directories(path);
	return true;
}

/// <summary>
/// Ensures that all the directories of the parent path of <paramref name="path"/> exist and creates them otherwise.
/// </summary>
/// <param name="path">The path of which the parent path must be verified.</param>
/// <returns>When successful, true is returned.</returns>
bool Path::EnsureParentExists(const std::string& path) {
	auto parent = fs::path(path).parent_path();
	if (!fs::is_directory(parent))
		return fs::create_directories(parent);
	return true;
}