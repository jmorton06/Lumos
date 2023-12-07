#include "Precompiled.h"
#include "String.h"
#include <cstdarg>

namespace Lumos
{
    bool CharIsAlpha(uint8_t c)
    {
        return CharIsAlphaUpper(c) || CharIsAlphaLower(c);
    }

    bool CharIsAlphaUpper(uint8_t c)
    {
        return c >= 'A' && c <= 'Z';
    }

    bool CharIsAlphaLower(uint8_t c)
    {
        return c >= 'a' && c <= 'z';
    }

    bool CharIsDigit(uint8_t c)
    {
        return (c >= '0' && c <= '9');
    }

    bool CharIsSymbol(uint8_t c)
    {
        return (c == '~' || c == '!' || c == '$' || c == '%' || c == '^' || c == '&' || c == '*' || c == '-' || c == '=' || c == '+' || c == '<' || c == '.' || c == '>' || c == '/' || c == '?' || c == '|' || c == '\\' || c == '{' || c == '}' || c == '(' || c == ')' || c == '\\' || c == '[' || c == ']' || c == '#' || c == ',' || c == ';' || c == ':' || c == '@');
    }

    bool CharIsSpace(uint8_t c)
    {
        return c == ' ' || c == '\r' || c == '\t' || c == '\f' || c == '\v' || c == '\n';
    }

    uint8_t CharToUpper(uint8_t c)
    {
        return (c >= 'a' && c <= 'z') ? ('A' + (c - 'a')) : c;
    }

    uint8_t CharToLower(uint8_t c)
    {
        return (c >= 'A' && c <= 'Z') ? ('a' + (c - 'A')) : c;
    }

    uint8_t CharToForwardSlash(uint8_t c)
    {
        return (c == '\\' ? '/' : c);
    }

    uint64_t CalculateCStringLength(char* cstr)
    {
        uint64_t length = 0;
        for(; cstr[length]; length += 1)
            ;
        return length;
    }

    String8 Str8(uint8_t* str, uint64_t size)
    {
        String8 string;
        string.str  = str;
        string.size = size;
        return string;
    }

    String8 Str8Range(uint8_t* first, uint8_t* one_past_last)
    {
        String8 string;
        string.str  = first;
        string.size = (uint64_t)(one_past_last - first);
        return string;
    }

    String16 Str16(uint16_t* str, uint64_t size)
    {
        String16 result;
        result.str  = str;
        result.size = size;
        return result;
    }

    String16 Str16C(uint16_t* ptr)
    {
        uint16_t* p = ptr;
        for(; *p; p += 1)
            ;
        String16 result = Str16(ptr, p - ptr);
        return result;
    }

    String32 Str32(uint32_t* str, uint64_t size)
    {
        String32 string = { 0 };
        string.str      = str;
        string.size     = size;
        return string;
    }

    String8
    Substr8(String8 str, Range1U64 rng)
    {
        uint64_t min = rng.min;
        uint64_t max = rng.max;
        if(max > str.size)
        {
            max = str.size;
        }
        if(min > str.size)
        {
            min = str.size;
        }
        if(min > max)
        {
            uint64_t swap = min;
            min           = max;
            max           = swap;
        }
        str.size = max - min;
        str.str += min;
        return str;
    }

    String8 Str8Skip(String8 str, uint64_t min)
    {
        return Substr8(str, Range1U64({ min, str.size }));
    }

    String8 Str8Chop(String8 str, uint64_t nmax)
    {
        return Substr8(str, Range1U64({ 0, str.size - nmax }));
    }

    String8 Prefix8(String8 str, uint64_t size)
    {
        return Substr8(str, Range1U64({ 0, size }));
    }

    String8 Suffix8(String8 str, uint64_t size)
    {
        return Substr8(str, Range1U64({ str.size - size, str.size }));
    }

    bool Str8Match(String8 a, String8 b, MatchFlags flags)
    {
        bool result = false;
        if(a.size == b.size)
        {
            result = 1;
            for(uint64_t i = 0; i < a.size; i += 1)
            {
                bool match = (a.str[i] == b.str[i]);
                if(match == false)
                {
                    result = false;
                    break;
                }
            }
        }
        return result;
    }

    uint64_t FindSubstr8(String8 haystack, String8 needle, uint64_t start_pos, MatchFlags flags)
    {
        bool found         = 0;
        uint64_t found_idx = haystack.size;
        for(uint64_t i = start_pos; i < haystack.size; i += 1)
        {
            if(i + needle.size <= haystack.size)
            {
                String8 substr = Substr8(haystack, Range1U64({ i, i + needle.size }));
                if(Str8Match(substr, needle, flags))
                {
                    found_idx = i;
                    found     = 1;
                    if(!(flags & MatchFlag_FindLast))
                    {
                        break;
                    }
                }
            }
        }
        return found_idx;
    }

