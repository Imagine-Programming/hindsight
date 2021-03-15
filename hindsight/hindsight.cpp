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
#include "DynaCli.hpp"
#include "ArgumentNames.hpp"
#include "rang.hpp"
#include "Version.hpp"

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
void PreProcessState(Cli::HindsightCli& cli, std::shared_ptr<Hindsight::Process::Process> process) {
	auto t  =  std::time(nullptr);
	tm time;
	localtime_s(&time, &t); 

	auto image = fs::path(process->Path).filename().string();

	if (cli.isset(Cli::Descriptors::NAME_LOGBIN))
		PreProcessPath(cli.get<std::string>(Cli::Descriptors::NAME_LOGBIN), time, image);
	if (cli.isset(Cli::Descriptors::NAME_LOGTEXT))
		PreProcessPath(cli.get<std::string>(Cli::Descriptors::NAME_LOGTEXT), time, image);
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
int LaunchCommand(Cli::HindsightCli& cli) {
	std::shared_ptr<Hindsight::Process::Process>   process;
	std::shared_ptr<Hindsight::Debugger::Debugger> debugger;

	auto& command = cli[cli.get_chosen_subcommand_name()];

	if (!command.isset(Cli::Descriptors::NAME_MAX_RECURSION) || command.get<size_t>(Cli::Descriptors::NAME_MAX_RECURSION) == 0)
		command.set(Cli::Descriptors::NAME_MAX_RECURSION, static_cast<size_t>(SIZE_MAX));

	try {
		process = Hindsight::Process::Launcher::StartSuspended(
			Hindsight::Utilities::Path::Absolute(command.get<std::string>(Cli::Descriptors::NAME_PROGPATH)),
			Hindsight::Utilities::Path::Absolute(command.get<std::string>(Cli::Descriptors::NAME_WORKDIR)),
			command.get<std::vector<std::string>>(Cli::Descriptors::NAME_ARGUMENTS));

		PreProcessState(cli, process);
	} catch (const Hindsight::Exceptions::LauncherFailedException& e) {
		std::cout << rang::fgB::red << "error: " << e.what() << std::endl << rang::style::reset;
		return 1;
	}

	// attach debugger
	try {
		debugger = std::make_shared<Hindsight::Debugger::Debugger>(process, cli);
	} catch (const Hindsight::Exceptions::ProcessNotRunningException & e) {
		std::cout << rang::fgB::red << "error: " << e.what() << std::endl << rang::style::reset;
		return 1;
	}

	// write to stdout?
	if (cli.isset(Cli::Descriptors::NAME_STDOUT)) 
		debugger->AddHandler(std::make_shared<Hindsight::Debugger::EventHandler::PrintingDebuggerEventHandler>(
			!cli.isset(Cli::Descriptors::NAME_BLAND), 
			command.isset(Cli::Descriptors::NAME_PRINTTIME), 
			command.isset(Cli::Descriptors::NAME_PRINTCTX)));
	
	// write to text file?
	if (cli.isset(Cli::Descriptors::NAME_LOGTEXT)) {
		Utilities::Path::EnsureParentExists(cli.get<std::string>(Cli::Descriptors::NAME_LOGTEXT));
		debugger->AddHandler(std::make_shared<Hindsight::Debugger::EventHandler::PrintingDebuggerEventHandler>(
			cli.get<std::string>(Cli::Descriptors::NAME_LOGTEXT), 
			command.isset(Cli::Descriptors::NAME_PRINTCTX)));
	}

	// write to binary file?
	if (cli.isset(Cli::Descriptors::NAME_LOGBIN)) {
		Utilities::Path::EnsureParentExists(cli.get<std::string>(Cli::Descriptors::NAME_LOGBIN));
		debugger->AddHandler(std::make_shared<Hindsight::Debugger::EventHandler::WriterDebuggerEventHandler>(cli.get<std::string>(Cli::Descriptors::NAME_LOGBIN)));
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
int ReplayCommand(Cli::HindsightCli& cli) {
	std::shared_ptr<Hindsight::BinaryLog::BinaryLogPlayer> player;

	auto& command = cli[cli.get_chosen_subcommand_name()];

	try {
		player = std::make_shared<Hindsight::BinaryLog::BinaryLogPlayer>(command.get<std::string>(Cli::Descriptors::NAME_BINPATH), cli);
	} catch (const std::exception & e) {
		std::cout << rang::fgB::red << "error: " << e.what() << std::endl << rang::style::reset;
		if (command.isset(Cli::Descriptors::NAME_PPAUSE))
			pause(continue_window);
		return 1;
	}

	// write to stdout?
	if (cli.isset(Cli::Descriptors::NAME_STDOUT)) 
		player->AddHandler(std::make_shared<Hindsight::Debugger::EventHandler::PrintingDebuggerEventHandler>(
			!cli.isset(Cli::Descriptors::NAME_BLAND),
			command.isset(Cli::Descriptors::NAME_PRINTTIME),
			command.isset(Cli::Descriptors::NAME_PRINTCTX)));

	// write to text file?
	if (cli.isset(Cli::Descriptors::NAME_LOGTEXT)) {
		Utilities::Path::EnsureParentExists(cli.get<std::string>(Cli::Descriptors::NAME_LOGTEXT));
		player->AddHandler(std::make_shared<Hindsight::Debugger::EventHandler::PrintingDebuggerEventHandler>(
			cli.get<std::string>(Cli::Descriptors::NAME_LOGTEXT),
			command.isset(Cli::Descriptors::NAME_PRINTCTX)));
	}

	// write to binary file?
	if (cli.isset(Cli::Descriptors::NAME_LOGBIN)) {
		Utilities::Path::EnsureParentExists(cli.get<std::string>(Cli::Descriptors::NAME_LOGBIN));
		player->AddHandler(std::make_shared<Hindsight::Debugger::EventHandler::WriterDebuggerEventHandler>(cli.get<std::string>(Cli::Descriptors::NAME_LOGBIN)));
	}

	try {
		player->Play();
	} catch (const std::exception& e) {
		std::cout << rang::fgB::red << "error: " << e.what() << std::endl << rang::style::reset;
		if (command.isset(Cli::Descriptors::NAME_PPAUSE))
			pause(continue_window);
		return 1;
	}

	if (command.isset(Cli::Descriptors::NAME_PPAUSE))
		pause(continue_window);
	
	return 0;
}

/// <summary>
/// Execute the hindsight [options] mortem [options] command.
/// </summary>
/// <param name="state">The state obtained through processing program arguments through <see cref="CLI::App"/>.</param>
/// <returns>The program exit code.</returns>
int MortemCommand(Cli::HindsightCli& cli) {
	HWND hWndConsole = GetConsoleWindow();
	ShowWindow(hWndConsole, SW_HIDE);

	auto& command = cli[cli.get_chosen_subcommand_name()];

	auto hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, command.get<DWORD>(Cli::Descriptors::NAME_JITPID));
	if (!hProcess) {
		std::cout << rang::fgB::red << "error: cannot open debuggee process with all access, " << Utilities::Error::GetErrorMessage(GetLastError()) << rang::style::reset << std::endl;
		return 1;
	}

	std::shared_ptr<Hindsight::Process::Process>   process;
	std::shared_ptr<Hindsight::Debugger::Debugger> debugger;

	if (!command.isset(Cli::Descriptors::NAME_MAX_RECURSION) || command.get<size_t>(Cli::Descriptors::NAME_MAX_RECURSION) == 0)
		command.set(Cli::Descriptors::NAME_MAX_RECURSION, static_cast<size_t>(SIZE_MAX));

	try {
		std::string path;
		std::string wdir;
		std::vector<std::string> args;

		PROCESS_INFORMATION pi = {
			hProcess,
			NULL,
			command.get<DWORD>(Cli::Descriptors::NAME_JITPID),
			NULL
		};
		process = std::make_shared<Hindsight::Process::Process>(pi, path, wdir, args);

		char imagePath[MAX_PATH + 1] = { 0 };
		GetModuleFileNameExA(process->hProcess, NULL, imagePath, MAX_PATH);
		//GetProcessImageFileNameA(process->hProcess, imagePath, MAX_PATH);
		process->Path = imagePath;
		process->WorkingDirectory = "";
		process->Arguments = {};

		PreProcessState(cli, process);
	} catch (const Hindsight::Exceptions::LauncherFailedException& e) {
		std::cout << rang::fgB::red << "error: " << e.what() << std::endl << rang::style::reset;
		return 1;
	}

	// attach debugger
	try {
		debugger = std::make_shared<Hindsight::Debugger::Debugger>(
			process, cli, 
			command.get<HANDLE>(Cli::Descriptors::NAME_JITEVENT), 
			command.get<void*>(Cli::Descriptors::NAME_JITINFO));

	} catch (const Hindsight::Exceptions::ProcessNotRunningException & e) {
		std::cout << rang::fgB::red << "error: " << e.what() << std::endl << rang::style::reset;
		return 1;
	}

	// write to text file?
	if (cli.isset(Cli::Descriptors::NAME_LOGTEXT)) {
		Utilities::Path::EnsureParentExists(cli.get<std::string>(Cli::Descriptors::NAME_LOGTEXT));
		debugger->AddHandler(std::make_shared<Hindsight::Debugger::EventHandler::PrintingDebuggerEventHandler>(
			cli.get<std::string>(Cli::Descriptors::NAME_LOGTEXT),
			command.isset(Cli::Descriptors::NAME_PRINTCTX)));
	}

	// write to binary file?
	if (cli.isset(Cli::Descriptors::NAME_LOGBIN)) {
		Utilities::Path::EnsureParentExists(cli.get<std::string>(Cli::Descriptors::NAME_LOGBIN));
		debugger->AddHandler(std::make_shared<Hindsight::Debugger::EventHandler::WriterDebuggerEventHandler>(cli.get<std::string>(Cli::Descriptors::NAME_LOGBIN)));
	}

	if (!debugger->Attach()) {
		auto lastError = GetLastError();
		std::cout << rang::fgB::red << "error: cannot attach debugger (" << lastError << "), " << Hindsight::Utilities::Error::GetErrorMessage(lastError) << std::endl << rang::style::reset;
		pause(close_window);
		return 1;
	}

	if (command.isset(Cli::Descriptors::NAME_JITNOTIFY)) {
		auto image = fs::path(process->Path).filename().string();
		std::cout << rang::style::reset << "You were running " << rang::fgB::cyan << image << rang::style::reset
			<< " with PID " << rang::fgB::green << process->dwProcessId << rang::style::reset << "," << std::endl
			<< "but this process has crashed in a way that could not be recovered." << std::endl << std::endl;

		std::cout << "program path: " << rang::fgB::green << process->Path << rang::style::reset << std::endl << std::endl;

		std::cout << "hindsight, the debugger that you are seeing right now, has" << std::endl
			<< "placed information about this crash in one or more files on your device:" << std::endl << std::endl;

		if (cli.isset(Cli::Descriptors::NAME_LOGTEXT))
			std::cout << " - " << rang::fgB::green << cli.get<std::string>(Cli::Descriptors::NAME_LOGTEXT) << rang::style::reset << std::endl;
		if (cli.isset(Cli::Descriptors::NAME_LOGBIN))
			std::cout << " - " << rang::fgB::green << cli.get<std::string>(Cli::Descriptors::NAME_LOGBIN) << rang::style::reset << std::endl;

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
void create_launch_command(Cli::HindsightCli& cli) {
	// the launch command, see ArgumentNames for descriptions on each flag and option.
	auto& command = cli.add_subcommand(Cli::Descriptors::NAME_SUBCOMMAND_LAUNCH, Cli::Descriptors::DESC_SUBCOMMAND_LAUNCH);

	// flags and options
	command.add_option<std::string>(Cli::Descriptors::DESC_WORKDIR)->check(CLI::ExistingDirectory);
	command.add_flag(Cli::Descriptors::DESC_BREAKB);
	command.add_flag(Cli::Descriptors::DESC_BREAKE);
	command.add_flag(Cli::Descriptors::DESC_BREAKF)->needs(command.get_option(Cli::Descriptors::NAME_BREAKE));
	command.add_option<size_t>(Cli::Descriptors::DESC_MAX_RECURSION)->default_val("0");
	command.add_option<size_t>(Cli::Descriptors::DESC_MAX_INSTRUCTION)->default_val("0");
	command.add_flag(Cli::Descriptors::DESC_PRINTCTX);
	command.add_flag(Cli::Descriptors::DESC_PRINTTIME);
	command.add_option<std::vector<std::string>>(Cli::Descriptors::DESC_PDBSEARCH)->check(CLI::ExistingDirectory);
	command.add_flag(Cli::Descriptors::DESC_PDBSELF);

	// positional arguments 
	command.add_option<std::string>(Cli::Descriptors::DESC_PROGPATH)->required(true)->check(CLI::ExistingFile);
	command.add_option<std::vector<std::string>>(Cli::Descriptors::DESC_ARGUMENTS);
}

/// <summary>
/// Generate the `hindsight [options] replay [options] subcommand`.
/// </summary>
/// <param name="app">The <see cref="CLI::App"/> instance that represents the parent command for this subcommand.</param>
/// <param name="state">The state obtained through processing program arguments through <see cref="CLI::App"/>.</param>
/// <param name="print_context">A reference to a <see cref="CLI::Option"/> pointer that will receive the replay_print_context flag.</param>
/// <param name="print_timestamp">A reference to a <see cref="CLI::Option"/> pointer that will receive the replay_print_timestamp flag.</param>
void create_replay_command(Cli::HindsightCli& cli) {
	// the replay command
	auto& command = cli.add_subcommand(Cli::Descriptors::NAME_SUBCOMMAND_REPLAY, Cli::Descriptors::DESC_SUBCOMMAND_REPLAY);

	// flags and options
	command.add_flag(Cli::Descriptors::DESC_BREAKB);
	command.add_flag(Cli::Descriptors::DESC_BREAKE);
	command.add_flag(Cli::Descriptors::DESC_BREAKF)->needs(command.get_option(Cli::Descriptors::NAME_BREAKE));
	command.add_flag(Cli::Descriptors::DESC_PRINTCTX);
	command.add_flag(Cli::Descriptors::DESC_PRINTTIME);

	// this one is initialized directly due to the description relying on the EventFilterValidator entries
	command.add_option<std::vector<std::string>>(
		Cli::Descriptors::DESC_FILTER.Name,
		Cli::Descriptors::DESC_FILTER.Flag,
		Cli::Descriptors::DESC_FILTER.Desc + std::string(", options: ") + Hindsight::Cli::CliValidator::EventFilterValidator::GetValid()
	)->check(Hindsight::Cli::CliValidator::EventFilterValidator::Validator);

	command.add_flag(Cli::Descriptors::DESC_NOSANITY);
	command.add_flag(Cli::Descriptors::DESC_PPAUSE);

	// positionals
	command.add_option<std::string>(Cli::Descriptors::DESC_BINPATH)->required(true)->check(CLI::ExistingFile);
}

/// <summary>
/// Generate the `hindsight [options] mortem [options] subcommand`.
/// </summary>
/// <param name="app">The <see cref="CLI::App"/> instance that represents the parent command for this subcommand.</param>
/// <param name="state">The state obtained through processing program arguments through <see cref="CLI::App"/>.</param>
/// <param name="print_context">A reference to a <see cref="CLI::Option"/> pointer that will receive the mortem_print_context flag.</param>
/// <param name="print_timestamp">A reference to a <see cref="CLI::Option"/> pointer that will receive the mortem_print_timestamp flag.</param>
void create_mortem_command(Cli::HindsightCli& cli) {
	auto& command = cli.add_subcommand(Cli::Descriptors::NAME_SUBCOMMAND_MORTEM, Cli::Descriptors::DESC_SUBCOMMAND_MORTEM);

	// flags and options
	command.add_flag(Cli::Descriptors::DESC_PRINTCTX);
	command.add_flag(Cli::Descriptors::DESC_PRINTTIME);
	command.add_option<size_t>(Cli::Descriptors::DESC_MAX_RECURSION)->default_val("0");
	command.add_option<size_t>(Cli::Descriptors::DESC_MAX_INSTRUCTION)->default_val("0");
	command.add_option<std::vector<std::string>>(Cli::Descriptors::DESC_PDBSEARCH)->check(CLI::ExistingDirectory);
	command.add_flag(Cli::Descriptors::DESC_PDBSELF);
	command.add_option<DWORD>(Cli::Descriptors::DESC_JITPID)->required(true);
	command.add_option<HANDLE>(Cli::Descriptors::DESC_JITEVENT)->required(true);
	command.add_option<void*>(Cli::Descriptors::DESC_JITINFO)->required(true);
	command.add_flag(Cli::Descriptors::DESC_JITNOTIFY);
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

	Cli::HindsightCli cli("A portable hindsight debugger that is designed for detecting issues in software when it is already published.", "hindsight");

	cli.command().require_subcommand();
	cli.command().set_help_all_flag("-H,--help-all", "Show help for all subcommands");
	cli.command().footer("note: use _NT_SYMBOL_PATH and _NT_ALT_SYMBOL_PATH environment variables to override default search paths for .pdb files.\r\n      launch --pdb-search-path can also be used to add multiple directories");

	// add the PrintingDebuggerEventHandler for stdout?
	cli.add_flag(Cli::Descriptors::DESC_STDOUT);

	// add the PrintingDebuggerEventHandler for a file?
	cli.add_option<std::string>(Cli::Descriptors::DESC_LOGTEXT);

	// add the WritingDebuggerEventHandler
	cli.add_option<std::string>(Cli::Descriptors::DESC_LOGBIN);

	// disable colours
	cli.add_flag(Cli::Descriptors::DESC_BLAND)
		->needs(cli.get_option(Cli::Descriptors::NAME_STDOUT));

	create_launch_command(cli);
	create_replay_command(cli);
	create_mortem_command(cli);

	// hindsight --version
	cli.add_flag(Cli::Descriptors::DESC_VERSION, [&](size_t count) {
		std::cout << rang::style::reset << "hindsight " << get_version() << " " << hindsight_version_year_s << ", " << hindsight_author << std::endl;

		if (Hindsight::_contributors.size() > 1)
			std::wcout << L"contributors: " << Utilities::String::Join(Hindsight::_contributors, L", ") << std::endl;

		exit(0);
	});

	// make sure to reset the rang style on exit
	std::atexit([]() {
		std::cout << rang::style::reset;
	});

	try {
		cli.command().parse(argc, argv);
	} catch (const CLI::ParseError & e) {
		std::cout << (e.get_exit_code() == 0 ? rang::fg::gray : rang::fg::red);
		auto code = cli.command().exit(e);
		std::cout << rang::style::reset;
		return code;
	}

	auto textual_output = cli.anyset({ Cli::Descriptors::NAME_LOGTEXT, Cli::Descriptors::NAME_STDOUT });

	// ensure the --print-context has a required option to specify where to print to
	if (!textual_output && cli.subcommand_anyset({ Cli::Descriptors::NAME_SUBCOMMAND_LAUNCH, Cli::Descriptors::NAME_SUBCOMMAND_REPLAY }, { Cli::Descriptors::NAME_PRINTCTX, Cli::Descriptors::NAME_PRINTTIME })) {
		std::cout << rang::fgB::red << "error: cannot use --print-context or --print-timestamp without either --stdout or --log" << std::endl << rang::style::reset;
		return 1;
	}

	if (cli.is_subcommand_chosen(Cli::Descriptors::NAME_SUBCOMMAND_LAUNCH)) 
		return LaunchCommand(cli);

	if (cli.is_subcommand_chosen(Cli::Descriptors::NAME_SUBCOMMAND_REPLAY))
		return ReplayCommand(cli);

	if (cli.is_subcommand_chosen(Cli::Descriptors::NAME_SUBCOMMAND_MORTEM)) {
		if (cli.isset(Cli::Descriptors::NAME_STDOUT)) {
			std::cout << rang::fgB::red << "error: cannot use --stdout in the post-mortem debug mode" << std::endl << rang::style::reset;
			pause(close_window);
			return 1;
		} else if (!cli.anyset({ Cli::Descriptors::NAME_LOGTEXT, Cli::Descriptors::NAME_LOGBIN })) {
			std::cout << rang::fgB::red << "error: cannot use the mortem subcommand without a file-based output handler (such as -l or -w)" << std::endl << rang::style::reset;
			pause(close_window);
			return 1;
		}

		return MortemCommand(cli);
	}

	return 0;
}