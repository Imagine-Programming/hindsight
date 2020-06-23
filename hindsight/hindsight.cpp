#include <iostream>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <conio.h>

#include <Windows.h>
#include <Psapi.h>
#include "resource.h"

#include "CLI11.hpp"
#include "rang.hpp"
#include "Version.hpp"

#include "State.hpp"
#include "EventFilterValidator.hpp"

#include "Launcher.hpp"
#include "Process.hpp"
#include "Debugger.hpp"
#include "BinaryLogPlayer.hpp"
#include "PrintingDebuggerEventHandler.hpp"
#include "WriterDebuggerEventHandler.hpp"
#include "Path.hpp"
#include "String.hpp"

namespace fs = std::filesystem;
using namespace Hindsight;

/// <summary>
/// The maximum username length.
/// </summary>
static constexpr auto UNLEN = 256;

/// <summary>
/// Retrieve the hindsight version as string.
/// </summary>
/// <returns>The hindsight version string.</returns>
static const std::string get_version() {
	return std::string(hindsight_version_full);
}

/// <summary>
/// Format the provided <see cref="tm"/> as time string.
/// </summary>
/// <param name="time">The <see cref="tm"/> instance to format.</param>
/// <returns>The time in <paramref name="time"/> as time string.</returns>
std::string ftime(tm& time) {
	std::ostringstream ss;
	ss << std::put_time(&time, "%H_%M_%S");
	return ss.str();
}

/// <summary>
/// Format the provided <see cref="tm"/> as date string.
/// </summary>
/// <param name="time">The <see cref="tm"/> instance to format.</param>
/// <returns>The time in <paramref name="time"/> as date string.</returns>
std::string fdate(tm& time) {
	std::ostringstream ss;
	ss << std::put_time(&time, "%d-%m-%Y");
	return ss.str();
}

