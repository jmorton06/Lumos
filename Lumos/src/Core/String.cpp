#include "lmpch.h"
#include "String.h"
#include <cctype>

namespace Lumos
{
	std::vector<std::string> SplitString(const std::string& string, const std::string& delimiters)
	{
		size_t start = 0;
		size_t end = string.find_first_of(delimiters);

		std::vector<std::string> result;

		while(end <= std::string::npos)
		{
			std::string token = string.substr(start, end - start);
			if(!token.empty())
				result.push_back(token);

			if(end == std::string::npos)
				break;

			start = end + 1;
			end = string.find_first_of(delimiters, start);
		}

		return result;
	}

	std::vector<std::string> SplitString(const std::string& string, const char delimiter)
	{
		return SplitString(string, std::string(1, delimiter));
	}

	std::vector<std::string> Tokenize(const std::string& string)
	{
		return SplitString(string, " \t\n");
	}

	std::vector<std::string> GetLines(const std::string& string)
	{
		return SplitString(string, "\n");
	}

	const char* FindToken(const char* str, const std::string& token)
	{
		const char* t = str;
		while((t = strstr(t, token.c_str())))
		{
			bool left = str == t || isspace(t[-1]);
			bool right = !t[token.size()] || isspace(t[token.size()]);
			if(left && right)
				return t;

			t += token.size();
		}
		return nullptr;
	}

	const char* FindToken(const std::string& string, const std::string& token)
	{
		return FindToken(string.c_str(), token);
	}

	i32 FindStringPosition(const std::string& string, const std::string& search, u32 offset)
	{
		const char* str = string.c_str() + offset;
		const char* found = strstr(str, search.c_str());
		if(found == nullptr)
			return -1;
		return (i32)(found - str) + offset;
	}

	std::string StringRange(const std::string& string, u32 start, u32 length)
	{
		return string.substr(start, length);
	}

	std::string RemoveStringRange(const std::string& string, u32 start, u32 length)
	{
		std::string result = string;
		return result.erase(start, length);
	}

	std::string GetBlock(const char* str, const char** outPosition)
	{
		const char* end = strstr(str, "}");
		if(!end)
			return std::string(str);

		if(outPosition)
			*outPosition = end;
		const u32 length = static_cast<u32>(end - str + 1);
		return std::string(str, length);
	}

	std::string GetBlock(const std::string& string, u32 offset)
	{
		const char* str = string.c_str() + offset;
		return GetBlock(str);
	}

	std::string GetStatement(const char* str, const char** outPosition)
	{
		const char* end = strstr(str, ";");
		if(!end)
			return std::string(str);

		if(outPosition)
			*outPosition = end;
		const u32 length = static_cast<u32>(end - str + 1);
		return std::string(str, length);
	}

	bool StringContains(const std::string& string, const std::string& chars)
	{
		return string.find(chars) != std::string::npos;
	}

	bool StartsWith(const std::string& string, const std::string& start)
	{
		return string.find(start) == 0;
	}

	i32 NextInt(const std::string& string)
	{
		for(u32 i = 0; i < string.size(); i++)
		{
			if(isdigit(string[i]))
				return atoi(&string[i]);
		}
		return -1;
	}

	bool StringEquals(const std::string& string1, const std::string& string2)
	{
		return strcmp(string1.c_str(), string2.c_str()) == 0;
	}

	std::string StringReplace(std::string str, char ch1, char ch2)
	{
		for(int i = 0; i < str.length(); ++i)
		{
			if(str[i] == ch1)
				str[i] = ch2;
		}

		return str;
	}

	std::string StringReplace(std::string str, char ch)
	{
		for(int i = 0; i < str.length(); ++i)
		{
			if(str[i] == ch)
			{
				str = std::string(str).substr(0, i) + std::string(str).substr(i + 1, str.length());
			}
		}

		return str;
	}

	std::string& BackSlashesToSlashes(std::string& string)
	{
		size_t len = string.length();
		for(size_t i = 0; i < len; i++)
		{
			if(string[i] == '\\')
			{
				string[i] = '/';
			}
		}
		return string;
	}

	std::string& SlashesToBackSlashes(std::string& string)
	{
		size_t len = string.length();
		for(size_t i = 0; i < len; i++)
		{
			if(string[i] == '/')
			{
				string[i] = '\\';
			}
		}
		return string;
	}

	std::string& RemoveSpaces(std::string& string)
	{
		std::string::iterator endIterator = std::remove(string.begin(), string.end(), ' ');
		string.erase(endIterator, string.end());
		string.erase(std::remove_if(string.begin(),
						 string.end(),
						 [](unsigned char x) {
							 return std::isspace(x);
						 }),
			string.end());

		return string;
	}
}
