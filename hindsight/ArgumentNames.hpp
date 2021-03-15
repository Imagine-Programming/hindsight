#pragma once

#ifndef argument_names_h
#define argument_names_h
	#include "DynaCli.hpp"
	#include <Windows.h>
	#include <string>
	#include <vector>

	namespace Hindsight {
		namespace Cli {
			namespace Descriptors {
				// hindsight [opts] launch [opts]
				static constexpr auto NAME_SUBCOMMAND_LAUNCH = "launch";
				static constexpr auto DESC_SUBCOMMAND_LAUNCH = "Launch an application, suspend it and attach this debugger to it";

				// hindsight [opts] replay [opts]
				static constexpr auto NAME_SUBCOMMAND_REPLAY = "replay";
				static constexpr auto DESC_SUBCOMMAND_REPLAY = "Replay a previously recorded binary log file";

				// hindsight [opts] mortem [opts]
				static constexpr auto NAME_SUBCOMMAND_MORTEM = "mortem";
				static constexpr auto DESC_SUBCOMMAND_MORTEM = "The postmortem debugger, which can be registered and automatically invoked by the system";

				// hindsight --stdout [opts] [launch|replay] [opts]
				static constexpr auto NAME_STDOUT = "stdout";
				static constexpr const OptionDescriptor DESC_STDOUT(NAME_STDOUT, "-s,--stdout", "Indicate that the debugger should output to stdout");

				// hindsight --log [opts] [launch|replay|mortem] [opts]
				static constexpr auto NAME_LOGTEXT = "logtext";
				static constexpr const OptionDescriptor DESC_LOGTEXT(NAME_LOGTEXT, "-l,--log", "Indicate that the debugger should output to log file");

				// hindsight --write-binary [opts] [launch|replay|mortem] [opts]
				static constexpr auto NAME_LOGBIN = "logbinary";
				static constexpr const OptionDescriptor DESC_LOGBIN(NAME_LOGBIN, "-w,--write-binary", "Indicate that the debugger should output to binary log file");

				// hindsight --bland [opts] [subcommand] [opts]
				static constexpr auto NAME_BLAND = "bland";
				static constexpr const OptionDescriptor DESC_BLAND(NAME_BLAND, "-b,--bland", "Disable colours in terminal output when --stdout was specified");

				// hindsight --version
				static constexpr auto NAME_VERSION = "version";
				static constexpr const OptionDescriptor DESC_VERSION(NAME_VERSION, "-v,--version", "Display the version of hindsight");

				// hindsight [opts] launch --working-directory [opts]
				static constexpr auto NAME_WORKDIR = "workdir";
				static constexpr const OptionDescriptor DESC_WORKDIR(NAME_WORKDIR, "-w,--working-directory", "The working directory for the program to start");

				// hindsight [opts] [launch|replay] --break-breakpoint [opts]
				static constexpr auto NAME_BREAKB = "breakb"; 
				static constexpr const OptionDescriptor DESC_BREAKB(NAME_BREAKB, "-b,--break-breakpoint", "Break on breakpoints");

				// hindsight [opts] [launch|replay] --break-exception [opts]
				static constexpr auto NAME_BREAKE = "breake";
				static constexpr const OptionDescriptor DESC_BREAKE(NAME_BREAKE, "-e,--break-exception", "Break on exceptions");

				// hindsight [opts] [launch|replay] --first-chance [opts]
				static constexpr auto NAME_BREAKF = "breakf";
				static constexpr const OptionDescriptor DESC_BREAKF(NAME_BREAKF, "-f,--first-chance", "Only break on first-chance exceptions");

				// hindsight [opts] [launch|mortem] --max-recursion [opts]
				static constexpr auto NAME_MAX_RECURSION = "maxrecursion";
				static constexpr const OptionDescriptor DESC_MAX_RECURSION(NAME_MAX_RECURSION, "-r,--max-recursion", "Set the maximum number of recursive frames in a stack trace. Use 0 to set to unlimited");

				// hindsight [opts] [launch|mortem] --max-instruction [opts]
				static constexpr auto NAME_MAX_INSTRUCTION = "maxinstruction";
				static constexpr const OptionDescriptor DESC_MAX_INSTRUCTION(NAME_MAX_INSTRUCTION, "-i,--max-instruction", "Set the maximum number of instructions to include in a stack trace. Use 0 to disable");

				// hindsight [opts] [launch|replay] --print--context [opts]
				static constexpr auto NAME_PRINTCTX = "printctx";
				static constexpr const OptionDescriptor DESC_PRINTCTX(NAME_PRINTCTX, "-c,--print-context", "Print the CPU context when a stack trace is printed for the textual output modes");

				// hindsight [opts] [launch|replay] --print--timestamp [opts]
				static constexpr auto NAME_PRINTTIME = "printtime";
				static constexpr const OptionDescriptor DESC_PRINTTIME(NAME_PRINTTIME, "-t,--print-timestamp", "Print a timestamp in front of each entry for the textual output modes");

				// hindsight [opts] [launch|mortem] --pdb-search-path... [opts]
				static constexpr auto NAME_PDBSEARCH = "pdbsearch";
				static constexpr const OptionDescriptor DESC_PDBSEARCH(NAME_PDBSEARCH, "-s,--pdb-search-path", "Set one or multiple search paths for PDB files");

				// hindsight [opts] [launch|mortem] --self-search-path [opts]
				static constexpr auto NAME_PDBSELF = "pdbself";
				static constexpr const OptionDescriptor DESC_PDBSELF(NAME_PDBSELF, "-S,--self-search-path", "Add the module path as search path for PDB files");

				// hindsight [opts] replay [opts] --filter... [file]
				static constexpr auto NAME_FILTER = "filter";
				static constexpr const OptionDescriptor DESC_FILTER(NAME_FILTER, "-i,--include-only", "Specify a collection of events to include in the replay");

				// hindsight [opts] replay [opts] --no-sanity-check [file]
				static constexpr auto NAME_NOSANITY = "nosanity";
				static constexpr const OptionDescriptor DESC_NOSANITY(NAME_NOSANITY, "--no-sanity-check", "Do not verify the checksum of the event data in the file");

				// hindsight [opts] mortem [opts] --process-id
				static constexpr auto NAME_JITPID = "jitpid";
				static constexpr const OptionDescriptor DESC_JITPID(NAME_JITPID, "-p,--process-id", "The post-mortem target process ID");

				// hindsight [opts] mortem [opts] --event-handle
				static constexpr auto NAME_JITEVENT = "jitevent";
				static constexpr const OptionDescriptor DESC_JITEVENT(NAME_JITEVENT, "-e,--event-handle", "The post-mortem debug event handle");

				// hindsight [opts] mortem [opts] --jit-debug-info
				static constexpr auto NAME_JITINFO = "jitinfo";
				static constexpr const OptionDescriptor DESC_JITINFO(NAME_JITINFO, "-j,--jit-debug-info", "The post-mortem JIT_DEBUG_INFO structure reference");

				// hindsight [opts] mortem [opts] --notify
				static constexpr auto NAME_JITNOTIFY = "jitnotify";
				static constexpr const OptionDescriptor DESC_JITNOTIFY(NAME_JITNOTIFY, "-n,--notify", "Notify the user after hindsight is ready processing the postmortem debug event");

				// hindsight [opts] launch [opts] path
				static constexpr auto NAME_PROGPATH = "progpath";
				static constexpr const OptionDescriptor DESC_PROGPATH(NAME_PROGPATH, "program", "The path to the application to start and debug");

				// hindsight [opts] replay [opts] path
				static constexpr auto NAME_BINPATH = "binpath";
				static constexpr const OptionDescriptor DESC_BINPATH(NAME_BINPATH, "path", "The path to the binary log file to replay");

				// hindsight [opts] launch [opts] [path] arguments...
				static constexpr auto NAME_ARGUMENTS = "arguments";
				static constexpr const OptionDescriptor DESC_ARGUMENTS(NAME_ARGUMENTS, "arguments", "The program parameters");
			}

			// Define the types that are supported in this implementation of DynaCli
			using HindsightCli = Cli::DynaCli<
				bool, size_t, DWORD, void*,
				std::string, std::vector<std::string>
			>;
		}
	}

#endif 