/// <summary>
/// Preprocess a provided filepath by replacing certain placeholders with concrete values. 
/// - $time will be replaced with a hh:mm:ss format of the time;
/// - $date will be replaced with a dd-mm-yyyy format of the time;
/// - $image will be replaced with the name of the debugged image;
/// - $hostname will be replaced with the FQDNS name of the host;
/// - $username will be replaced with the user running the debugged process;
/// - $random will be replaced with a random number between 0 and 1e6.
/// </summary>
/// <param name="input">A reference to the path to process.</param>
/// <param name="time">The time of the preprocessing.</param>
/// <param name="image">The debugged process' image name.</param>
void PreProcessPath(std::string& input, tm& time, const std::string& image) {
	DWORD length;

	if (Utilities::String::Contains(input, "$time"))
		input = Utilities::String::ReplaceAll(input, "$time", ftime(time));
	if (Utilities::String::Contains(input, "$date"))
		input = Utilities::String::ReplaceAll(input, "$date", fdate(time));
	if (Utilities::String::Contains(input, "$image"))
		input = Utilities::String::ReplaceAll(input, "$image", image);

	if (Utilities::String::Contains(input, "$hostname")) {
		// Retrieve the hostname and replace all occurrences of $hostname with the result.
		length = 0;
		GetComputerNameExA(ComputerNameDnsFullyQualified, nullptr, &length);
		if (length) {
			std::vector<char> hostname(static_cast<size_t>(length) + 1);

			if (GetComputerNameExA(ComputerNameDnsFullyQualified, &hostname[0], &length)) 
				input = Utilities::String::ReplaceAll(input, "$hostname", std::string(hostname.begin(), hostname.begin() + static_cast<size_t>(length)));
		}
	}

	if (Utilities::String::Contains(input, "$username")) {
		// Retrieve the username and replace all occurrences of $username with the result.
		std::vector<char> username(UNLEN + 1);
		length = UNLEN;

		/*
			Note that 1 is subtracted from the length, to account for the NUL-character that is included in the character count by GetUserNameA,
			just another gorgeous example of consistency in the Windows API.
		*/
		if (GetUserNameA(&username[0], &length))
			input = Utilities::String::ReplaceAll(input, "$username", std::string(username.begin(), username.begin() + static_cast<size_t>(length) - 1));
	}

	if (Utilities::String::Contains(input, "$random")) {
		// Add a random value
		srand(static_cast<long unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
		auto r = static_cast<uint64_t>((static_cast<double>(rand()) / static_cast<double>(RAND_MAX)) * 1e6);
		input = Utilities::String::ReplaceAll(input, "$random", std::to_string(r));
	}
}

/// <summary>
/// Preprocess all the filepaths in the CLI parser state.
/// </summary>
/// <param name="state">The state obtained through processing program arguments through <see cref="CLI::App"/>.</param>
/// <param name="process">The <see cref="::Hindsight::Process::Process"/> that is being debugged.</param>
void PreProcessState(State& state, std::shared_ptr<Hindsight::Process::Process> process) {
	auto t  =  std::time(nullptr);
	tm time;
	localtime_s(&time, &t); 

	auto image = fs::path(process->Path).filename().string();

	if (!state.OutputBinaryFile.empty())
		PreProcessPath(state.OutputBinaryFile, time, image);
	if (!state.OutputTextFile.empty())
		PreProcessPath(state.OutputTextFile, time, image);
}

/// <summary>
/// For <see cref="pause(const char* what)"/>, display "Press any key to close this window".
/// </summary>
static const char* close_window = "close this window";

/// <summary>
/// For <see cref="pause(const char* what)"/>, display "continue".
/// </summary>
static const char* continue_window = "continue";

/// <summary>
/// Use <see cref="::std::istream::ignore"/> to simulate a pause and display a message.
/// </summary>
/// <param name="what">The suffix to "Press any key to ".</param>
void pause(const char* what) {
	std::cout << "Press any key to " << what << "." << std::endl;
	(void)_getch();
}

/// <summary>
/// Execute the hindsight [options] launch [options] command.
/// </summary>
/// <param name="state">The state obtained through processing program arguments through <see cref="CLI::App"/>.</param>
/// <returns>The program exit code.</returns>
int LaunchCommand(State& state) {
	std::shared_ptr<Hindsight::Process::Process>   process;
	std::shared_ptr<Hindsight::Debugger::Debugger> debugger;

	if (state.MaxRecursion == 0)
		state.MaxRecursion = SIZE_MAX;

	try {
		process = Hindsight::Process::Launcher::StartSuspended(
			Hindsight::Utilities::Path::Absolute(state.ProgramPath), 
			Hindsight::Utilities::Path::Absolute(state.WorkingDirectory), 
			state.Arguments);

		PreProcessState(state, process);
	} catch (const Hindsight::Exceptions::LauncherFailedException& e) {
		std::cout << rang::fgB::red << "error: " << e.what() << std::endl << rang::style::reset;
		return 1;
	}

	// attach debugger
	try {
		debugger = std::make_shared<Hindsight::Debugger::Debugger>(process, state);
	} catch (const Hindsight::Exceptions::ProcessNotRunningException & e) {
		std::cout << rang::fgB::red << "error: " << e.what() << std::endl << rang::style::reset;
		return 1;
	}

	// write to stdout?
	if (state.StandardOut) 
		debugger->AddHandler(std::make_shared<Hindsight::Debugger::EventHandler::PrintingDebuggerEventHandler>(!state.Bland, state.PrintTimestamp, state.PrintContext));
	
	// write to text file?
	if (state.TextFileOut) {
		Utilities::Path::EnsureParentExists(state.OutputTextFile);
		debugger->AddHandler(std::make_shared<Hindsight::Debugger::EventHandler::PrintingDebuggerEventHandler>(state.OutputTextFile, state.PrintContext));
	}

	// write to binary file?
	if (state.BinaryFileOut) {
		Utilities::Path::EnsureParentExists(state.OutputBinaryFile);
		debugger->AddHandler(std::make_shared<Hindsight::Debugger::EventHandler::WriterDebuggerEventHandler>(state.OutputBinaryFile));
	}

	if (!debugger->Attach()) {
		auto lastError = GetLastError();
		std::cout << rang::fgB::red << "error: cannot attach debugger (" << lastError << "), " << Hindsight::Utilities::Error::GetErrorMessage(lastError) << std::endl << rang::style::reset;
		return 1; 
	}

	// resume main thread 
	process->Resume();

	// start the debugger
	debugger->Start();

	return 0;
}

/// <summary>
/// Execute the hindsight [options] replay [options] command.
/// </summary>
/// <param name="state">The state obtained through processing program arguments through <see cref="CLI::App"/>.</param>
/// <returns>The program exit code.</returns>
int ReplayCommand(State& state) {
	std::shared_ptr<Hindsight::BinaryLog::BinaryLogPlayer> player;

	try {
		player = std::make_shared<Hindsight::BinaryLog::BinaryLogPlayer>(state.ReplayFile, state);
	} catch (const std::exception & e) {
		std::cout << rang::fgB::red << "error: " << e.what() << std::endl << rang::style::reset;
	}

	// write to stdout?
	if (state.StandardOut)
		player->AddHandler(std::make_shared<Hindsight::Debugger::EventHandler::PrintingDebuggerEventHandler>(!state.Bland, state.PrintTimestamp, state.PrintContext));

	// write to text file?
	if (state.TextFileOut) {
		Utilities::Path::EnsureParentExists(state.OutputTextFile);
		player->AddHandler(std::make_shared<Hindsight::Debugger::EventHandler::PrintingDebuggerEventHandler>(state.OutputTextFile, state.PrintContext));
	}

	// write to binary file?
	if (state.BinaryFileOut) {
		Utilities::Path::EnsureParentExists(state.OutputBinaryFile);
		player->AddHandler(std::make_shared<Hindsight::Debugger::EventHandler::WriterDebuggerEventHandler>(state.OutputBinaryFile));
	}

	try {
		player->Play();
	} catch (const std::exception& e) {
		std::cout << rang::fgB::red << "error: " << e.what() << std::endl << rang::style::reset;
	}
	
	return 0;
}

/// <summary>
/// Execute the hindsight [options] mortem [options] command.
/// </summary>
/// <param name="state">The state obtained through processing program arguments through <see cref="CLI::App"/>.</param>
/// <returns>The program exit code.</returns>
int MortemCommand(State& state) {
	HWND hWndConsole = GetConsoleWindow();
	ShowWindow(hWndConsole, SW_HIDE);

	auto hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, state.PostMortemProcessId);
	if (!hProcess) {
		std::cout << rang::fgB::red << "error: cannot open debuggee process with all access, " << Utilities::Error::GetErrorMessage(GetLastError()) << rang::style::reset << std::endl;
		return 1;
	}

	std::shared_ptr<Hindsight::Process::Process>   process;
	std::shared_ptr<Hindsight::Debugger::Debugger> debugger;

	if (state.MaxRecursion == 0)
		state.MaxRecursion = SIZE_MAX;

	try {
		std::string path;
		std::string wdir;
		std::vector<std::string> args;
		PROCESS_INFORMATION pi = {
			hProcess,
			NULL,
			state.PostMortemProcessId,
			NULL
		};
		process = std::make_shared<Hindsight::Process::Process>(pi, path, wdir, args);

		char imagePath[MAX_PATH + 1] = { 0 };
		GetModuleFileNameExA(process->hProcess, NULL, imagePath, MAX_PATH);
		//GetProcessImageFileNameA(process->hProcess, imagePath, MAX_PATH);
		process->Path = imagePath;
		process->WorkingDirectory = "";
		process->Arguments = {};

		PreProcessState(state, process);
	} catch (const Hindsight::Exceptions::LauncherFailedException& e) {
		std::cout << rang::fgB::red << "error: " << e.what() << std::endl << rang::style::reset;
		return 1;
	}

	// attach debugger
	try {
		debugger = std::make_shared<Hindsight::Debugger::Debugger>(process, state, state.PostMortemEvent, state.PostMortemJitDebugEventInfo);
	} catch (const Hindsight::Exceptions::ProcessNotRunningException & e) {
		std::cout << rang::fgB::red << "error: " << e.what() << std::endl << rang::style::reset;
		return 1;
	}

	// write to text file?
	if (state.TextFileOut) {
		Utilities::Path::EnsureParentExists(state.OutputTextFile);
		debugger->AddHandler(std::make_shared<Hindsight::Debugger::EventHandler::PrintingDebuggerEventHandler>(state.OutputTextFile, state.PrintContext));
	}

	// write to binary file?
	if (state.BinaryFileOut) {
		Utilities::Path::EnsureParentExists(state.OutputBinaryFile);
		debugger->AddHandler(std::make_shared<Hindsight::Debugger::EventHandler::WriterDebuggerEventHandler>(state.OutputBinaryFile));
	}

	if (!debugger->Attach()) {
		auto lastError = GetLastError();
		std::cout << rang::fgB::red << "error: cannot attach debugger (" << lastError << "), " << Hindsight::Utilities::Error::GetErrorMessage(lastError) << std::endl << rang::style::reset;
		pause(close_window);
		return 1;
	}

	if (state.PostMortemNotify) {
		auto image = fs::path(process->Path).filename().string();
		std::cout << rang::style::reset << "You were running " << rang::fgB::cyan << image << rang::style::reset
			<< " with PID " << rang::fgB::green << process->dwProcessId << rang::style::reset << "," << std::endl
			<< "but this process has crashed in a way that could not be recovered." << std::endl << std::endl;

		std::cout << "program path: " << rang::fgB::green << process->Path << rang::style::reset << std::endl << std::endl;

		std::cout << "hindsight, the debugger that you are seeing right now, has" << std::endl
			<< "placed information about this crash in one or more files on your device:" << std::endl << std::endl;

		if (state.TextFileOut)
			std::cout << " - " << rang::fgB::green << state.OutputTextFile << rang::style::reset << std::endl;
		if (state.BinaryFileOut)
			std::cout << " - " << rang::fgB::green << state.OutputBinaryFile << rang::style::reset << std::endl;

		std::cout << std::endl
			<< "You can view these files yourself, or send them unmodified to your " << std::endl
			<< "systems administrator for further inspection." << std::endl << std::endl;

		ShowWindow(hWndConsole, SW_SHOW);
		pause(close_window);
	}

	return 0;
}

