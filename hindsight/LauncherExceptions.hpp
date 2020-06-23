#pragma once

#ifndef exceptions_launcher_h
#define exceptions_launcher_h
	#include <stdexcept>
	#include <string>
	#include <Windows.h>
	#include "Error.hpp"

	namespace Hindsight {
		namespace Exceptions {
			/// <summary>
			/// A <see cref="::std::runtime_error"/> derived exception that is thrown when an application cannot be launched.
			/// </summary>
			class LauncherFailedException : public std::runtime_error {
				public:
					/// <summary>
					/// Construct a new LauncherFailedException instance based on a system error code (from <see cref="GetLastError"/>).
					/// </summary>
					/// <param name="errorCode">The system error code.</param>
					LauncherFailedException(DWORD errorCode) 
						: std::runtime_error(std::string("application could not be launched (" + std::to_string(errorCode) + "): " + Utilities::Error::GetErrorMessage(errorCode))) {

					}
			};

		}
	}

#endif 