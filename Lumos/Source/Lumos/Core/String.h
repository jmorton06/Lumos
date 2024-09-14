#pragma once
#include "OS/Memory.h"
#include <stdarg.h>

namespace Lumos
{
    struct String8
    {
        uint8_t* str  = nullptr;
        uint64_t size = 0;

        friend bool operator==(const String8& lhs, const String8& rhs);
    };

    struct String16
    {
        uint16_t* str;
        uint64_t size;
    };

    struct String32
    {
        uint32_t* str;
        uint64_t size;
    };

    struct String8Node
    {
        String8Node* next;
        String8 string;
    };

    struct String8List
    {
        String8Node* first;
        String8Node* last;
        uint64_t nodeCount;
        uint64_t totalSize;
    };

    struct String8Array
    {
        uint64_t count;
        String8* v;
    };

    struct StringJoin
    {
        String8 pre;
        String8 sep;
        String8 post;
    };

    enum MatchFlags : uint32_t
    {
        CaseInsensitive  = (1 << 0),
        RightSideSloppy  = (1 << 1),
        SlashInsensitive = (1 << 2),
        FindLast         = (1 << 3),
        KeepEmpties      = (1 << 4),
    };

    enum IdentifierStyle : uint8_t
    {
        UpperCamelCase,
        LowerCamelCase,
        UpperCase,
        LowerCase,
    };

    struct RangeU64
    {
        uint64_t min;
        uint64_t max;
    };

    bool CharIsAlpha(uint8_t c);
    bool CharIsAlphaUpper(uint8_t c);
    bool CharIsAlphaLower(uint8_t c);
    bool CharIsDigit(uint8_t c);
    bool CharIsSymbol(uint8_t c);
    bool CharIsSpace(uint8_t c);
    uint8_t CharToUpper(uint8_t c);
    uint8_t CharToLower(uint8_t c);
    uint8_t CharToForwardSlash(uint8_t c);

    uint64_t CalculateCStringLength(char* cstr);
    String8 Str8(uint8_t* str, uint64_t size);

    String8 Str8Range(uint8_t* first, uint8_t* one_past_last);
    String16 Str16(uint16_t* str, uint64_t size);
    String16 Str16C(uint16_t* ptr);
    String32 Str32(uint32_t* str, uint64_t size);

    String8 PushStr8Copy(Arena* arena, const char* string);
    String8 PushStr8Copy(Arena* arena, String8 string);
    String8 PushStr8FV(Arena* arena, const char* fmt, va_list args);
    String8 PushStr8F(Arena* arena, const char* fmt, ...);
    String8 PushStr8FillByte(Arena* arena, uint64_t size, uint8_t byte);

    String8 Str8FV(String8 allocatedString, const char* fmt, va_list args);
    String8 Str8F(String8 allocatedString, const char* fmt, ...);

    void Str8ListPushNode(String8List* list, String8Node* n);
    void Str8ListPushNodeFront(String8List* list, String8Node* n);
    void Str8ListPush(Arena* arena, String8List* list, String8 str);
    void Str8ListPushF(Arena* arena, String8List* list, char* fmt, ...);
    void Str8ListPushFront(Arena* arena, String8List* list, String8 str);
    void Str8ListConcatInPlace(String8List* list, String8List* to_push);
    String8List StrSplit8(Arena* arena, String8 string, uint64_t split_count, String8* splits);
    String8 Str8ListJoin(Arena* arena, String8List list, StringJoin* optional_params);

    String8 Substr8(String8 str, RangeU64 rng);
    String8 Str8Skip(String8 str, uint64_t min);
    String8 Str8Chop(String8 str, uint64_t nmax);
    String8 Prefix8(String8 str, uint64_t size);
    String8 Suffix8(String8 str, uint64_t size);

    bool Str8Match(String8 a, String8 b, MatchFlags flags = MatchFlags(0));
    uint64_t FindSubstr8(String8 haystack, String8 needle, uint64_t start_pos, MatchFlags flags = MatchFlags(0));

    uint64_t U64FromStr8(String8 str, uint32_t radix);
    bool StringIsU64(String8 string, uint32_t radix);

    int64_t IntFromStr8(String8 str);
    double DoubleFromStr8(String8 str);
    String8 HexStringFromU64(Arena* arena, uint64_t x, bool caps);

    bool operator==(const String8& lhs, const String8& rhs);

#define Str8Struct(ptr) Str8((uint8_t*)(ptr), sizeof(*(ptr)))
#define Str8C(cstring) Str8((uint8_t*)(cstring), CalculateCStringLength(cstring))
#define Str8StdS(stdString) Str8((uint8_t*)(stdString.c_str()), stdString.size())
#define Str8Lit(s) Str8((uint8_t*)(s), sizeof(s) - 1)
#define Str8VArg(s) (int)(s).size, (s).str
#define Str8ListFirst(list) ((list)->first != 0 ? (list)->first->string : Str8Lit(""))
#define ToStdString(s) std::string((const char*)s.str, s.size)
#define ToCChar(s) (const char*)s.str
}
