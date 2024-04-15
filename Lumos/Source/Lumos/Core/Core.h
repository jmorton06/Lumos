#pragma once

#ifdef LUMOS_PLATFORM_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX // For windows.h
#endif
#endif

#ifndef LUMOS_PLATFORM_WINDOWS
#include <signal.h>
#endif

#include <stdint.h>
#include "Types.h"

#ifdef LUMOS_PLATFORM_WINDOWS

#define MEM_ALIGNMENT 16
#define MEM_ALIGN __declspec(align(MEM_ALIGNMENT))

#else

#define MEM_ALIGNMENT 16
#define MEM_ALIGN __attribute__((aligned(MEM_ALIGNMENT)))

#endif

#ifdef LUMOS_PLATFORM_WINDOWS
#pragma warning(disable : 4251)
#ifdef LUMOS_DYNAMIC
#ifdef LUMOS_ENGINE
#define LUMOS_EXPORT __declspec(dllexport)
#else
#define LUMOS_EXPORT __declspec(dllimport)
#endif
#else
#define LUMOS_EXPORT
#endif
#define LUMOS_HIDDEN
#else
#define LUMOS_EXPORT __attribute__((visibility("default")))
#define LUMOS_HIDDEN __attribute__((visibility("hidden")))
#endif

#define BIT(x) (1 << x)

#define NUMBONES 64
#define INPUT_BUF_SIZE 128

#define SAFE_DELETE(mem) \
    {                    \
        if(mem)          \
        {                \
            delete mem;  \
            mem = NULL;  \
        }                \
    }
#define SAFE_UNLOAD(mem, ...)         \
    {                                 \
        if(mem)                       \
        {                             \
            mem->Unload(__VA_ARGS__); \
            delete mem;               \
            mem = NULL;               \
        }                             \
    }

#ifdef LUMOS_DEBUG
#define LUMOS_DEBUG_METHOD(x) x;
#define LUMOS_DEBUG_METHOD_CALL(x) x;
#else
#define LUMOS_DEBUG_METHOD(x) \
    x                         \
    {                         \
    }
#define LUMOS_DEBUG_METHOD_CALL(x) ;
#endif

#define MAX_OBJECTS 2048

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

#if LUMOS_PLATFORM_WINDOWS
#define LUMOS_BREAK() __debugbreak()
#else
#define LUMOS_BREAK() raise(SIGTRAP)
#endif

#ifndef LUMOS_PRODUCTION
#define LUMOS_ENABLE_ASSERTS
#endif

#define HEX2CHR(m_hex) \
    ((m_hex >= '0' && m_hex <= '9') ? (m_hex - '0') : ((m_hex >= 'A' && m_hex <= 'F') ? (10 + m_hex - 'A') : ((m_hex >= 'a' && m_hex <= 'f') ? (10 + m_hex - 'a') : 0)))

