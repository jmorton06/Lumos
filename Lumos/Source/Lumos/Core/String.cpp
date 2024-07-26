#include "Precompiled.h"
#include "String.h"
#include <cstdarg>

// Based on https://github.com/Dion-Systems/metadesk/blob/master/source/md.h

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
        for(; cstr[length]; length += 1);
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
        for(; *p; p += 1);
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
    Substr8(String8 str, RangeU64 rng)
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
        return Substr8(str, RangeU64({ min, str.size }));
    }

    String8 Str8Chop(String8 str, uint64_t nmax)
    {
        return Substr8(str, RangeU64({ 0, str.size - nmax }));
    }

    String8 Prefix8(String8 str, uint64_t size)
    {
        return Substr8(str, RangeU64({ 0, size }));
    }

    String8 Suffix8(String8 str, uint64_t size)
    {
        return Substr8(str, RangeU64({ str.size - size, str.size }));
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
        uint64_t found_idx = haystack.size;
        for(uint64_t i = start_pos; i < haystack.size; i += 1)
        {
            if(i + needle.size <= haystack.size)
            {
                String8 substr = Substr8(haystack, RangeU64({ i, i + needle.size }));
                if(Str8Match(substr, needle, flags))
                {
                    found_idx = i;
                    if(!(flags & MatchFlags::FindLast))
                    {
                        break;
                    }
                }
            }
        }
        return found_idx;
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

        uint64_t needed_bytes = vsnprintf(0, 0, fmt, args) + 1;
        result.str            = PushArrayNoZero(arena, uint8_t, needed_bytes);
        result.size           = needed_bytes - 1;

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
        list->nodeCount++;
        list->totalSize += n->string.size;
    }

    void Str8ListPushNodeFront(String8List* list, String8Node* n)
    {
        QueuePushFront(list->first, list->last, n);
        list->nodeCount++;
        list->totalSize += n->string.size;
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
            list->nodeCount += to_push->nodeCount;
            list->totalSize += to_push->totalSize;
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

        uint64_t sepCount = 0;
        if(list.nodeCount > 1)
        {
            sepCount = list.nodeCount - 1;
        }
        String8 result = { 0 };
        result.size    = (list.totalSize + join.pre.size + sepCount * join.sep.size + join.post.size);
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

    bool operator==(const String8& lhs, const String8& rhs)
    {
        return Str8Match(lhs, rhs, MatchFlags(0));
    }

    uint8_t charToValue[] = {
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

    uint8_t charIsInteger[] = {
        0,
        0,
        0,
        0,
        0,
        0,
        1,
        1,
        1,
        0,
        0,
        0,
        1,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    };

    uint64_t U64FromStr8(String8 string, uint32_t radix)
    {
        ASSERT(2 <= radix && radix <= 16);

        uint64_t value = 0;
        for(uint64_t i = 0; i < string.size; i += 1)
        {
            value *= radix;
            uint8_t c = string.str[i];
            value += charToValue[(c - 0x30) & 0x1F];
        }
        return value;
    }

    bool StringIsU64(String8 string, uint32_t radix)
    {
        bool result = false;
        if(string.size > 0)
        {
            result = true;
            for(uint8_t *ptr = string.str, *opl = string.str + string.size; ptr < opl; ptr++)
            {
                uint8_t c = *ptr;
                if(!charIsInteger[c >> 3])
                {
                    result = false;
                    break;
                }
                if(charToValue[(c - 0x30) & 0x1F] >= radix)
                {
                    result = false;
                    break;
                }
            }
        }
        return (result);
    }

    int64_t IntFromStr8(String8 string)
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

    String8 HexStringFromU64(Arena* arena, uint64_t x, bool caps)
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