    FuzzyMatchList FindFuzzy8(Arena* arena, String8 haystack, String8 needle, uint64_t start_pos, MatchFlags flags)
    {
        ArenaTemp scratch      = ArenaTempBegin(arena);
        FuzzyMatchList matches = { 0 };
        {
            String8 splits[] = {
                Str8Lit(" "),
                Str8Lit("/"),
                Str8Lit("\\"),
                Str8Lit("\t"),
                Str8Lit("\n"),
                Str8Lit("*"),
                Str8Lit("_"),
                Str8Lit("-"),
            };
            String8List needle_parts           = StrSplit8(scratch.arena, needle, ArrayCount(splits), splits);
            uint64_t start_search_haystack_pos = 0;
            for(String8Node* n = needle_parts.first; n != 0; n = n->next)
            {
                bool found = 0;
                for(uint64_t search_pos = start_search_haystack_pos; search_pos < haystack.size;)
                {
                    uint64_t needle_part_pos = FindSubstr8(haystack, n->string, search_pos, flags);
                    search_pos               = needle_part_pos + 1;
                    if(needle_part_pos < haystack.size)
                    {
                        FuzzyMatchNode* match_n = PushArray(arena, FuzzyMatchNode, 1);
                        match_n->match          = Range1U64({ needle_part_pos, needle_part_pos + n->string.size });
                        QueuePush(matches.first, matches.last, match_n);
                        matches.count += 1;
                        found = 1;
                    }
                }
                if(found == 0)
                {
                    matches.missed_count += 1;
                }
            }
        }
        ArenaTempEnd(scratch);
        return matches;
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
        return Substr8(str, Range1U64({ first_non_ws, str.size }));
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
        return Substr8(str, Range1U64({ 0, first_ws_at_end }));
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

        return Substr8(str, Range1U64({ first_non_ws, first_ws_at_end }));
    }

    String8 Str8PathChopLastPeriod(String8 string)
    {
        uint64_t period_pos = FindSubstr8(string, Str8Lit("."), 0, MatchFlag_FindLast);
        if(period_pos < string.size)
        {
            string.size = period_pos;
        }
        return string;
    }

    String8 Str8PathSkipLastSlash(String8 string)
    {
        uint64_t slash_pos = FindSubstr8(string, Str8Lit("/"), 0, MatchFlag_SlashInsensitive | MatchFlag_FindLast);
        if(slash_pos < string.size)
        {
            string.str += slash_pos + 1;
            string.size -= slash_pos + 1;
        }
        return string;
    }

    String8 Str8PathChopLastSlash(String8 string)
    {
        uint64_t slash_pos = FindSubstr8(string, Str8Lit("/"), 0, MatchFlag_SlashInsensitive | MatchFlag_FindLast);
        if(slash_pos < string.size)
        {
            string.size = slash_pos;
        }
        return string;
    }

    String8 Str8PathSkipLastPeriod(String8 string)
    {
        uint64_t period_pos = FindSubstr8(string, Str8Lit("."), 0, MatchFlag_FindLast);
        if(period_pos < string.size)
        {
            string.str += period_pos + 1;
            string.size -= period_pos + 1;
        }
        return string;
    }

    String8 Str8PathChopPastLastSlash(String8 string)
    {
        uint64_t slash_pos = FindSubstr8(string, Str8Lit("/"), 0, MatchFlag_SlashInsensitive | MatchFlag_FindLast);
        if(slash_pos < string.size)
        {
            string.size = slash_pos + 1;
        }
        return string;
    }

