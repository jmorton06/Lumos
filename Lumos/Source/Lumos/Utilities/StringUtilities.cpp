#include "Precompiled.h"
#include "StringUtilities.h"
#include <cctype>

#ifdef LUMOS_PLATFORM_WINDOWS
#include <windows.h>
#include <DbgHelp.h>
#else
#include <cxxabi.h> // __cxa_demangle()
#endif

#include <iomanip>

namespace Lumos
{
    namespace StringUtilities
    {
        std::string GetFilePathExtension(const std::string& FileName)
        {
            auto pos = FileName.find_last_of('.');
            if(pos != std::string::npos)
                return FileName.substr(pos + 1);
            return "";
        }

        std::string RemoveFilePathExtension(const std::string& FileName)
        {
            auto pos = FileName.find_last_of('.');
            if(pos != std::string::npos)
                return FileName.substr(0, pos);
            return FileName;
        }

        std::string GetFileName(const std::string& FilePath)
        {
            auto pos = FilePath.find_last_of('/');
            if(pos != std::string::npos)
                return FilePath.substr(pos + 1);

            pos = FilePath.find_last_of("\\");
            if(pos != std::string::npos)
                return FilePath.substr(pos + 1);

            return FilePath;
        }

        std::string ToLower(const std::string& text)
        {
            std::string lowerText = text;
            transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
            return lowerText;
        }

        std::string GetFileLocation(const std::string& FilePath)
        {
            auto pos = FilePath.find_last_of('/');
            if(pos != std::string::npos)
                return FilePath.substr(0, pos + 1);

            pos = FilePath.find_last_of("\\");
            if(pos != std::string::npos)
                return FilePath.substr(0, pos + 1);

            return FilePath;
        }

        std::string RemoveName(const std::string& FilePath)
        {
            auto pos = FilePath.find_last_of('/');
            if(pos != std::string::npos)
                return FilePath.substr(0, pos + 1);

            pos = FilePath.find_last_of("\\");
            if(pos != std::string::npos)
                return FilePath.substr(0, pos + 1);

            return FilePath;
        }

        bool IsHiddenFile(const std::string& path)
        {
            if(path != ".." && path != "." && path[0] == '.')
            {
                return true;
            }

            return false;
        }

        TDArray<std::string> SplitString(const std::string& string, const std::string& delimiters)
        {
            size_t start = 0;
            size_t end   = string.find_first_of(delimiters);

            TDArray<std::string> result;

            while(end <= std::string::npos)
            {
                std::string token = string.substr(start, end - start);
                if(!token.empty())
                    result.PushBack(token);

                if(end == std::string::npos)
                    break;

                start = end + 1;
                end   = string.find_first_of(delimiters, start);
            }

            return result;
        }

        TDArray<std::string> SplitString(const std::string& string, const char delimiter)
        {
            return SplitString(string, std::string(1, delimiter));
        }

        TDArray<std::string> Tokenize(const std::string& string)
        {
            return SplitString(string, " \t\n");
        }

        TDArray<std::string> GetLines(const std::string& string)
        {
            return SplitString(string, "\n");
        }

        const char* FindToken(const char* str, const std::string& token)
        {
            const char* t = str;
            while((t = strstr(t, token.c_str())))
            {
                bool left  = str == t || isspace(t[-1]);
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

        int32_t FindStringPosition(const std::string& string, const std::string& search, uint32_t offset)
        {
            const char* str   = string.c_str() + offset;
            const char* found = strstr(str, search.c_str());
            if(found == nullptr)
                return -1;
            return (int32_t)(found - str) + offset;
        }

        std::string StringRange(const std::string& string, uint32_t start, uint32_t length)
        {
            return string.substr(start, length);
        }

        std::string RemoveStringRange(const std::string& string, uint32_t start, uint32_t length)
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
            const uint32_t length = static_cast<uint32_t>(end - str + 1);
            return std::string(str, length);
        }

        std::string GetBlock(const std::string& string, uint32_t offset)
        {
            const char* str = string.c_str() + offset;
            return StringUtilities::GetBlock(str);
        }

        std::string GetStatement(const char* str, const char** outPosition)
        {
            const char* end = strstr(str, ";");
            if(!end)
                return std::string(str);

            if(outPosition)
                *outPosition = end;
            const uint32_t length = static_cast<uint32_t>(end - str + 1);
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

        int32_t NextInt(const std::string& string)
        {
            for(uint32_t i = 0; i < string.size(); i++)
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
                                        [](unsigned char x)
                                        {
                                            return std::isspace(x);
                                        }),
                         string.end());

            return string;
        }

