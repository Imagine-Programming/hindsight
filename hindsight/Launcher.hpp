#pragma once

#ifndef process_launcher_h
#define process_launcher_h
	#include "Process.hpp"
	#include "LauncherExceptions.hpp"
	#include <memory>
	#include <string>
	#include <vector>

	namespace Hindsight {
		namespace Process {
			/// <summary>
			/// A simple class with static methods for launching an application that is to be debugged.
			/// </summary>
			class Launcher {
				public:
					/// <summary>
					/// Start a program in suspended state.
					/// </summary>
					/// <param name="path">Image filepath.</param>
					/// <param name="working_directory">Working directory for the new process.</param>
					/// <param name="arguments">Program arguments (argv).</param>
					/// <returns>A shared pointer to an instance of <see cref="::Hindsight::Process::Process"/> of the started process, or <see langword="nullptr"/> on failure.</returns>
					static std::shared_ptr<Process> StartSuspended(const std::string& path, const std::string& working_directory, const std::vector<std::string>& arguments);

					/// <summary>
					/// Convert vector of argument strings to a command-line argument string.
					/// </summary>
					/// <param name="arguments">Program arguments, might include the program path for argv[0].</param>
					/// <returns>The command-line compatible argument string.</returns>
					static std::string GetArgumentString(const std::vector<std::string>& arguments);
			};

		}
	}

#endif 