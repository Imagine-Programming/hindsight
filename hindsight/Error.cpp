#include "Error.hpp"

using namespace Hindsight::Utilities;

/// <summary>
/// Get the error message for a certain system error code.
/// </summary>
/// <param name="error">The system error code to fetch a message for.</param>
/// <returns>The error message or an empty string when something went wrong.</returns>
std::string Error::GetErrorMessage(DWORD error) {
	if (error == 0)
		return "";

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&messageBuffer), 0, NULL);

	std::string message(messageBuffer, size);

	//Free the buffer.
	LocalFree(messageBuffer);

	return message;
}

/// <summary>
/// Get the unicode error message for a certain system error code.
/// </summary>
/// <param name="error">The system error code to fetch a message for.</param>
/// <returns>The error message or an empty string when something went wrong.</returns>
std::wstring Error::GetErrorMessageW(DWORD error) {
	if (error == 0)
		return L"";

	LPWSTR messageBuffer = nullptr;
	size_t size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&messageBuffer), 0, NULL);

	std::wstring message(messageBuffer, size);

	//Free the buffer.
	LocalFree(messageBuffer);

	return message;
}