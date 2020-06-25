#include "String.hpp"
#include <numeric>
#include <stdexcept>
#include <algorithm>
#include <string>
#include <cwctype>
#include <Windows.h>

using namespace Hindsight::Utilities;

/// <summary>
/// Replaces the first occurrence of <paramref name="from"/> to <paramref name="to"/> in <paramref name="str"/> and produce a new string.
/// </summary>
/// <param name="str">The string to search in.</param>
/// <param name="from">The string to find.</param>
/// <param name="to">The string to replace the found occurrence with.</param>
/// <returns>The resulting string.</returns>
std::string String::Replace(const std::string& str, const std::string& from, const std::string& to) {
	auto result    = str;
	auto start_pos = result.find(from);

	if (start_pos == std::string::npos)
		return result;

	result.replace(start_pos, from.length(), to);
	return result;
}

/// <summary>
/// Replaces all occurrences of <paramref name="from"/> to <paramref name="to"/> in <paramref name="str"/> and produce a new string.
/// </summary>
/// <param name="str">The string to search in.</param>
/// <param name="from">The string to find.</param>
/// <param name="to">The string to replace the found occurrence with.</param>
/// <returns>The resulting string.</returns>
std::string String::ReplaceAll(const std::string& str, const std::string& from, const std::string& to) {
	if (from.empty())
		return str;

	auto result    = str;
	auto start_pos = (size_t)0;

	while ((start_pos = result.find(from, start_pos)) != std::string::npos) {
		result.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}

	return result;
}

/// <summary>
/// Replaces the first occurrence of <paramref name="from"/> to <paramref name="to"/> in <paramref name="str"/> and produce a new string.
/// </summary>
/// <param name="str">The string to search in.</param>
/// <param name="from">The string to find.</param>
/// <param name="to">The string to replace the found occurrence with.</param>
/// <returns>The resulting string.</returns>
std::wstring String::Replace(const std::wstring& str, const std::wstring& from, const std::wstring& to) {
	auto result    = str;
	auto start_pos = result.find(from);

	if (start_pos == std::wstring::npos)
		return result;

	result.replace(start_pos, from.length(), to);
	return result;
}

/// <summary>
/// Replaces all occurrences of <paramref name="from"/> to <paramref name="to"/> in <paramref name="str"/> and produce a new string.
/// </summary>
/// <param name="str">The string to search in.</param>
/// <param name="from">The string to find.</param>
/// <param name="to">The string to replace the found occurrence with.</param>
/// <returns>The resulting string.</returns>
std::wstring String::ReplaceAll(const std::wstring& str, const std::wstring& from, const std::wstring& to) {
	if (from.empty())
		return str;

	auto result    = str;
	auto start_pos = (size_t)0;

	while ((start_pos = result.find(from, start_pos)) != std::wstring::npos) {
		result.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}

	return result;
}

/// <summary>
/// Join a vector of strings into one string and separate each entry with <paramref name="separator"/>.
/// </summary>
/// <param name="list">The vector of string.</param>
/// <param name="separator">The separator to use in between each string.</param>
/// <returns>The joined string.</returns>
std::string String::Join(const std::vector<std::string>& list, const std::string& separator) {
	return std::accumulate(list.begin(), list.end(), std::string(),
		[&separator](const std::string& a, const std::string& b) -> std::string {
		return a + (a.length() > 0 ? separator : "") + b;
	});
}

/// <summary>
/// Join a vector of strings into one string and separate each entry with <paramref name="separator"/>.
/// </summary>
/// <param name="list">The vector of string.</param>
/// <param name="separator">The separator to use in between each string.</param>
/// <returns>The joined string.</returns>
std::wstring String::Join(const std::vector<std::wstring>& list, const std::wstring& separator) {
	return std::accumulate(list.begin(), list.end(), std::wstring(),
		[&separator](const std::wstring& a, const std::wstring& b) -> std::wstring {
		return a + (a.length() > 0 ? separator : L"") + b;
	});
}

/// <summary>
/// Pads a string on the left side with character <paramref name="ch"/> if its size is smaller than <paramref name="count"/> and returns the resulting string.
/// </summary>
/// <param name="input">The input string.</param>
/// <param name="count">The minimum length of the string, in characters.</param>
/// <param name="ch">The character to pad with.</param>
/// <returns>The new padded string.</returns>
std::string String::PadLeft(const std::string& input, size_t count, char ch) {
	if (input.size() >= count)
		return input;

	auto result = input;
	result.insert(result.begin(), count - input.size(), ch);
	return result;
}

