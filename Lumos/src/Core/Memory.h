#pragma once
#include "LM.h"

namespace Lumos::Memory
{
	static inline void* AlignedAlloc(size_t size, size_t alignment)
	{
		void *data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
		data = _aligned_malloc(size, alignment);
#else
		int res = posix_memalign(&data, alignment, size);
		if (res != 0)
			data = nullptr;
#endif
		return data;
	}

	static inline void AlignedFree(void* data)
	{
#if defined(_MSC_VER) || defined(__MINGW32__)
		_aligned_free(data);
#else
		free(data);
#endif
	}

	LUMOS_EXPORT void* NewFunc(std::size_t size, const char *file, int line);
	LUMOS_EXPORT void DeleteFunc(void* p);
	LUMOS_EXPORT void LogMemoryInformation();
}

//#define CUSTOM_MEMORY_ALLOCATOR
#ifdef CUSTOM_MEMORY_ALLOCATOR

#define lmnew		new(__FILE__, __LINE__)
#define lmdel		delete

#pragma warning(disable : 4595)

LUMOS_EXPORT void* operator new(std::size_t size);

LUMOS_EXPORT void* operator new(std::size_t size, const char *file, int line);

LUMOS_EXPORT void* operator new[](std::size_t size, const char *file, int line);

LUMOS_EXPORT void* operator new (std::size_t size, const std::nothrow_t& nothrow_value) noexcept;

LUMOS_EXPORT void operator delete(void * p) throw();

LUMOS_EXPORT void* operator new[](std::size_t size);

LUMOS_EXPORT void operator delete[](void *p) throw();

LUMOS_EXPORT void operator delete(void* block, const char* file, int line);

LUMOS_EXPORT void operator delete[](void* block, const char* file, int line);

#pragma warning(default : 4595)

#else
#define lmnew new
#define lmdel delete
#endif
