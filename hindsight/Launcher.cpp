#include "Launcher.hpp"
#include "String.hpp"
#include <Windows.h>
#include <numeric>
#include <iostream>
#include <filesystem>

using namespace Hindsight::Process;
using namespace Hindsight::Utilities;

namespace fs = std::filesystem;

/// <summary>
/// Start a program in suspended state.
/// </summary>
/// <param name="path">Image filepath.</param>
/// <param name="working_directory">Working directory for the new process.</param>
/// <param name="arguments">Program arguments (argv).</param>
/// <returns>A shared pointer to an instance of <see cref="::Hindsight::Process::Process"/> of the started process, or <see langword="nullptr"/> on failure.</returns>
std::shared_ptr<Process> Launcher::StartSuspended(const std::string& path, const std::string& working_directory, const std::vector<std::string>& arguments) {
	auto workdir = working_directory;
	if (workdir.empty())
		workdir = fs::path(path).parent_path().string();

	PROCESS_INFORMATION pi;
	STARTUPINFOA si = { sizeof(STARTUPINFOA) };
	auto argument_list = arguments;
	argument_list.insert(argument_list.begin(), path);
	auto argument_string = GetArgumentString(argument_list);

	auto status = ::CreateProcessA(
		nullptr,
		const_cast<char*>(argument_string.c_str()),
		nullptr,
		nullptr,
		false,
		CREATE_SUSPENDED,
		nullptr,
		workdir.c_str(),
		&si,
		&pi
	);

	if (!status) {
		auto exception = Hindsight::Exceptions::LauncherFailedException(GetLastError());
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		throw exception;
	}

	return std::make_shared<Process>(pi, path, workdir, arguments);
}

/// <summary>
/// Convert vector of argument strings to a command-line argument string.
/// </summary>
/// <param name="arguments">Program arguments, might include the program path for argv[0].</param>
/// <returns>The command-line compatible argument string.</returns>
std::string Launcher::GetArgumentString(const std::vector<std::string>& arguments) {
	std::vector<std::string> escaped;

	for (auto& a : arguments)
		escaped.push_back(String::ReplaceAll(a, "\"", "\\\""));

	return std::accumulate(escaped.begin(), escaped.end(), std::string(),
		[](const auto& a, const auto& b) {
			return a + (a.length() > 0 ? " " : "") + b;
		});
}