/// <summary>
/// Pads a string on the right side with character <paramref name="ch"/> if its size is smaller than <paramref name="count"/> and returns the resulting string.
/// </summary>
/// <param name="input">The input string.</param>
/// <param name="count">The minimum length of the string, in characters.</param>
/// <param name="ch">The character to pad with.</param>
/// <returns>The new padded string.</returns>
std::string String::PadRight(const std::string& input, size_t count, char ch) {
	if (input.size() >= count)
		return input;

	auto result = input;
	result.insert(result.end(), count - input.size(), ch);
	return result;
}

/// <summary>
/// Pads a string on the left side with character <paramref name="ch"/> if its size is smaller than <paramref name="count"/> and returns the resulting string.
/// </summary>
/// <param name="input">The input string.</param>
/// <param name="count">The minimum length of the string, in characters.</param>
/// <param name="ch">The character to pad with.</param>
/// <returns>The new padded string.</returns>
std::wstring String::PadLeft(const std::wstring& input, size_t count, wchar_t ch) {
	if (input.size() >= count)
		return input;

	auto result = input;
	result.insert(result.begin(), count - input.size(), ch);
	return result;
}

/// <summary>
/// Pads a string on the right side with character <paramref name="ch"/> if its size is smaller than <paramref name="count"/> and returns the resulting string.
/// </summary>
/// <param name="input">The input string.</param>
/// <param name="count">The minimum length of the string, in characters.</param>
/// <param name="ch">The character to pad with.</param>
/// <returns>The new padded string.</returns>
std::wstring String::PadRight(const std::wstring& input, size_t count, wchar_t ch) {
	if (input.size() >= count)
		return input;

	auto result = input;
	result.insert(result.end(), count - input.size(), ch);
	return result;
}

/// <summary>
/// Pads a string on the left side with character <paramref name="ch"/> if its size is smaller than <paramref name="count"/> and returns the resulting string.
/// </summary>
/// <param name="input">The input string.</param>
/// <param name="count">The minimum length of the string, in characters.</param>
/// <param name="ch">The character to pad with.</param>
/// <returns>The new padded string.</returns>
std::string String::PadLeft(const char* input, size_t count, char ch) {
	return PadLeft(std::string(input), count, ch);
}

/// <summary>
/// Pads a string on the right side with character <paramref name="ch"/> if its size is smaller than <paramref name="count"/> and returns the resulting string.
/// </summary>
/// <param name="input">The input string.</param>
/// <param name="count">The minimum length of the string, in characters.</param>
/// <param name="ch">The character to pad with.</param>
/// <returns>The new padded string.</returns>
std::string String::PadRight(const char* input, size_t count, char ch) {
	return PadRight(std::string(input), count, ch);
}

/// <summary>
/// Pads a string on the left side with character <paramref name="ch"/> if its size is smaller than <paramref name="count"/> and returns the resulting string.
/// </summary>
/// <param name="input">The input string.</param>
/// <param name="count">The minimum length of the string, in characters.</param>
/// <param name="ch">The character to pad with.</param>
/// <returns>The new padded string.</returns>
std::wstring String::PadLeft(const wchar_t* input, size_t count, wchar_t ch) {
	return PadLeft(std::wstring(input), count, ch);
}

/// <summary>
/// Pads a string on the right side with character <paramref name="ch"/> if its size is smaller than <paramref name="count"/> and returns the resulting string.
/// </summary>
/// <param name="input">The input string.</param>
/// <param name="count">The minimum length of the string, in characters.</param>
/// <param name="ch">The character to pad with.</param>
/// <returns>The new padded string.</returns>
std::wstring String::PadRight(const wchar_t* input, size_t count, wchar_t ch) {
	return PadRight(std::wstring(input), count, ch);
}

/// <summary>
/// Convert a <see cref="::std::string"/> to a <see cref="::std::wstring"/>.
/// </summary>
/// <param name="input">The input string.</param>
/// <returns>The converted string.</returns>
std::wstring String::ToWString(const std::string& input) {
	if (input.empty())
		return std::wstring();

	size_t charsNeeded = ::MultiByteToWideChar(CP_UTF8, 0, input.data(), (int)input.size(), NULL, 0);
	if (charsNeeded == 0)
		throw std::runtime_error("Failed converting UTF-8 string to UTF-16");

	std::vector<wchar_t> buffer(charsNeeded);
	size_t charsConverted = ::MultiByteToWideChar(CP_UTF8, 0, input.data(), (int)input.size(), &buffer[0], (int)buffer.size());
	if (charsConverted == 0)
		throw std::runtime_error("Failed converting UTF-8 string to UTF-16");

	return std::wstring(&buffer[0], charsConverted);
}

