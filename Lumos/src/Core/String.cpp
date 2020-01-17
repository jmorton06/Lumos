#include "lmpch.h"
#include "String.h"

namespace Lumos
{
	std::vector<String> SplitString(const String& string, const String& delimiters)
	{
		size_t start = 0;
		size_t end = string.find_first_of(delimiters);

		std::vector<String> result;

		while (end <= String::npos)
		{
			String token = string.substr(start, end - start);
			if (!token.empty())
				result.push_back(token);

			if (end == String::npos)
				break;

			start = end + 1;
			end = string.find_first_of(delimiters, start);
		}

		return result;
	}

	std::vector<String> SplitString(const String& string, const char delimiter)
	{
		return SplitString(string, String(1, delimiter));
	}

	std::vector<String> Tokenize(const String& string)
	{
		return SplitString(string, " \t\n");
	}

	std::vector<String> GetLines(const String& string)
	{
		return SplitString(string, "\n");
	}

	const char* FindToken(const char* str, const String& token)
	{
		const char* t = str;
		while ((t = strstr(t, token.c_str())))
		{
			bool left = str == t || isspace(t[-1]);
			bool right = !t[token.size()] || isspace(t[token.size()]);
			if (left && right)
				return t;

			t += token.size();
		}
		return nullptr;
	}

	const char* FindToken(const String& string, const String& token)
	{
		return FindToken(string.c_str(), token);
	}

	i32 FindStringPosition(const String& string, const String& search, u32 offset)
	{
		const char* str = string.c_str() + offset;
		const char* found = strstr(str, search.c_str());
		if (found == nullptr)
			return -1;
		return (i32)(found - str) + offset;
	}

	String StringRange(const String& string, u32 start, u32 length)
	{
		return string.substr(start, length);
	}

	String RemoveStringRange(const String& string, u32 start, u32 length)
	{
		String result = string;
		return result.erase(start, length);
	}

	String GetBlock(const char* str, const char** outPosition)
	{
		const char* end = strstr(str, "}");
		if (!end)
			return String(str);

		if (outPosition)
			*outPosition = end;
		const u32 length = static_cast<u32>(end - str + 1);
		return String(str, length);
	}

	String GetBlock(const String& string, u32 offset)
	{
		const char* str = string.c_str() + offset;
		return GetBlock(str);
	}

	String GetStatement(const char* str, const char** outPosition)
	{
		const char* end = strstr(str, ";");
		if (!end)
			return String(str);

		if (outPosition)
			*outPosition = end;
		const u32 length = static_cast<u32>(end - str + 1);
		return String(str, length);
	}

	bool StringContains(const String& string, const String& chars)
	{
		return string.find(chars) != String::npos;
	}

	bool StartsWith(const String& string, const String& start)
	{
		return string.find(start) == 0;
	}

	i32 NextInt(const String& string)
	{
		for (u32 i = 0; i < string.size(); i++)
		{
			if (isdigit(string[i]))
				return atoi(&string[i]);
		}
		return -1;
	}

	bool StringEquals(const String& string1, const String& string2)
	{
		return strcmp(string1.c_str(), string2.c_str()) == 0;
	}

	String StringReplace(String str, char ch1, char ch2) 
	{
		for (int i = 0; i < str.length(); ++i) 
		{
			if (str[i] == ch1)
				str[i] = ch2;
		}

		return str;
	}

	String StringReplace(String str, char ch) 
	{
		for (int i = 0; i < str.length(); ++i)
		{
			if (str[i] == ch)
			{
				str = String(str).substr(0, i) + String(str).substr(i + 1, str.length());
			}
		}

		return str;
	}

	String& BackSlashesToSlashes(String& string)
	{
		size_t len = string.length();
		for (size_t i = 0; i < len; i++) 
		{
			if (string[i] == '\\') 
			{
				string[i] = '/';
			}
		}
		return string;
	}

	String& SlashesToBackSlashes(String& string)
	{
		size_t len = string.length();
		for (size_t i = 0; i < len; i++) 
		{
			if (string[i] == '/') 
			{
				string[i] = '\\';
			}
		}
		return string;
	}

}
