#pragma once

#ifdef LUMOS_PLATFORM_ANDROID
template <typename T>
std::string to_string(const T& n)
{
    std::ostringstream stm;
    stm << n;
    return stm.str();
}
#endif

namespace Lumos
{
    namespace StringUtilities
    {
        template <typename T>
        static std::string ToString(const T& input)
        {
#ifdef LUMOS_PLATFORM_ANDROID
            return to_string(input);
#else
            return std::to_string(input);
#endif
        }

        std::string GetFilePathExtension(const std::string& FileName);
        std::string GetFileName(const std::string& FilePath);
        std::string GetFileLocation(const std::string& FilePath);

        std::string RemoveName(const std::string& FilePath);
        std::string RemoveFilePathExtension(const std::string& FileName);

        std::string& BackSlashesToSlashes(std::string& string);
        std::string& SlashesToBackSlashes(std::string& string);
        std::string& RemoveSpaces(std::string& string);
        std::string Demangle(const std::string& string);

        bool IsHiddenFile(const std::string& path);

        std::vector<std::string> LUMOS_EXPORT SplitString(const std::string& string, const std::string& delimiters);
        std::vector<std::string> LUMOS_EXPORT SplitString(const std::string& string, const char delimiter);
        std::vector<std::string> LUMOS_EXPORT Tokenize(const std::string& string);
        std::vector<std::string> GetLines(const std::string& string);

        const char* FindToken(const char* str, const std::string& token);
        const char* FindToken(const std::string& string, const std::string& token);
        int32_t FindStringPosition(const std::string& string, const std::string& search, uint32_t offset = 0);
        std::string StringRange(const std::string& string, uint32_t start, uint32_t length);
        std::string RemoveStringRange(const std::string& string, uint32_t start, uint32_t length);

        std::string GetBlock(const char* str, const char** outPosition = nullptr);
        std::string GetBlock(const std::string& string, uint32_t offset = 0);

        std::string GetStatement(const char* str, const char** outPosition = nullptr);

        bool StringContains(const std::string& string, const std::string& chars);
        bool StartsWith(const std::string& string, const std::string& start);
        int32_t NextInt(const std::string& string);

        bool StringEquals(const std::string& string1, const std::string& string2);
        std::string StringReplace(std::string str, char ch1, char ch2);
        std::string StringReplace(std::string str, char ch);
    }
}
