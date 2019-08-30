#include "LM.h"
#include "Memory.h"
#include <new>

//#define USE_STB_LEAKCHECK
#ifdef USE_STB_LEAKCHECK

#define STB_LEAKCHECK_IMPLEMENTATION
#include <stb/stb_leakcheck.h>

#endif

namespace Lumos
{
    void* Memory::AlignedAlloc(size_t size, size_t alignment)
    {
        void *data = nullptr;
#if defined(LUMOS_PLATFORM_WINDOWS)
        data = _aligned_malloc(size, alignment);
#else
        int res = posix_memalign(&data, alignment, size);
        if (res != 0)
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
    
    void* Memory::NewFunc(std::size_t size, const char *file, int line)
    {
#ifdef USE_STB_LEAKCHECK
        void* p = stb_leakcheck_malloc(size, file, line);
#else
        void* p = malloc(size);
#endif
        return p;
    }
    
    void Memory::DeleteFunc(void* p)
    {
#ifdef USE_STB_LEAKCHECK
        stb_leakcheck_free(p);
#else
        free(p);
#endif
        
    }
    
    void Memory::LogMemoryInformation()
    {
#ifdef USE_STB_LEAKCHECK
        stb_leakcheck_dumpmem();
#endif
    }
}

#ifdef CUSTOM_MEMORY_ALLOCATOR
void* operator new(std::size_t size)
{
    void* result = Lumos::Memory::NewFunc(size, __FILE__, __LINE__);
    if (result == nullptr)
    {
        throw std::bad_alloc();
    }
    return result;
}

void* operator new(std::size_t size, const char *file, int line)
{
    void* result = Lumos::Memory::NewFunc(size, file, line);
    if (result == nullptr)
    {
        throw std::bad_alloc();
    }
    return result;
}

void* operator new[](std::size_t size, const char *file, int line)
{
    void* result = Lumos::Memory::NewFunc(size, file, line);
    if (result == nullptr)
    {
        throw std::bad_alloc();
    }
    return result;
}

void* operator new (std::size_t size, const std::nothrow_t& nothrow_value) noexcept
{
    return Lumos::Memory::NewFunc(size, __FILE__, __LINE__);
}

void operator delete(void * p) throw()
{
    Lumos::Memory::DeleteFunc(p);
}

void* operator new[](std::size_t size)
{
    void* result = Lumos::Memory::NewFunc(size, __FILE__, __LINE__);
    if (result == nullptr)
    {
        throw std::bad_alloc();
    }
    return result;
}

void operator delete[](void *p) throw()
{
    Lumos::Memory::DeleteFunc(p);
}

void operator delete(void* block, const char* file, int line)
{
    Lumos::Memory::DeleteFunc(block);
}

void operator delete[](void* block, const char* file, int line)
{
    Lumos::Memory::DeleteFunc(block);
}
#endif
