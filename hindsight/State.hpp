#pragma once

#ifndef state_h
#define state_h

	#include <string>
	#include <vector>
	#include <Windows.h>

	namespace Hindsight {
		/// <summary>
		/// The State class represents the program argument state.
		/// </summary>
		struct State {
			std::string					ProgramPath;		/* launch [program path] */
			std::string					WorkingDirectory;	/* launch -w [working directory] */
			std::vector<std::string>	Arguments;			/* launch [program path] [...args] */
			
			bool StandardOut = false;						/* hindsight -s [path] */
			bool TextFileOut = false;						/* hindsight -l [path] */
			std::string OutputTextFile;						/* path to log file for hindsight -l */
			bool BinaryFileOut = false;						/* hindsight -w */
			std::string OutputBinaryFile;					/* path to binary log file for hindsight -w */
			bool Bland = false;								/* hindsight -b */
			bool PrintContext = false;						/* hindsight -[sl] [launch|replay] -c */
			bool PrintTimestamp = false;					/* hindsight -[sl] [launch|replay] -t */
			bool BreakOnBreakpoints = false;				/* hindsight [launch|replay] -b */
			bool BreakOnExceptions = false;					/* hindsight [launch|replay] -e */
			bool BreakOnFirstChanceOnly = false;			/* hindsight [launch|replay] -[be]f */
			bool NoSanityCheck = false;						/* hindsight replay --no-sanity-check */

			size_t MaxRecursion = 10;						/* hindsight [launch|mortem] -r[number] */
			size_t MaxInstruction = 0;						/* hindsight [launch|mortem] -i[number] */

			std::string ReplayFile;							/* hindsight replay [binary log path] */
			std::vector<std::string> ReplayEventFilter;		/* hindsight replay -i [name] -i [name] -i.... */

			std::vector<std::string> PdbSearchPath;			/* hindsight [launch|mortem] -s [path] */
			bool PdbSearchSelf;								/* hindsight [launch|mortem] -S */

			DWORD		PostMortemProcessId;				/* hindsight mortem -p */
			HANDLE		PostMortemEvent;					/* hindsight mortem -e */
			void*		PostMortemJitDebugEventInfo;		/* hindsight mortem -j */
			bool		PostMortemNotify;					/* hindsight mortem -n */
		};
	}

#endif 