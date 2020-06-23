#pragma once

#ifndef error_h
#define error_h
	#include <string>
	#include <Windows.h>

	namespace Hindsight {
		namespace Utilities {
			/// <summary>
			/// Simple utility functions for system errors.
			/// </summary>
			class Error {
				public:
					/// <summary>
					/// Get the error message for a certain system error code.
					/// </summary>
					/// <param name="error">The system error code to fetch a message for.</param>
					/// <returns>The error message or an empty string when something went wrong.</returns>
					static std::string GetErrorMessage(DWORD error);

					/// <summary>
					/// Get the unicode error message for a certain system error code.
					/// </summary>
					/// <param name="error">The system error code to fetch a message for.</param>
					/// <returns>The error message or an empty string when something went wrong.</returns>
					static std::wstring GetErrorMessageW(DWORD error);
			};

		}
	}

#endif 