        std::string& RemoveCharacter(std::string& string, const char character)
        {
            std::string::iterator endIterator = std::remove(string.begin(), string.end(), character);
            string.erase(endIterator, string.end());
            string.erase(std::remove_if(string.begin(),
                                        string.end(),
                                        [](unsigned char x)
                                        {
                                            return std::isspace(x);
                                        }),
                         string.end());

            return string;
        }

        std::string Demangle(const std::string& string)
        {
            if(string.empty())
                return {};

#if defined(LUMOS_PLATFORM_WINDOWS)
            char undecorated_name[1024];
            if(!UnDecorateSymbolName(
                   string.c_str(), undecorated_name, sizeof(undecorated_name),
                   UNDNAME_COMPLETE))
            {
                return string;
            }
            else
            {
                return std::string(undecorated_name);
            }
#else
            char* demangled = nullptr;
            int status      = -1;
            demangled       = abi::__cxa_demangle(string.c_str(), nullptr, nullptr, &status);
            std::string ret = status == 0 ? std::string(demangled) : string;
            free(demangled);
            return ret;
#endif
        }

        std::string BytesToString(uint64_t bytes)
        {
            static const float gb = 1024 * 1024 * 1024;
            static const float mb = 1024 * 1024;
            static const float kb = 1024;

            std::stringstream result;
            if(bytes > gb)
                result << std::fixed << std::setprecision(2) << (float)bytes / gb << " gb";
            else if(bytes > mb)
                result << std::fixed << std::setprecision(2) << (float)bytes / mb << " mb";
            else if(bytes > kb)
                result << std::fixed << std::setprecision(2) << (float)bytes / kb << " kb";
            else
                result << std::fixed << std::setprecision(2) << (float)bytes << " bytes";

            return result.str();
        }

        String8 Str8SkipWhitespace(String8 str)
        {
            uint64_t first_non_ws = 0;
            for(uint64_t idx = 0; idx < str.size; idx += 1)
            {
                first_non_ws = idx;
                if(!CharIsSpace(str.str[idx]))
                {
                    break;
                }
                else if(idx == str.size - 1)
                {
                    first_non_ws = 1;
                }
            }
            return Substr8(str, RangeU64({ first_non_ws, str.size }));
        }

        String8 Str8ChopWhitespace(String8 str)
        {
            uint64_t first_ws_at_end = str.size;
            for(uint64_t idx = str.size - 1; idx < str.size; idx -= 1)
            {
                if(!CharIsSpace(str.str[idx]))
                {
                    break;
                }
                first_ws_at_end = idx;
            }
            return Substr8(str, RangeU64({ 0, first_ws_at_end }));
        }

        String8 Str8SkipChopWhitespace(String8 str)
        {
            return Str8SkipWhitespace(Str8ChopWhitespace(str));
        }

        String8 Str8SkipChopNewlines(String8 str)
        {
            uint64_t first_non_ws = 0;
            for(uint64_t idx = 0; idx < str.size; idx += 1)
            {
                first_non_ws = idx;
                if(str.str[idx] != '\n' && str.str[idx] != '\r')
                {
                    break;
                }
            }

            uint64_t first_ws_at_end = str.size;
            for(uint64_t idx = str.size - 1; idx < str.size; idx -= 1)
            {
                if(str.str[idx] != '\n' && str.str[idx] != '\r')
                {
                    break;
                }
                first_ws_at_end = idx;
            }

            return Substr8(str, RangeU64({ first_non_ws, first_ws_at_end }));
        }

        String8 Str8PathChopLastPeriod(String8 string)
        {
            uint64_t periodPos = FindSubstr8(string, Str8Lit("."), 0, MatchFlags::FindLast);
            if(periodPos < string.size)
            {
                string.size = periodPos;
            }
            return string;
        }

        String8 Str8PathSkipLastSlash(String8 string)
        {
            uint64_t slash_pos = FindSubstr8(string, Str8Lit("/"), 0, MatchFlags(MatchFlags::SlashInsensitive | MatchFlags::FindLast));
            if(slash_pos < string.size)
            {
                string.str += slash_pos + 1;
                string.size -= slash_pos + 1;
            }
            return string;
        }

