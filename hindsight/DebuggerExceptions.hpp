#pragma once

#ifndef exceptions_debugger_h
#define exceptions_debugger_h

	#include <stdexcept>
	#include "Error.hpp"

	namespace Hindsight {
		namespace Exceptions {
			/// <summary>
			/// A <see cref="::std::runtime_error"/> derived exception that is thrown when the process to be debugged is no longer running.
			/// </summary>
			class ProcessNotRunningException : public std::runtime_error {
				public:
					/// <summary>
					/// Construct a new ProcessNotRunningException instance.
					/// </summary>
					ProcessNotRunningException() : std::runtime_error("process is no longer running, cannot attach debugger") {

					}
			};

			/// <summary>
			/// A <see cref="::std::runtime_error"/> derived exception that is thrown when memory in the remote process that is being 
			/// debugged cannot be read.
			/// </summary>
			class RemoteReadException : public std::runtime_error {
				public:
					/// <summary>
					/// Construct a new RemoteReadException instance based on a system error code (from <see cref="GetLastError"/>).
					/// </summary>
					/// <param name="error">The system error code.</param>
					RemoteReadException(DWORD error) : std::runtime_error("cannot read memory in debuggee process: " + Utilities::Error::GetErrorMessage(error)) {

					}
			};
		}
	}

#endif 