    PathType PathTypeFromStr8(String8 path)
    {
        PathType kind = PathType_Relative;
        if(path.size >= 1 && path.str[0] == '/')
        {
            kind = PathType_RootAbsolute;
        }
        if(path.size >= 2 && CharIsAlpha(path.str[0]) && path.str[1] == ':')
        {
            kind = PathType_DriveAbsolute;
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
        if(type == PathType_Relative)
        {
            String8List concatted_parts = { 0 };
            String8List source_parts    = PathPartsFromStr8(arena, source);
            Str8ListConcatInPlace(&concatted_parts, &source_parts);
            Str8ListConcatInPlace(&concatted_parts, &parts);
            parts = concatted_parts;
        }

        return parts;
    }

    String8 PushStr8Copy(Arena* arena, const char* string)
    {
        return PushStr8Copy(arena, Str8C((char*)string));
    }

    String8 PushStr8Copy(Arena* arena, String8 string)
    {
        String8 res;
        res.size = string.size;
        res.str  = PushArrayNoZero(arena, uint8_t, string.size + 1);
        MemoryCopy(res.str, string.str, string.size);
        res.str[string.size] = 0;
        return res;
    }

    String8 PushStr8FV(Arena* arena, const char* fmt, va_list args)
    {
        String8 result = { 0 };
        va_list args2;
        va_copy(args2, args);

        uint64_t needed_bytes = vsnprintf(0, 0, fmt, args) + 1; // fmt::formatted_size(fmt, args2);// ts_stbsp_vsnprintf(0, 0, fmt, args)+1;
        result.str            = PushArrayNoZero(arena, uint8_t, needed_bytes);
        result.size           = needed_bytes - 1;
        // fmt::format_to(result.str, fmt, args2);

        vsnprintf((char*)result.str, needed_bytes, fmt, args2);

        return result;
    }

    // snprintf formatting
    String8 PushStr8F(Arena* arena, const char* fmt, ...)
    {
        String8 result = { 0 };
        va_list args;
        va_start(args, fmt);
        result = PushStr8FV(arena, fmt, args);
        va_end(args);
        return result;
    }

    String8 PushStr8FillByte(Arena* arena, uint64_t size, uint8_t byte)
    {
        String8 result = { 0 };
        result.str     = PushArrayNoZero(arena, uint8_t, size);
        MemorySet(result.str, byte, size);
        result.size = size;
        return result;
    }

    void Str8ListPushNode(String8List* list, String8Node* n)
    {
        QueuePush(list->first, list->last, n);
        list->node_count += 1;
        list->total_size += n->string.size;
    }

    void Str8ListPushNodeFront(String8List* list, String8Node* n)
    {
        QueuePushFront(list->first, list->last, n);
        list->node_count += 1;
        list->total_size += n->string.size;
    }

    void Str8ListPush(Arena* arena, String8List* list, String8 str)
    {
        String8Node* n = PushArray(arena, String8Node, 1);
        n->string      = str;
        Str8ListPushNode(list, n);
    }

    void Str8ListPushF(Arena* arena, String8List* list, char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        String8 string = PushStr8FV(arena, fmt, args);
        va_end(args);
        Str8ListPush(arena, list, string);
    }

    void Str8ListPushFront(Arena* arena, String8List* list, String8 str)
    {
        String8Node* n = PushArray(arena, String8Node, 1);
        n->string      = str;
        Str8ListPushNodeFront(list, n);
    }

    void Str8ListConcatInPlace(String8List* list, String8List* to_push)
    {
        if(to_push->first)
        {
            list->node_count += to_push->node_count;
            list->total_size += to_push->total_size;
            if(list->last == 0)
            {
                *list = *to_push;
            }
            else
            {
                list->last->next = to_push->first;
                list->last       = to_push->last;
            }
        }
        MemoryZero(to_push, sizeof(*to_push));
    }

    String8List StrSplit8(Arena* arena, String8 string, uint64_t split_count, String8* splits)
    {
        String8List list = { 0 };

        uint64_t split_start = 0;
        for(uint64_t i = 0; i < string.size; i += 1)
        {
            bool was_split = 0;
            for(uint64_t split_idx = 0; split_idx < split_count; split_idx += 1)
            {
                bool match = 0;
                if(i + splits[split_idx].size <= string.size)
                {
                    match = 1;
                    for(uint64_t split_i = 0; split_i < splits[split_idx].size && i + split_i < string.size; split_i += 1)
                    {
                        if(splits[split_idx].str[split_i] != string.str[i + split_i])
                        {
                            match = 0;
                            break;
                        }
                    }
                }
                if(match)
                {
                    String8 split_string = Str8(string.str + split_start, i - split_start);
                    Str8ListPush(arena, &list, split_string);
                    split_start = i + splits[split_idx].size;
                    i += splits[split_idx].size - 1;
                    was_split = 1;
                    break;
                }
            }

            if(was_split == 0 && i == string.size - 1)
            {
                String8 split_string = Str8(string.str + split_start, i + 1 - split_start);
                Str8ListPush(arena, &list, split_string);
                break;
            }
        }

        return list;
    }

    String8 Str8ListJoin(Arena* arena, String8List list, StringJoin* optional_params)
    {
        StringJoin join = { 0 };
        if(optional_params != 0)
        {
            MemoryCopy(&join, optional_params, sizeof(join));
        }

        uint64_t sep_count = 0;
        if(list.node_count > 1)
        {
            sep_count = list.node_count - 1;
        }
        String8 result = { 0 };
        result.size    = (list.total_size + join.pre.size + sep_count * join.sep.size + join.post.size);
        result.str     = PushArrayNoZero(arena, uint8_t, result.size + 1);

        uint8_t* ptr = result.str;
        MemoryCopy(ptr, join.pre.str, join.pre.size);
        ptr += join.pre.size;
        for(String8Node* node = list.first; node; node = node->next)
        {
            MemoryCopy(ptr, node->string.str, node->string.size);
            ptr += node->string.size;
            if(node != list.last)
            {
                MemoryCopy(ptr, join.sep.str, join.sep.size);
                ptr += join.sep.size;
            }
        }
        MemoryCopy(ptr, join.post.str, join.post.size);
        ptr += join.post.size;

        result.str[result.size] = 0;

        return result;
    }

    String8List DotResolvedPathPartsFromParts(Arena* arena, String8List parts)
    {
        ArenaTemp scratch = ArenaTempBegin(arena);
        typedef struct NodeNode NodeNode;
        struct NodeNode
        {
            NodeNode* next;
            String8Node* node;
        };
        NodeNode* part_stack_top = 0;
        for(String8Node* n = parts.first; n != 0; n = n->next)
        {
            if(Str8Match(n->string, Str8Lit(".."), 0))
            {
                StackPop(part_stack_top);
            }
            else if(Str8Match(n->string, Str8Lit("."), 0))
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
        String8 absolute_resolved_path = Str8ListJoin(scratch.arena, absolute_resolved_path_parts, &join);
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

    bool operator==(const String8& lhs, const String8& rhs)
    {
        return Str8Match(lhs, rhs, 0);
    }

    uint64_t U64FromStr8(String8 string, uint32_t radix)
    {
        LUMOS_ASSERT(2 <= radix && radix <= 16);
        uint8_t char_to_value[] = {
            0x00,
            0x01,
            0x02,
            0x03,
            0x04,
            0x05,
            0x06,
            0x07,
            0x08,
            0x09,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0x0A,
            0x0B,
            0x0C,
            0x0D,
            0x0E,
            0x0F,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
        };
        uint64_t value = 0;
        for(uint64_t i = 0; i < string.size; i += 1)
        {
            value *= radix;
            uint8_t c = string.str[i];
            value += char_to_value[(c - 0x30) & 0x1F];
        }
        return value;
    }

    int64_t CStyleIntFromStr8(String8 string)
    {
        uint64_t p = 0;

        // consume sign
        double sign = +1;
        if(p < string.size)
        {
            uint8_t c = string.str[p];
            if(c == '-')
            {
                sign = -1;
                p += 1;
            }
            else if(c == '+')
            {
                p += 1;
            }
        }

        // radix from prefix
        int64_t radix = 10;
        if(p < string.size)
        {
            uint8_t c0 = string.str[p];
            if(c0 == '0')
            {
                p += 1;
                radix = 8;
                if(p < string.size)
                {
                    uint8_t c1 = string.str[p];
                    if(c1 == 'x')
                    {
                        p += 1;
                        radix = 16;
                    }
                    else if(c1 == 'b')
                    {
                        p += 1;
                        radix = 2;
                    }
                }
            }
        }

        // consume integer "digits"
        String8 digits_substr = Str8Skip(string, p);
        uint64_t n            = U64FromStr8(digits_substr, (uint32_t)radix);

        // combine result
        int64_t result = (int64_t)sign * (int64_t)n;
        return result;
    }

    double DoubleFromStr8(String8 string)
    {
        char str[64];
        uint64_t str_size = string.size;
        if(str_size > sizeof(str) - 1)
        {
            str_size = sizeof(str) - 1;
        }
        MemoryCopy(str, string.str, str_size);
        str[str_size] = 0;
        return atof(str);
    }

    String8 CStyleHexStringFromU64(Arena* arena, uint64_t x, bool caps)
    {
        char int_value_to_char[] = "0123456789abcdef";
        uint8_t buffer[10];
        uint8_t* opl = buffer + 10;
        uint8_t* ptr = opl;
        if(x == 0)
        {
            ptr -= 1;
            *ptr = '0';
        }
        else
        {
            for(;;)
            {
                uint32_t val = x % 16;
                x /= 16;
                uint8_t c = (uint8_t)int_value_to_char[val];
                if(caps)
                {
                    c = CharToUpper(c);
                }
                ptr -= 1;
                *ptr = c;
                if(x == 0)
                {
                    break;
                }
            }
        }
        ptr -= 1;
        *ptr = 'x';
        ptr -= 1;
        *ptr = '0';

        String8 result = { 0 };
        result.size    = (uint64_t)(ptr - buffer);
        result.str     = PushArrayNoZero(arena, uint8_t, result.size);
        MemoryCopy(result.str, buffer, result.size);

        return result;
    }
}