        String8 Str8PathChopLastSlash(String8 string)
        {
            uint64_t slash_pos = FindSubstr8(string, Str8Lit("/"), 0, MatchFlags(MatchFlags::SlashInsensitive | MatchFlags::FindLast));
            if(slash_pos < string.size)
            {
                string.size = slash_pos;
            }
            return string;
        }

        String8 Str8PathSkipLastPeriod(String8 string)
        {
            uint64_t period_pos = FindSubstr8(string, Str8Lit("."), 0, MatchFlags::FindLast);
            if(period_pos < string.size)
            {
                string.str += period_pos + 1;
                string.size -= period_pos + 1;
            }
            return string;
        }

        String8 Str8PathChopPastLastSlash(String8 string)
        {
            uint64_t slash_pos = FindSubstr8(string, Str8Lit("/"), 0, MatchFlags(MatchFlags::SlashInsensitive | MatchFlags::FindLast));
            if(slash_pos < string.size)
            {
                string.size = slash_pos + 1;
            }
            return string;
        }

        PathType PathTypeFromStr8(String8 path)
        {
            PathType kind = PathType::Relative;
            if(path.size >= 1 && path.str[0] == '/')
            {
                kind = PathType::RootAbsolute;
            }
            if(path.size >= 2 && CharIsAlpha(path.str[0]) && path.str[1] == ':')
            {
                kind = PathType::DriveAbsolute;
            }
            return kind;
        }

        String8List PathPartsFromStr8(Arena* arena, String8 path)
        {
            String8 splits[] = { Str8Lit("/"), Str8Lit("\\") };
            String8List strs = StrSplit8(arena, path, ArrayCount(splits), splits);
            return strs;
        }

        String8List AbsolutePathPartsFromSourcePartsType(Arena* arena, String8 source, String8List parts, PathType type)
        {
            if(type == PathType::Relative)
            {
                String8List concattedParts = { 0 };
                String8List sourceParts    = PathPartsFromStr8(arena, source);
                Str8ListConcatInPlace(&concattedParts, &sourceParts);
                Str8ListConcatInPlace(&concattedParts, &parts);
                parts = concattedParts;
            }

            return parts;
        }

        String8 ResolveRelativePath(Arena* arena, String8 path)
        {
            ArenaTemp scratch      = ArenaTempBegin(arena);
            String8 pathCopy       = BackSlashesToSlashes(scratch.arena, path);
            String8 resolvedString = NormalizedPathFromStr8(arena, pathCopy, pathCopy);
            ArenaTempEnd(scratch);
            return resolvedString;
        }

        String8List DotResolvedPathPartsFromParts(Arena* arena, String8List parts)
        {
            ArenaTemp scratch = ArenaTempBegin(arena);
            struct NodeNode
            {
                NodeNode* next;
                String8Node* node;
            };

            NodeNode* part_stack_top = 0;
            for(String8Node* n = parts.first; n != 0; n = n->next)
            {
                if(Str8Match(n->string, Str8Lit(".."), MatchFlags(0)))
                {
                    StackPop(part_stack_top);
                }
                else if(Str8Match(n->string, Str8Lit("."), MatchFlags(0)))
                {
                }
                else
                {
                    NodeNode* nn = PushArray(scratch.arena, NodeNode, 1);
                    nn->node     = n;
                    StackPush(part_stack_top, nn);
                }
            }
            String8List result = { 0 };
            for(NodeNode* nn = part_stack_top; nn != 0; nn = nn->next)
            {
                Str8ListPushFront(arena, &result, nn->node->string);
            }
            ArenaTempEnd(scratch);
            return result;
        }

        String8 NormalizedPathFromStr8(Arena* arena, String8 source, String8 path)
        {
            ArenaTemp scratch                        = ArenaTempBegin(arena);
            path                                     = Str8SkipWhitespace(path);
            bool trailing_slash                      = path.size > 0 && (path.str[path.size - 1] == '/' || path.str[path.size - 1] == '\\');
            PathType type                            = PathTypeFromStr8(path);
            String8List path_parts                   = PathPartsFromStr8(scratch.arena, path);
            String8List absolute_path_parts          = AbsolutePathPartsFromSourcePartsType(scratch.arena, source, path_parts, type);
            String8List absolute_resolved_path_parts = DotResolvedPathPartsFromParts(scratch.arena, absolute_path_parts);
            StringJoin join                          = { 0 };
            join.sep                                 = Str8Lit("/");
            if(trailing_slash)
            {
                join.post = Str8Lit("/");
            }
            String8 absolute_resolved_path = Str8ListJoin(arena, absolute_resolved_path_parts, &join);
            ArenaTempEnd(scratch);
            return absolute_resolved_path;
        }

