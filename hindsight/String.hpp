#pragma once

#ifndef utilities_string_h
#define utilities_string_h
	#include <string>
	#include <vector>

	namespace Hindsight {
		namespace Utilities {
			/// <summary>
			/// String utility methods with several compatibility overloads.
			/// </summary>
			class String {
				public:
					/// <summary>
					/// Replaces the first occurrence of <paramref name="from"/> to <paramref name="to"/> in <paramref name="str"/> and produce a new string.
					/// </summary>
					/// <param name="str">The string to search in.</param>
					/// <param name="from">The string to find.</param>
					/// <param name="to">The string to replace the found occurrence with.</param>
					/// <returns>The resulting string.</returns>
					static std::string Replace(const std::string& str, const std::string& from, const std::string& to);

					/// <summary>
					/// Replaces all occurrences of <paramref name="from"/> to <paramref name="to"/> in <paramref name="str"/> and produce a new string.
					/// </summary>
					/// <param name="str">The string to search in.</param>
					/// <param name="from">The string to find.</param>
					/// <param name="to">The string to replace the found occurrence with.</param>
					/// <returns>The resulting string.</returns>
					static std::string ReplaceAll(const std::string& str, const std::string& from, const std::string& to);

					/// <summary>
					/// Replaces the first occurrence of <paramref name="from"/> to <paramref name="to"/> in <paramref name="str"/> and produce a new string.
					/// </summary>
					/// <param name="str">The string to search in.</param>
					/// <param name="from">The string to find.</param>
					/// <param name="to">The string to replace the found occurrence with.</param>
					/// <returns>The resulting string.</returns>
					static std::wstring Replace(const std::wstring& str, const std::wstring& from, const std::wstring& to);

					/// <summary>
					/// Replaces all occurrences of <paramref name="from"/> to <paramref name="to"/> in <paramref name="str"/> and produce a new string.
					/// </summary>
					/// <param name="str">The string to search in.</param>
					/// <param name="from">The string to find.</param>
					/// <param name="to">The string to replace the found occurrence with.</param>
					/// <returns>The resulting string.</returns>
					static std::wstring ReplaceAll(const std::wstring& str, const std::wstring& from, const std::wstring& to);

					/// <summary>
					/// Join a vector of strings into one string and separate each entry with <paramref name="separator"/>.
					/// </summary>
					/// <param name="list">The vector of string.</param>
					/// <param name="separator">The separator to use in between each string.</param>
					/// <returns>The joined string.</returns>
					static std::string Join(const std::vector<std::string>& list, const std::string& separator);

					/// <summary>
					/// Join a vector of strings into one string and separate each entry with <paramref name="separator"/>.
					/// </summary>
					/// <param name="list">The vector of string.</param>
					/// <param name="separator">The separator to use in between each string.</param>
					/// <returns>The joined string.</returns>
					static std::wstring Join(const std::vector<std::wstring>& list, const std::wstring& separator);

					/// <summary>
					/// Pads a string on the left side with character <paramref name="ch"/> if its size is smaller than <paramref name="count"/> and returns the resulting string.
					/// </summary>
					/// <param name="input">The input string.</param>
					/// <param name="count">The minimum length of the string, in characters.</param>
					/// <param name="ch">The character to pad with.</param>
					/// <returns>The new padded string.</returns>
					static std::string PadLeft(const std::string& input, size_t count, char ch);

					/// <summary>
					/// Pads a string on the right side with character <paramref name="ch"/> if its size is smaller than <paramref name="count"/> and returns the resulting string.
					/// </summary>
					/// <param name="input">The input string.</param>
					/// <param name="count">The minimum length of the string, in characters.</param>
					/// <param name="ch">The character to pad with.</param>
					/// <returns>The new padded string.</returns>
					static std::string PadRight(const std::string& input, size_t count, char ch);

					/// <summary>
					/// Pads a string on the left side with character <paramref name="ch"/> if its size is smaller than <paramref name="count"/> and returns the resulting string.
					/// </summary>
					/// <param name="input">The input string.</param>
					/// <param name="count">The minimum length of the string, in characters.</param>
					/// <param name="ch">The character to pad with.</param>
					/// <returns>The new padded string.</returns>
					static std::wstring PadLeft(const std::wstring& input, size_t count, wchar_t ch);

					/// <summary>
					/// Pads a string on the right side with character <paramref name="ch"/> if its size is smaller than <paramref name="count"/> and returns the resulting string.
					/// </summary>
					/// <param name="input">The input string.</param>
					/// <param name="count">The minimum length of the string, in characters.</param>
					/// <param name="ch">The character to pad with.</param>
					/// <returns>The new padded string.</returns>
					static std::wstring PadRight(const std::wstring& input, size_t count, wchar_t ch);

					/// <summary>
					/// Pads a string on the left side with character <paramref name="ch"/> if its size is smaller than <paramref name="count"/> and returns the resulting string.
					/// </summary>
					/// <param name="input">The input string.</param>
					/// <param name="count">The minimum length of the string, in characters.</param>
					/// <param name="ch">The character to pad with.</param>
					/// <returns>The new padded string.</returns>
					static std::string PadLeft(const char* input, size_t count, char ch);

					/// <summary>
					/// Pads a string on the right side with character <paramref name="ch"/> if its size is smaller than <paramref name="count"/> and returns the resulting string.
					/// </summary>
					/// <param name="input">The input string.</param>
					/// <param name="count">The minimum length of the string, in characters.</param>
					/// <param name="ch">The character to pad with.</param>
					/// <returns>The new padded string.</returns>
					static std::string PadRight(const char* input, size_t count, char ch);

					/// <summary>
					/// Pads a string on the left side with character <paramref name="ch"/> if its size is smaller than <paramref name="count"/> and returns the resulting string.
					/// </summary>
					/// <param name="input">The input string.</param>
					/// <param name="count">The minimum length of the string, in characters.</param>
					/// <param name="ch">The character to pad with.</param>
					/// <returns>The new padded string.</returns>
					static std::wstring PadLeft(const wchar_t* input, size_t count, wchar_t ch);

					/// <summary>
					/// Pads a string on the right side with character <paramref name="ch"/> if its size is smaller than <paramref name="count"/> and returns the resulting string.
					/// </summary>
					/// <param name="input">The input string.</param>
					/// <param name="count">The minimum length of the string, in characters.</param>
					/// <param name="ch">The character to pad with.</param>
					/// <returns>The new padded string.</returns>
					static std::wstring PadRight(const wchar_t* input, size_t count, wchar_t ch);

					/// <summary>
					/// Convert a <see cref="::std::string"/> to a <see cref="::std::wstring"/>.
					/// </summary>
					/// <param name="input">The input string.</param>
					/// <returns>The converted string.</returns>
					static std::wstring ToWString(const std::string& input);

					/// <summary>
					/// Convert a <see cref="::std::wstring"/> to a <see cref="::std::string"/>.
					/// </summary>
					/// <param name="input">The input string.</param>
					/// <returns>The converted string.</returns>
					static std::string ToString(const std::wstring& input);

					/// <summary>
					/// Trim all whitespace of the left side of a string and return a new trimmed string.
					/// </summary>
					/// <param name="input">The input string.</param>
					/// <returns>The trimmed string.</returns>
					static std::string TrimLeft(const std::string& input);

					/// <summary>
					/// Trim all whitespace of the left side of a string and return a new trimmed string.
					/// </summary>
					/// <param name="input">The input string.</param>
					/// <returns>The trimmed string.</returns>
					static std::wstring TrimLeft(const std::wstring& input);

					/// <summary>
					/// Trim all whitespace of the right side of a string and return a new trimmed string.
					/// </summary>
					/// <param name="input">The input string.</param>
					/// <returns>The trimmed string.</returns>
					static std::string TrimRight(const std::string& input);

					/// <summary>
					/// Trim all whitespace of the right side of a string and return a new trimmed string.
					/// </summary>
					/// <param name="input">The input string.</param>
					/// <returns>The trimmed string.</returns>
					static std::wstring TrimRight(const std::wstring& input);

					/// <summary>
					/// Trim all whitespace of both sides of a string and return a new trimmed string.
					/// </summary>
					/// <param name="input">The input string.</param>
					/// <returns>The trimmed string.</returns>
					static std::string Trim(const std::string& input);

					/// <summary>
					/// Trim all whitespace of both sides of a string and return a new trimmed string.
					/// </summary>
					/// <param name="input">The input string.</param>
					/// <returns>The trimmed string.</returns>
					static std::wstring Trim(const std::wstring& input);

					/// <summary>
					/// Determines if string <paramref name="in"/> contains the string <paramref name="find"/>
					/// </summary>
					/// <param name="in">The string to search.</param>
					/// <param name="find">The string to find.</param>
					/// <returns>When found, true is returned.</returns>
					static bool Contains(const std::string& in, const std::string& find);
			};

		}
	}

#endif 