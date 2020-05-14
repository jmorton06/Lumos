#pragma once

#include "lmpch.h"

typedef std::string String;

#ifdef LUMOS_PLATFORM_ANDROID 
template < typename T > std::string to_string( const T& n )
    {
        std::ostringstream stm ;
        stm << n ;
        return stm.str() ;
    }
#endif

namespace Lumos 
{
	class LUMOS_EXPORT StringFormat
	{
	public:
		template<typename T>
		static String ToString(const T& input)
		{
			#ifdef LUMOS_PLATFORM_ANDROID
			return to_string(input);
			#else
			return std::to_string(input);
			#endif
		}

		static String GetFilePathExtension(const String &FileName)
		{
			auto pos = FileName.find_last_of('.');
			if (pos != String::npos)
				return FileName.substr(pos + 1);
			return "";
		}
    
        static String GetFileName(const String &FilePath)
        {
            auto pos = FilePath.find_last_of('/');
            if (pos != String::npos)
                return FilePath.substr(pos + 1);
            return FilePath;
        }
	};

	std::vector<String> LUMOS_EXPORT SplitString(const String& string, const String& delimiters);
	std::vector<String> LUMOS_EXPORT SplitString(const String& string, const char delimiter);
	std::vector<String> LUMOS_EXPORT Tokenize(const String& string);
	std::vector<String> GetLines(const String& string);

	const char* FindToken(const char* str, const String& token);
	const char* FindToken(const String& string, const String& token);
	i32 FindStringPosition(const String& string, const String& search, u32 offset = 0);
	String StringRange(const String& string, u32 start, u32 length);
	String RemoveStringRange(const String& string, u32 start, u32 length);

	String GetBlock(const char* str, const char** outPosition = nullptr);
	String GetBlock(const String& string, u32 offset = 0);

	String GetStatement(const char* str, const char** outPosition = nullptr);

	bool StringContains(const String& string, const String& chars);
	bool StartsWith(const String& string, const String& start);
	i32 NextInt(const String& string);

	bool StringEquals(const String& string1, const String& string2);
	String StringReplace(String str, char ch1, char ch2);
	String StringReplace(String str, char ch);

	String& BackSlashesToSlashes(String& string);
	String& SlashesToBackSlashes(String& string);
}