        String8 GetFileName(String8 str, bool directory)
        {
            if(directory)
                return Str8PathSkipLastSlash(str);
            else
                return Str8PathSkipLastSlash(Str8PathChopLastPeriod(str));
        }

        String8 AbsolutePathToRelativeFileSystemPath(Arena* arena, String8 path, String8 fileSystemPath, String8 prefix)
        {
            LUMOS_PROFILE_FUNCTION();

            ArenaTemp scratch      = ScratchBegin(&arena, 1);
            String8 pathCopy       = BackSlashesToSlashes(scratch.arena, path);
            String8 resolvedString = NormalizedPathFromStr8(scratch.arena, pathCopy, pathCopy);
            String8 outString;

            uint64_t loc = FindSubstr8(resolvedString, fileSystemPath, 0);

            if(loc != resolvedString.size)
            {
                resolvedString.str  = resolvedString.str + fileSystemPath.size;
                resolvedString.size = resolvedString.size - fileSystemPath.size;

                outString      = { 0 };
                outString.size = (resolvedString.size + prefix.size);
                outString.str  = PushArrayNoZero(arena, uint8_t, outString.size + 1);

                MemoryCopy(outString.str, prefix.str, prefix.size);
                MemoryCopy(outString.str + prefix.size, resolvedString.str, resolvedString.size);
                outString.str[outString.size] = 0;
            }
            else
                outString = PushStr8Copy(arena, resolvedString);

            ScratchEnd(scratch);
            return outString;
        }

        String8 RelativeToAbsolutePath(Arena* arena, String8 path, String8 prefix, String8 fileSystemPath)
        {
            LUMOS_PROFILE_FUNCTION();
            ArenaTemp scratch      = ScratchBegin(&arena, 1);
            String8 pathCopy       = BackSlashesToSlashes(scratch.arena, path);
            String8 resolvedString = NormalizedPathFromStr8(scratch.arena, pathCopy, pathCopy);
            String8 outString;

            uint64_t loc = FindSubstr8(resolvedString, prefix, 0);

            if(loc != resolvedString.size)
            {
                resolvedString.str  = resolvedString.str + prefix.size;
                resolvedString.size = resolvedString.size - prefix.size;

                outString      = { 0 };
                outString.size = (resolvedString.size + fileSystemPath.size);
                outString.str  = PushArrayNoZero(arena, uint8_t, outString.size + 1);

                MemoryCopy(outString.str, fileSystemPath.str, fileSystemPath.size);
                MemoryCopy(outString.str + fileSystemPath.size, resolvedString.str, resolvedString.size);
                outString.str[outString.size] = 0;
            }
            else
                outString = PushStr8Copy(arena, resolvedString);

            ScratchEnd(scratch);
            return outString;
        }

        uint64_t BasicHashFromString(String8 string)
        {
            uint64_t result = 5381;
            for(uint64_t i = 0; i < string.size; i += 1)
            {
                result = ((result << 5) + result) + string.str[i];
            }
            return result;
        }

        String8 BackSlashesToSlashes(Arena* arena, String8& string)
        {
            String8 copy = PushStr8Copy(arena, string);

            ArenaTemp scratch = ArenaTempBegin(arena);
            size_t len        = string.size;

            for(size_t i = 0; i < len; i++)
            {
                if(string.str[i] == '\\')
                {
                    copy.str[i] = '/';
                }
                else
                    copy.str[i] = string.str[i];
            }
            ArenaTempEnd(scratch);

            return copy;
        }

        String8 SlashesToBackSlashes(Arena* arena, String8& string)
        {
            String8 copy = PushStr8Copy(arena, string);

            ArenaTemp scratch = ArenaTempBegin(arena);
            size_t len        = string.size;
            for(size_t i = 0; i < len; i++)
            {
                if(string.str[i] == '/')
                {
                    copy.str[i] = '\\';
                }
                else
                    copy.str[i] = string.str[i];
            }
            ArenaTempEnd(scratch);

            return copy;
        }
    }
}
