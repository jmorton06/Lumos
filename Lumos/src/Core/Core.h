#pragma once

#ifndef LUMOS_PLATFORM_WINDOWS
#include <signal.h>
#endif

#include <stdint.h>

#ifdef LUMOS_PLATFORM_WINDOWS

#define MEM_ALIGNMENT 16
#define MEM_ALIGN __declspec(align(MEM_ALIGNMENT))

#else

#define MEM_ALIGNMENT 16
#define MEM_ALIGN  __attribute__((aligned(MEM_ALIGNMENT)))

#endif

#ifdef LUMOS_PLATFORM_WINDOWS
#pragma warning (disable:4251)
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
#define LUMOS_EXPORT __attribute__ ((visibility ("default")))
#define LUMOS_HIDDEN __attribute__ ((visibility ("hidden")))
#endif

#define BIT(x) (1 << x)

#define NUMBONES 64
#define INPUT_BUF_SIZE 128

#define SAFE_DELETE(mem) { if(mem) { delete mem; mem = NULL; } }
#define SAFE_UNLOAD(mem, ...) { if(mem) { mem->Unload(__VA_ARGS__); delete mem; mem = NULL; } }

#ifdef LUMOS_DEBUG
#define LUMOS_DEBUG_METHOD(x) x;
#define LUMOS_DEBUG_METHOD_CALL(x) x;
#else
#define LUMOS_DEBUG_METHOD(x) x {}
#define LUMOS_DEBUG_METHOD_CALL(x);
#endif

#define MAX_OBJECTS 2048

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)
#define ROOT_DIR STRINGIZE(LUMOS_ROOT_DIR)

#if LUMOS_PLATFORM_WINDOWS
#define LUMOS_BREAK() __debugbreak(); 
#else
#define LUMOS_BREAK() raise(SIGTRAP);
#endif

#ifdef LUMOS_DEBUG
	#define LUMOS_ENABLE_ASSERTS
#endif

#ifndef _ALWAYS_INLINE_

#if defined(__GNUC__) && (__GNUC__ >= 4)
#define _ALWAYS_INLINE_ __attribute__((always_inline)) _FORCE_INLINE_
#elif defined(__llvm__)
#define _ALWAYS_INLINE_ __attribute__((always_inline)) _FORCE_INLINE_
#elif defined(_MSC_VER)
#define _ALWAYS_INLINE_ __forceinline
#else
#define _ALWAYS_INLINE_ _FORCE_INLINE_
#endif

#endif

//should always _FORCE_INLINE_, except in some cases because it makes debugging harder
#ifndef _FORCE_INLINE_

#ifdef DISABLE_FORCED_INLINE
#define _FORCE_INLINE_ _FORCE_INLINE_
#else
#define _FORCE_INLINE_ _ALWAYS_INLINE_
#endif

#endif

#define HEX2CHR(m_hex) \
((m_hex >= '0' && m_hex <= '9') ? (m_hex - '0') : \
((m_hex >= 'A' && m_hex <= 'F') ? (10 + m_hex - 'A') : \
((m_hex >= 'a' && m_hex <= 'f') ? (10 + m_hex - 'a') : 0)))

#ifdef LUMOS_ENABLE_ASSERTS

	#define LUMOS_ASSERT_NO_MESSAGE(condition)		\
	{												\
		if(!(condition))							\
		{											\
			LUMOS_LOG_ERROR("Assertion Failed!");	\
			LUMOS_BREAK();							\
		}											\
	}												

	#define LUMOS_ASSERT_MESSAGE(condition, ...)					\
	{																\
		if(!(condition))											\
		{															\
			LUMOS_LOG_ERROR("Assertion Failed : {0}", __VA_ARGS__);	\
			LUMOS_BREAK();											\
		}															\
	}																

	#define LUMOS_CLIENT_ASSERT	 LUMOS_ASSERT_MESSAGE
	#define LUMOS_CORE_ASSERT		 LUMOS_ASSERT_MESSAGE
#else
	#define LUMOS_CLIENT_ASSERT(...)
	#define LUMOS_CORE_ASSERT(...)
#endif

#ifdef LUMOS_ENGINE
	#define LUMOS_ASSERT LUMOS_CORE_ASSERT
#else
	#define LUMOS_ASSERT LUMOS_CLIENT_ASSERT
#endif

#define UNIMPLEMENTED	 												\
{																		\
	LUMOS_LOG_ERROR("Unimplemented : {0} : {1}", __FILE__, __LINE__); 	\
	LUMOS_BREAK();  													\
}

#define NONCOPYABLE(type_identifier)								\
    type_identifier(const type_identifier&) = delete;				\
    type_identifier& operator=(const type_identifier&) = delete;

#define Lumos_SSE 

namespace Lumos
{
    namespace detail
    {
        template <template <typename> typename Op, typename T, typename = void>
        struct is_detected : std::false_type {};
        
        template <template <typename> typename Op, typename T>
        struct is_detected<Op, T, std::void_t<Op<T>>> : std::true_type {};
    }
    template <template <typename> typename Op, typename T>
    static constexpr bool is_detected_v = detail::is_detected<Op, T>::value;
}