/// <summary>
/// Generate the `hindsight [options] launch [options] subcommand`.
/// </summary>
/// <param name="app">The <see cref="CLI::App"/> instance that represents the parent command for this subcommand.</param>
/// <param name="state">The state obtained through processing program arguments through <see cref="CLI::App"/>.</param>
/// <param name="print_context">A reference to a <see cref="CLI::Option"/> pointer that will receive the print_context flag.</param>
/// <param name="print_timestamp">A reference to a <see cref="CLI::Option"/> pointer that will receive the print_timestamp flag.</param>
/// <returns>A pointer to the resulting <see cref="CLI:App"/> instance for this subcommand.</returns>
CLI::App* create_launch_command(CLI::App& app, State& state, CLI::Option*& print_context, CLI::Option*& print_timestamp) {
	// the launch command
	auto launch_command = app.add_subcommand("launch", "Launch an application, suspend it and attach this debugger to it");

	// launch -w "working directory"
	auto workdir = launch_command->add_option("-w,--working-directory", state.WorkingDirectory, "The working directory for the program to start")
		->required(false)
		->check(CLI::ExistingDirectory);

	// launch -b 
	auto break_on_breakpoint = launch_command->add_flag("-b,--break-breakpoint", state.BreakOnBreakpoints, "Break on breakpoints")
		->required(false);

	// launch -e
	auto break_on_exceptions = launch_command->add_flag("-e,--break-exception", state.BreakOnExceptions, "Break on exceptions")
		->required(false);

	// launch -f
	auto break_on_first_chance = launch_command->add_flag("-f,--first-chance", state.BreakOnFirstChanceOnly, "Only break on first-chance exceptions")
		->needs(break_on_exceptions)
		->required(false);

	// launch -r #num
	auto max_recursion = launch_command->add_option("-r,--max-recursion", state.MaxRecursion, "Set the maximum number of recursive frames in a stack trace. Use 0 to set to unlimited")
		->default_val("10")
		->required(false);

	// launch -i #num 
	auto max_instruction = launch_command->add_option("-i,--max-instruction", state.MaxInstruction, "Set the maximum number of instructions to include in a stack trace. Use 0 to disable")
		->default_val("0")
		->required(false);

	// launch -c 
	print_context = launch_command->add_flag("-c,--print-context", state.PrintContext, "Print the CPU context when a stack trace is printed for the textual output modes")
		->required(false);

	// launch -t
	print_timestamp = launch_command->add_flag("-t,--print-timestamp", state.PrintTimestamp, "Print a timestamp in front of each entry for the textual output modes")
		->required(false);

	// launch -s
	auto search_path = launch_command->add_option("-s,--pdb-search-path", state.PdbSearchPath, "Set one or multiple search paths for PDB files")
		->required(false)
		->check(CLI::ExistingDirectory);

	// launch -S
	auto search_path_auto = launch_command->add_flag("-S,--self-search-path", state.PdbSearchSelf, "Add the module path as search path for PDB files")
		->required(false);

	// launch [program path]
	auto path_option = launch_command->add_option("program", state.ProgramPath, "The path to the application to start and debug")
		->required(true)
		->check(CLI::ExistingFile);

	// launch ... [arguments]
	auto args_option = launch_command->add_option("arguments", state.Arguments, "The program parameters")
		->required(false);

	return launch_command;
}

