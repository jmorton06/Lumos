#pragma once
#include "LM.h"

namespace Lumos::Memory
{
    LUMOS_EXPORT void* AlignedAlloc(size_t size, size_t alignment);
    LUMOS_EXPORT void AlignedFree(void* data);

	LUMOS_EXPORT void* NewFunc(std::size_t size, const char *file, int line);
	LUMOS_EXPORT void DeleteFunc(void* p);
	LUMOS_EXPORT void LogMemoryInformation();
}
#define CUSTOM_MEMORY_ALLOCATOR
#if  defined(CUSTOM_MEMORY_ALLOCATOR) && defined(LUMOS_ENGINE)

#define lmnew		new(__FILE__, __LINE__)
#define lmdel		delete

LUMOS_EXPORT void* operator new(std::size_t size);
LUMOS_EXPORT void* operator new(std::size_t size, const char *file, int line);
LUMOS_EXPORT void* operator new[](std::size_t size, const char *file, int line);
LUMOS_EXPORT void* operator new (std::size_t size, const std::nothrow_t& nothrow_value) noexcept;
LUMOS_EXPORT void* operator new[](std::size_t size);

LUMOS_EXPORT void operator delete(void * p) throw();
LUMOS_EXPORT void operator delete[](void *p) throw();
LUMOS_EXPORT void operator delete(void* block, const char* file, int line);
LUMOS_EXPORT void operator delete[](void* block, const char* file, int line);

#else
#define lmnew new
#define lmdel delete
#endif
