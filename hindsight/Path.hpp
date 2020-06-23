#pragma once

#ifndef util_path_h
#define util_path_h
	#include <string>
	#include <Windows.h>

	namespace Hindsight {
		namespace Utilities {
			/// <summary>
			/// A class with utility functions for handling filesystem paths and what they point to.
			/// </summary>
			class Path {
				public:
					/// <summary>
					/// Determines the absolute path for a certain input path.
					/// </summary>
					/// <param name="path">The input path.</param>
					/// <returns>The absolute path.</returns>
					static std::string Absolute(const std::string& path);

					/// <summary>
					/// Get the full path to a certain file denoted by a file handle.
					/// </summary>
					/// <param name="file">The handle to the open file to determine the path for.</param>
					/// <returns>The full filepath, or an empty string when something went wrong.</returns>
					static std::string GetPathFromFileHandleA(HANDLE file);

					/// <summary>
					/// Get the full unicode path to a certain file denoted by a file handle.
					/// </summary>
					/// <param name="file">The handle to the open file to determine the path for.</param>
					/// <returns>The full unicode filepath, or an empty string when something went wrong.</returns>
					static std::wstring GetPathFromFileHandleW(HANDLE file);

					/// <summary>
					/// Get the full path to a module <paramref name="hModule"/> loaded into <paramref name="hProcess"/>. When NULL 
					/// is provided as <paramref name="hModule"/>, the path for the executing module is returned.
					/// </summary>
					/// <param name="hProcess">The handle to the process that loaded the module.</param>
					/// <param name="hModule">The module base address, or NULL when the path of the main module is to be returned.</param>
					/// <returns>The full module path.</returns>
					static std::string GetModulePath(HANDLE hProcess, HMODULE hModule);

					/// <summary>
					/// Ensures that all the directories in <paramref name="path"/> exist and creates them otherwise.
					/// </summary>
					/// <param name="path">The path to verify.</param>
					/// <returns>When successful, true is returned.</returns>
					static bool EnsureDirectoryExists(const std::string& path);

					/// <summary>
					/// Ensures that all the directories of the parent path of <paramref name="path"/> exist and creates them otherwise.
					/// </summary>
					/// <param name="path">The path of which the parent path must be verified.</param>
					/// <returns>When successful, true is returned.</returns>
					static bool	EnsureParentExists(const std::string& path);
			};
		}
	}

#endif 