/// <summary>
/// Generate the `hindsight [options] replay [options] subcommand`.
/// </summary>
/// <param name="app">The <see cref="CLI::App"/> instance that represents the parent command for this subcommand.</param>
/// <param name="state">The state obtained through processing program arguments through <see cref="CLI::App"/>.</param>
/// <param name="print_context">A reference to a <see cref="CLI::Option"/> pointer that will receive the replay_print_context flag.</param>
/// <param name="print_timestamp">A reference to a <see cref="CLI::Option"/> pointer that will receive the replay_print_timestamp flag.</param>
/// <returns>A pointer to the resulting <see cref="CLI:App"/> instance for this subcommand.</returns>
CLI::App* create_replay_command(CLI::App& app, State& state, CLI::Option*& replay_print_context, CLI::Option*& replay_print_timestamp) {
	// the replay command
	auto replay_command = app.add_subcommand("replay", "Replay a previously recorded binary log file");

	// replay -b 
	auto replay_break_on_breakpoint = replay_command->add_flag("-b,--break-breakpoint", state.BreakOnBreakpoints, "Break on breakpoints")
		->required(false);

	// replay -e
	auto replay_break_on_exceptions = replay_command->add_flag("-e,--break-exception", state.BreakOnExceptions, "Break on exceptions")
		->required(false);

	// replay -f
	auto replay_break_on_first_chance = replay_command->add_flag("-f,--first-chance", state.BreakOnFirstChanceOnly, "Only break on first-chance exceptions")
		->needs(replay_break_on_exceptions)
		->required(false);

	// replay -c 
	replay_print_context = replay_command->add_flag("-c,--print-context", state.PrintContext, "Print the CPU context when a stack trace is printed for the textual output modes")
		->required(false);

	// replay -t
	replay_print_timestamp = replay_command->add_flag("-t,--print-timestamp", state.PrintTimestamp, "Print a timestamp in front of each entry for the textual output modes")
		->required(false);

	auto replay_include = replay_command->add_option(
		"-i,--include-only",
		state.ReplayEventFilter,
		"Specify a collection of events to include in the replay, options: " + Hindsight::CliValidator::EventFilterValidator::GetValid())
		->required(false)
		->check(Hindsight::CliValidator::EventFilterValidator::Validator);

	// replay --no-sanity-check
	auto replay_no_sanity_check = replay_command->add_flag("--no-sanity-check", state.NoSanityCheck, "Do not verify the checksum of the event data in the file")
		->required(false);

	// replay [binary log file path]
	auto replay_path_option = replay_command->add_option("path", state.ReplayFile, "The path to the binary log file to replay")
		->required(true)
		->check(CLI::ExistingFile);

	return replay_command;
}