/// <summary>
/// Convert a <see cref="::std::wstring"/> to a <see cref="::std::string"/>.
/// </summary>
/// <param name="input">The input string.</param>
/// <returns>The converted string.</returns>
std::string String::ToString(const std::wstring& input) {
	if (input.empty())
		return std::string();

	char defaultChar = '?';
	BOOL defaultCharUsed = false;
	size_t charsNeeded = ::WideCharToMultiByte(CP_UTF8, 0, input.data(), (int)input.size(), NULL, 0, &defaultChar, &defaultCharUsed);
	
	if (charsNeeded == 0)
		throw std::runtime_error("Failed converting UTF-16 string to UTF-8");

	std::vector<char> buffer(charsNeeded);
	size_t charsConverted = ::WideCharToMultiByte(CP_UTF8, 0, input.data(), (int)input.size(), &buffer[0], (int)buffer.size(), &defaultChar, &defaultCharUsed);
	if (charsConverted == 0)
		throw std::runtime_error("Failed converting UTF-16 string to UTF-8");

	return std::string(&buffer[0], charsConverted);
}

/// <summary>
/// Whitespace characters to be trimmed.
/// </summary>
const std::string WHITESPACE = " \n\r\t\f\v";

/// <summary>
/// Whitespace characters to be trimmed.
/// </summary>
const std::wstring WWHITESPACE = L" \n\r\t\f\v";

/// <summary>
/// Trim all whitespace of the left side of a string and return a new trimmed string.
/// </summary>
/// <param name="input">The input string.</param>
/// <returns>The trimmed string.</returns>
std::string String::TrimLeft(const std::string& input) {
	auto result = input;

	result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](int ch) {
        return !std::isspace(ch);
    }));

	return result;
}

/// <summary>
/// Trim all whitespace of the left side of a string and return a new trimmed string.
/// </summary>
/// <param name="input">The input string.</param>
/// <returns>The trimmed string.</returns>
std::wstring String::TrimLeft(const std::wstring& input) {
	auto result = input;

	result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](wint_t ch) {
        return !std::iswspace(ch);
    }));

	return result;
}

/// <summary>
/// Trim all whitespace of the right side of a string and return a new trimmed string.
/// </summary>
/// <param name="input">The input string.</param>
/// <returns>The trimmed string.</returns>
std::string String::TrimRight(const std::string& input) {
	auto result = input;

    result.erase(std::find_if(result.rbegin(), result.rend(), [](int ch) {
		return !std::isspace(ch);
	}).base(), result.end());

	return result;
}

/// <summary>
/// Trim all whitespace of the right side of a string and return a new trimmed string.
/// </summary>
/// <param name="input">The input string.</param>
/// <returns>The trimmed string.</returns>
std::wstring String::TrimRight(const std::wstring& input) {
	auto result = input;

    result.erase(std::find_if(result.rbegin(), result.rend(), [](wint_t ch) {
        return !std::iswspace(ch);
    }).base(), result.end());

	return result;
}

/// <summary>
/// Trim all whitespace of both sides of a string and return a new trimmed string.
/// </summary>
/// <param name="input">The input string.</param>
/// <returns>The trimmed string.</returns>
std::string String::Trim(const std::string& input) {
	return TrimRight(TrimLeft(input));
}

/// <summary>
/// Trim all whitespace of both sides of a string and return a new trimmed string.
/// </summary>
/// <param name="input">The input string.</param>
/// <returns>The trimmed string.</returns>
std::wstring String::Trim(const std::wstring& input) {
	return TrimRight(TrimLeft(input));
}

/// <summary>
/// Determines if string <paramref name="in"/> contains the string <paramref name="find"/>
/// </summary>
/// <param name="in">The string to search.</param>
/// <param name="find">The string to find.</param>
/// <returns>When found, true is returned.</returns>
bool String::Contains(const std::string& in, const std::string& find) {
	return (in.find(find, 0) != std::string::npos);
}