#include "Precompiled.h"
#include "Memory.h"
#include "Allocators/BinAllocator.h"
#include "Allocators/DefaultAllocator.h"
#include "Allocators/StbAllocator.h"

namespace Lumos
{
    Allocator* const Memory::MemoryAllocator = new DefaultAllocator();

    void* Memory::AlignedAlloc(size_t size, size_t alignment)
    {
        void* data;
#if defined(LUMOS_PLATFORM_WINDOWS)
        data = _aligned_malloc(size, alignment);
#else
        int res = posix_memalign(&data, alignment, size);
        if(res != 0)
            data = nullptr;
#endif
        return data;
    }

    void Memory::AlignedFree(void* data)
    {
#if defined(LUMOS_PLATFORM_WINDOWS)
        _aligned_free(data);
#else
        free(data);
#endif
    }

    void* Memory::NewFunc(std::size_t size, const char* file, int line)
    {
        if(MemoryAllocator)
            return MemoryAllocator->Malloc(size, file, line);
        else
            return malloc(size);
    }

    void Memory::DeleteFunc(void* p)
    {
        if(MemoryAllocator)
            return MemoryAllocator->Free(p);
        else
            return free(p);
    }

    void Memory::LogMemoryInformation()
    {
        if(MemoryAllocator)
            return MemoryAllocator->Print();
    }
}

#ifdef CUSTOM_MEMORY_ALLOCATOR

void* operator new(std::size_t size)
{
    void* result = Lumos::Memory::NewFunc(size, __FILE__, __LINE__);
    if(result == nullptr)
    {
        throw std::bad_alloc();
    }
#if defined(LUMOS_PROFILE) && defined(TRACY_ENABLE)
    TracyAlloc(result, size);
#endif
    return result;
}

void* operator new[](std::size_t size)
{
    void* result = Lumos::Memory::NewFunc(size, __FILE__, __LINE__);
    if(result == nullptr)
    {
        throw std::bad_alloc();
    }
#if defined(LUMOS_PROFILE) && defined(TRACY_ENABLE)
    TracyAlloc(result, size);
#endif
    return result;
}

void operator delete(void* p) throw()
{
#if defined(LUMOS_PROFILE) && defined(TRACY_ENABLE)
    TracyFree(p);
#endif
    Lumos::Memory::DeleteFunc(p);
}

void operator delete[](void* p) throw()
{
#if defined(LUMOS_PROFILE) && defined(TRACY_ENABLE)
    TracyFree(p);
#endif
    Lumos::Memory::DeleteFunc(p);
}
#endif