/// <summary>
/// Generate the `hindsight [options] mortem [options] subcommand`.
/// </summary>
/// <param name="app">The <see cref="CLI::App"/> instance that represents the parent command for this subcommand.</param>
/// <param name="state">The state obtained through processing program arguments through <see cref="CLI::App"/>.</param>
/// <param name="print_context">A reference to a <see cref="CLI::Option"/> pointer that will receive the mortem_print_context flag.</param>
/// <param name="print_timestamp">A reference to a <see cref="CLI::Option"/> pointer that will receive the mortem_print_timestamp flag.</param>
/// <returns>A pointer to the resulting <see cref="CLI:App"/> instance for this subcommand.</returns>
CLI::App* create_mortem_command(CLI::App& app, State& state, CLI::Option*& mortem_print_context, CLI::Option*& mortem_print_timestamp) {
	auto mortem_command = app.add_subcommand("mortem", "The postmortem debugger, which can be registered and automatically invoked by the system");

	// mortem -c 
	mortem_print_context = mortem_command->add_flag("-c,--print-context", state.PrintContext, "Print the CPU context when a stack trace is printed for the textual output modes")
		->required(false);

	// mortem -t
	mortem_print_timestamp = mortem_command->add_flag("-t,--print-timestamp", state.PrintTimestamp, "Print a timestamp in front of each entry for the textual output modes")
		->required(false);

	// launch -r #num
	auto max_recursion = mortem_command->add_option("-r,--max-recursion", state.MaxRecursion, "Set the maximum number of recursive frames in a stack trace. Use 0 to set to unlimited")
		->default_val("10")
		->required(false);

	// launch -i #num 
	auto max_instruction = mortem_command->add_option("-i,--max-instruction", state.MaxInstruction, "Set the maximum number of instructions to include in a stack trace. Use 0 to disable")
		->default_val("0")
		->required(false);

	// mortem -s
	auto search_path = mortem_command->add_option("-s,--pdb-search-path", state.PdbSearchPath, "Set one or multiple search paths for PDB files")
		->required(false)
		->check(CLI::ExistingDirectory);

	// mortem -S
	auto search_path_auto = mortem_command->add_flag("-S,--self-search-path", state.PdbSearchSelf, "Add the module path as search path for PDB files")
		->required(false);

	// mortem -p
	auto process_id = mortem_command->add_option("-p,--process-id", state.PostMortemProcessId, "The post-mortem target process ID")
		->required(true);

	// mortem -e
	auto event_handle = mortem_command->add_option("-e,--event-handle", state.PostMortemEvent, "The post-mortem debug event handle")
		->required(true);

	// mortem -j
	auto jit_debug = mortem_command->add_option("-j,--jit-debug-info", state.PostMortemJitDebugEventInfo, "The post-mortem JIT_DEBUG_INFO structure reference")
		->required(true);

	// mortem -n
	auto notify = mortem_command->add_flag("-n,--notify", state.PostMortemNotify, "Notify the user after hindsight is ready processing the postmortem debug event")
		->required(false);
	
	return mortem_command;
}

