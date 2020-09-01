#pragma once

#ifdef LUMOS_PLATFORM_ANDROID
template<typename T>
std::string to_string(const T& n)
{
	std::ostringstream stm;
	stm << n;
	return stm.str();
}
#endif

namespace Lumos
{
	class LUMOS_EXPORT StringUtilities
	{
	public:
		template<typename T>
		static std::string ToString(const T& input)
		{
#ifdef LUMOS_PLATFORM_ANDROID
			return to_string(input);
#else
			return std::to_string(input);
#endif
		}

		static std::string GetFilePathExtension(const std::string& FileName)
		{
			auto pos = FileName.find_last_of('.');
			if(pos != std::string::npos)
				return FileName.substr(pos + 1);
			return "";
		}

        static std::string RemoveFilePathExtension(const std::string& FileName)
        {
            auto pos = FileName.find_last_of('.');
            if(pos != std::string::npos)
                return FileName.substr(0, pos);
            return FileName;
        }

		static std::string GetFileName(const std::string& FilePath)
		{
			auto pos = FilePath.find_last_of('/');
			if(pos != std::string::npos)
				return FilePath.substr(pos + 1);
			return FilePath;
		}
		
		static std::string RemoveName(const std::string& FilePath)
		{
			auto pos = FilePath.find_last_of('/');
			if(pos != std::string::npos)
                return FilePath.substr(0, pos + 1);
            return FilePath;}
	};

	std::vector<std::string> LUMOS_EXPORT SplitString(const std::string& string, const std::string& delimiters);
	std::vector<std::string> LUMOS_EXPORT SplitString(const std::string& string, const char delimiter);
	std::vector<std::string> LUMOS_EXPORT Tokenize(const std::string& string);
	std::vector<std::string> GetLines(const std::string& string);

	const char* FindToken(const char* str, const std::string& token);
	const char* FindToken(const std::string& string, const std::string& token);
	i32 FindStringPosition(const std::string& string, const std::string& search, u32 offset = 0);
	std::string StringRange(const std::string& string, u32 start, u32 length);
	std::string RemoveStringRange(const std::string& string, u32 start, u32 length);

	std::string GetBlock(const char* str, const char** outPosition = nullptr);
	std::string GetBlock(const std::string& string, u32 offset = 0);

	std::string GetStatement(const char* str, const char** outPosition = nullptr);

	bool StringContains(const std::string& string, const std::string& chars);
	bool StartsWith(const std::string& string, const std::string& start);
	i32 NextInt(const std::string& string);

	bool StringEquals(const std::string& string1, const std::string& string2);
	std::string StringReplace(std::string str, char ch1, char ch2);
	std::string StringReplace(std::string str, char ch);

	std::string& BackSlashesToSlashes(std::string& string);
	std::string& SlashesToBackSlashes(std::string& string);
	std::string& RemoveSpaces(std::string& string);
}
