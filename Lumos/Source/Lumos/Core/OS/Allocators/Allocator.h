#pragma once

namespace Lumos
{
    class Allocator
    {
    public:
        virtual void* Malloc(size_t size, const char* file, int line) = 0;
        virtual void Free(void* location)                             = 0;
        virtual void Print() { }
    };
}
