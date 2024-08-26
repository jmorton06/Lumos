#include "Precompiled.h"
#include "StbAllocator.h"

#define STB_LEAKCHECK_IMPLEMENTATION
#include <stb/stb_leakcheck.h>

namespace Lumos
{
    void* StbAllocator::Malloc(size_t size, const char* file, int line)
    {
        return stb_leakcheck_malloc(size, file, line);
    }

    void StbAllocator::Free(void* location)
    {
        stb_leakcheck_free(location);
    }

    void StbAllocator::Print()
    {
        stb_leakcheck_dumpmem();
    }
}
