#pragma once
#include "OS/Memory.h"

namespace Lumos
{
    struct String8
    {
        uint8_t* str;
        uint64_t size;

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
        uint64_t node_count;
        uint64_t total_size;
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

    typedef uint32_t MatchFlags;
    enum
    {
        MatchFlag_CaseInsensitive  = (1 << 0),
        MatchFlag_RightSideSloppy  = (1 << 1),
        MatchFlag_SlashInsensitive = (1 << 2),
        MatchFlag_FindLast         = (1 << 3),
        MatchFlag_KeepEmpties      = (1 << 4),
    };

    enum IdentifierStyle
    {
        IdentifierStyle_UpperCamelCase,
        IdentifierStyle_LowerCamelCase,
        IdentifierStyle_UpperCase,
        IdentifierStyle_LowerCase,
    };

    enum PathType
    {
        PathType_Relative,
        PathType_DriveAbsolute,
        PathType_RootAbsolute,
        PathKType_Count
    };

    struct Range1U64
    {
        uint64_t min;
        uint64_t max;
    };

    struct FuzzyMatchNode
    {
        FuzzyMatchNode* next;
        Range1U64 match;
    };

    struct FuzzyMatchList
    {
        FuzzyMatchNode* first;
        FuzzyMatchNode* last;
        uint64_t count;
        uint64_t missed_count;
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

    void Str8ListPushNode(String8List* list, String8Node* n);
    void Str8ListPushNodeFront(String8List* list, String8Node* n);
    void Str8ListPush(Arena* arena, String8List* list, String8 str);
    void Str8ListPushF(Arena* arena, String8List* list, char* fmt, ...);
    void Str8ListPushFront(Arena* arena, String8List* list, String8 str);
    void Str8ListConcatInPlace(String8List* list, String8List* to_push);
    String8List StrSplit8(Arena* arena, String8 string, uint64_t split_count, String8* splits);
    String8 Str8ListJoin(Arena* arena, String8List list, StringJoin* optional_params);

    String8 Substr8(String8 str, Range1U64 rng);
    String8 Str8Skip(String8 str, uint64_t min);
    String8 Str8Chop(String8 str, uint64_t nmax);
    String8 Prefix8(String8 str, uint64_t size);
    String8 Suffix8(String8 str, uint64_t size);

    bool Str8Match(String8 a, String8 b, MatchFlags flags);
    uint64_t FindSubstr8(String8 haystack, String8 needle, uint64_t start_pos, MatchFlags flags);
    FuzzyMatchList FindFuzzy8(Arena* arena, String8 haystack, String8 needle, uint64_t start_pos, MatchFlags flags);

    String8 Str8SkipWhitespace(String8 str);
    String8 Str8ChopWhitespace(String8 str);
    String8 Str8SkipChopWhitespace(String8 str);
    String8 Str8SkipChopNewlines(String8 str);

    String8 Str8PathChopLastPeriod(String8 str);
    String8 Str8PathSkipLastSlash(String8 str);
    String8 Str8PathChopLastSlash(String8 str);
    String8 Str8PathSkipLastPeriod(String8 str);
    String8 Str8PathChopPastLastSlash(String8 str);

    PathType PathTypeFromStr8(String8 path);
    String8List PathPartsFromStr8(Arena* arena, String8 path);
    String8List AbsolutePathPartsFromSourcePartsType(Arena* arena, String8 source, String8List parts, PathType type);

    String8List DotResolvedPathPartsFromParts(Arena* arena, String8List parts);
    String8 NormalizedPathFromStr8(Arena* arena, String8 source, String8 path);
    String8 GetFileName(String8 str, bool directory = false);

    uint64_t U64FromStr8(String8 str, uint32_t radix);
    int64_t CStyleIntFromStr8(String8 str);
    double DoubleFromStr8(String8 str);
    String8 CStyleHexStringFromU64(Arena* arena, uint64_t x, bool caps);

    bool operator==(const String8& lhs, const String8& rhs);

#define Str8Struct(ptr) Str8((uint8_t*)(ptr), sizeof(*(ptr)))

#define Str8C(cstring) Str8((uint8_t*)(cstring), CalculateCStringLength(cstring))
#define Str8StdS(stdString) Str8((uint8_t*)(stdString.c_str()), stdString.size())
#define Str8Lit(s) Str8((uint8_t*)(s), sizeof(s) - 1)
#define Str8LitComp(s)               \
    {                                \
        (uint8_t*)(s), sizeof(s) - 1 \
    }

#define Str8VArg(s) (int)(s).size, (s).str
#define Str8ListFirst(list) ((list)->first != 0 ? (list)->first->string : Str8Lit(""))
#define ToStdString(s) std::string((const char*)s.str, s.size)
}
