#pragma once

#include "Allocators/Allocator.h"

namespace Lumos
{
    class Memory
    {
    public:
        static void* AlignedAlloc(size_t size, size_t alignment);
        static void AlignedFree(void* data);

        static void* NewFunc(std::size_t size, const char* file, int line);
        static void DeleteFunc(void* p);
        static void LogMemoryInformation();

        static Allocator* const MemoryAllocator;
    };
}

#define CUSTOM_MEMORY_ALLOCATOR
#if defined(CUSTOM_MEMORY_ALLOCATOR) && defined(LUMOS_ENGINE)

void* operator new(std::size_t size);
//void* operator new(std::size_t size, const char *file, int line);
//void* operator new[](std::size_t size, const char *file, int line);
//void* operator new (std::size_t size, const std::nothrow_t& nothrow_value) noexcept;
void* operator new[](std::size_t size);

void operator delete(void* p) throw();
void operator delete[](void* p) throw();
//void operator delete(void* block, const char* file, int line);
//void operator delete[](void* block, const char* file, int line);

#endif