#ifndef LUMOS_ENABLE_ASSERTS
#define LUMOS_ASSERT(...) ((void)0)
#else
#if LUMOS_ENABLE_LOG
#ifdef LUMOS_PLATFORM_UNIX
#define LUMOS_ASSERT(condition, ...)                                                                                                                                \
    do {                                                                                                                                                            \
        if(!(condition))                                                                                                                                            \
        {                                                                                                                                                           \
            LUMOS_LOG_ERROR("Assertion failed: {0}, file {1}, line {2}", #condition, __FILE__, __LINE__);                                                           \
            (::Lumos::Debug::Log::GetCoreLogger())->log(spdlog::source_loc { __FILE__, __LINE__, SPDLOG_FUNCTION }, spdlog::level::level_enum::err, ##__VA_ARGS__); \
            LUMOS_BREAK();                                                                                                                                          \
        }                                                                                                                                                           \
    } while(0)
#else
#define LUMOS_ASSERT(condition, ...)                                                                                                                              \
    do {                                                                                                                                                          \
        if(!(condition))                                                                                                                                          \
        {                                                                                                                                                         \
            LUMOS_LOG_ERROR("Assertion failed: {0}, file {1}, line {2}", #condition, __FILE__, __LINE__);                                                         \
            (::Lumos::Debug::Log::GetCoreLogger())->log(spdlog::source_loc { __FILE__, __LINE__, SPDLOG_FUNCTION }, spdlog::level::level_enum::err, __VA_ARGS__); \
            LUMOS_BREAK();                                                                                                                                        \
        }                                                                                                                                                         \
    } while(0)
#endif
#else
#define LUMOS_ASSERT(condition, ...) \
    if(!(condition))                 \
        LUMOS_BREAK();
#endif
#endif

#define UNIMPLEMENTED                                                     \
    {                                                                     \
        LUMOS_LOG_ERROR("Unimplemented : {0} : {1}", __FILE__, __LINE__); \
        LUMOS_BREAK();                                                    \
    }

#define NONCOPYABLE(class_name)                        \
    class_name(const class_name&)            = delete; \
    class_name& operator=(const class_name&) = delete;

#define NONCOPYABLEANDMOVE(class_name)                 \
    class_name(const class_name&)            = delete; \
    class_name& operator=(const class_name&) = delete; \
    class_name(class_name&&)                 = delete; \
    class_name& operator=(class_name&&)      = delete;

#if defined(_MSC_VER)
#define DISABLE_WARNING_PUSH __pragma(warning(push))
#define DISABLE_WARNING_POP __pragma(warning(pop))
#define DISABLE_WARNING(warningNumber) __pragma(warning(disable \
                                                        : warningNumber))

#define DISABLE_WARNING_UNREFERENCED_FORMAL_PARAMETER DISABLE_WARNING(4100)
#define DISABLE_WARNING_UNREFERENCED_FUNCTION DISABLE_WARNING(4505)
#define DISABLE_WARNING_CONVERSION_TO_SMALLER_TYPE DISABLE_WARNING(4267)

#elif defined(__GNUC__) || defined(__clang__)
#define DO_PRAGMA(X) _Pragma(#X)
#define DISABLE_WARNING_PUSH DO_PRAGMA(GCC diagnostic push)
#define DISABLE_WARNING_POP DO_PRAGMA(GCC diagnostic pop)
#define DISABLE_WARNING(warningName) DO_PRAGMA(GCC diagnostic ignored #warningName)

#define DISABLE_WARNING_UNREFERENCED_FORMAL_PARAMETER DISABLE_WARNING(-Wunused - parameter)
#define DISABLE_WARNING_UNREFERENCED_FUNCTION DISABLE_WARNING(-Wunused - function)
#define DISABLE_WARNING_CONVERSION_TO_SMALLER_TYPE DISABLE_WARNING(-Wconversion)

#else
#define DISABLE_WARNING_PUSH
#define DISABLE_WARNING_POP
#define DISABLE_WARNING_UNREFERENCED_FORMAL_PARAMETER
#define DISABLE_WARNING_UNREFERENCED_FUNCTION
#define DISABLE_WARNING_CONVERSION_TO_SMALLER_TYPE
#endif

#define LUMOS_SERIALISABLE(x, version) \
    CEREAL_CLASS_VERSION(x, version);  \
    CEREAL_REGISTER_TYPE_WITH_NAME(x, #x);

#define Bytes(n) (n)
#define Kilobytes(n) (n << 10)
#define Megabytes(n) (n << 20)
#define Gigabytes(n) (((uint64_t)n) << 30)
#define Terabytes(n) (((uint64_t)n) << 40)

#define ArrayCount(a) (sizeof(a) / sizeof((a)[0]))
#define IntFromPtr(p) (uint64_t)(((uint8_t*)p) - 0)
#define PtrFromInt(i) (void*)(((uint8_t*)0) + i)
#define MemberOf(type, member_name) ((type*)0)->member_name
#define OffsetOf(type, member_name) IntFromPtr(&MemberOf(type, member_name))
#define BaseFromMember(type, member_name, ptr) (type*)((uint8_t*)(ptr) - OffsetOf(type, member_name))

#define MemoryCopy memcpy
#define MemoryMove memmove
#define MemorySet memset

#define MemoryCopyStruct(dst, src)                      \
    do {                                                \
        LUMOS_ASSERT(sizeof(*(dst)) == sizeof(*(src))); \
        MemoryCopy((dst), (src), sizeof(*(dst)));       \
    } while(0)
#define MemoryCopyArray(dst, src)                 \
    do {                                          \
        LUMOS_ASSERT(sizeof(dst) == sizeof(src)); \
        MemoryCopy((dst), (src), sizeof(src));    \
    } while(0)

#define MemoryZero(ptr, size) MemorySet((ptr), 0, (size))
#define MemoryZeroStruct(ptr) MemoryZero((ptr), sizeof(*(ptr)))
#define MemoryZeroArray(arr) MemoryZero((arr), sizeof(arr))

#define LUMOS_UNUSED(x) (void)(x)
#define LUMOS_STRINGIFY(x) #x

#if defined(__GNUC__) || defined(__clang__)
#define LUMOS_DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER)
#define LUMOS_DEPRECATED(msg) __declspec(deprecated(msg))
#else
#define LUMOS_DEPRECATED(msg)
#endif

#define CheckNull(p) ((p) == 0)
#define SetIsNull(p) ((p) = 0)

#define QueuePush_NZ(f, l, n, next, zchk, zset) (zchk(f) ? (((f) = (l) = (n)), zset((n)->next)) : ((l)->next = (n), (l) = (n), zset((n)->next)))
#define QueuePushFront_NZ(f, l, n, next, zchk, zset) (zchk(f) ? (((f) = (l) = (n)), zset((n)->next)) : ((n)->next = (f)), ((f) = (n)))
#define QueuePop_NZ(f, l, next, zset) ((f) == (l) ? (zset(f), zset(l)) : ((f) = (f)->next))
#define StackPush_N(f, n, next) ((n)->next = (f), (f) = (n))
#define StackPop_NZ(f, next, zchk) (zchk(f) ? 0 : ((f) = (f)->next))

#define QueuePush(f, l, n) QueuePush_NZ(f, l, n, next, CheckNull, SetIsNull)
#define QueuePushFront(f, l, n) QueuePushFront_NZ(f, l, n, next, CheckNull, SetIsNull)
#define QueuePop(f, l) QueuePop_NZ(f, l, next, SetIsNull)
#define StackPush(f, n) StackPush_N(f, n, next)
#define StackPop(f) StackPop_NZ(f, next, CheckNull)

#if LUMOS_PLATFORM_WINDOWS
#define PerThread __declspec(thread)
#else
#define PerThread thread_local
#endif