/// <summary>
/// The main entrypoint
/// </summary>
/// <param name="argc">Command line argument count.</param>
/// <param name="argv">Command line arguments.</param>
/// <returns>Process return code.</returns>
int main(int argc, char* argv[]) {
	// Set the console window icon, for when hindsight has its own console.
	auto hWndConsole = GetConsoleWindow();
	auto hIcon		 = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_ICON1));

	SendMessage(hWndConsole, WM_SETICON, ICON_SMALL,  reinterpret_cast<LPARAM>(hIcon));
	SendMessage(hWndConsole, WM_SETICON, ICON_BIG,    reinterpret_cast<LPARAM>(hIcon));

	State state;
	CLI::Option *print_context, *print_timestamp, *replay_print_context, *replay_print_timestamp, *mortem_print_context, *mortem_print_timestamp;

	CLI::App app("A portable hindsight debugger that is designed for detecting issues in software when it is already published.", "hindsight");
	app.require_subcommand();
	app.set_help_all_flag("-H,--help-all", "Show help for all subcommands");
	app.footer("note: use _NT_SYMBOL_PATH and _NT_ALT_SYMBOL_PATH environment variables to override default search paths for .pdb files.\r\n      launch --pdb-search-path can also be used to add multiple directories");

	// add the PrintingDebuggerEventHandler for stdout?
	auto standard_out = app.add_flag("-s,--stdout", state.StandardOut, "Indicate that the debugger should output to stdout")
		->required(false);

	// add the PrintingDebuggerEventHandler for a file?
	auto text_out = app.add_option("-l,--log", state.OutputTextFile, "Indicate that the debugger should output to log file")
		->required(false);

	// add the WritingDebuggerEventHandler
	auto bin_out = app.add_option("-w,--write-binary", state.OutputBinaryFile, "Indicate that the debugger should output to binary log file")
		->required(false);

	// disable colours
	auto bland = app.add_flag("-b,--bland", state.Bland, "Disable colours in terminal output when --stdout was specified")
		->needs(standard_out)
		->required(false);

	auto launch_command = create_launch_command(app, state, print_context, print_timestamp);
	auto replay_command = create_replay_command(app, state, replay_print_context, replay_print_timestamp);
	auto mortem_command = create_mortem_command(app, state, mortem_print_context, mortem_print_timestamp);

	// hindsight --version
	auto version_flag = app.add_flag("-v,--version", [&](size_t count) {
		std::cout << rang::style::reset << "hindsight " << get_version() << " " << hindsight_version_year_s << ", " << hindsight_author << std::endl;

		if (Hindsight::_contributors.size() > 1) 
			std::wcout << L"contributors: " << Utilities::String::Join(Hindsight::_contributors, L", ") << std::endl;

		exit(0);
	}, "Display the version of hindsight");

	// make sure to reset the rang style on exit
	std::atexit([]() {
		std::cout << rang::style::reset;
	});

	try {
		app.parse(argc, argv);
	} catch (const CLI::ParseError & e) {
		std::cout << (e.get_exit_code() == 0 ? rang::fg::gray : rang::fg::red);
		auto code = app.exit(e);
		std::cout << rang::style::reset;
		return code;
	}

	state.TextFileOut   = *text_out;
	state.BinaryFileOut = *bin_out;

	auto textual_output = (*text_out || *standard_out);

	// ensure the --print-context has a required option to specify where to print to
	if ((*print_context || *replay_print_context || *mortem_print_context) && !textual_output) {
		std::cout << rang::fgB::red << "error: cannot use --print-context without either --stdout or --log" << std::endl << rang::style::reset;
		return 1;
	}

	// ensure the --print-timestamp has a required option to specify where to print to 
	if ((*print_timestamp || *replay_print_timestamp || *mortem_print_timestamp) && !textual_output) {
		std::cout << rang::fgB::red << "error: cannot use --print-timestamp without either --stdout or --log" << std::endl << rang::style::reset;
		return 1;
	}

	if (launch_command->parsed()) 
		return LaunchCommand(state);

	if (replay_command->parsed())
		return ReplayCommand(state);

	if (mortem_command->parsed()) {
		if (*standard_out) {
			std::cout << rang::fgB::red << "error: cannot use --stdout in the post-mortem debug mode" << std::endl << rang::style::reset;
			pause(close_window);
			return 1;
		} else if (!*text_out && !*bin_out) {
			std::cout << rang::fgB::red << "error: cannot use the mortem subcommand without a file-based output handler (such as -l or -w)" << std::endl << rang::style::reset;
			pause(close_window);
			return 1;
		}

		return MortemCommand(state);
	}

	return 0;
}