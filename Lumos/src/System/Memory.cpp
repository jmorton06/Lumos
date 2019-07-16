#include "LM.h"
#include "Memory.h"
#include <new>

#ifdef USE_STB_LEAKCHECK

#define STB_LEAKCHECK_IMPLEMENTATION
#include <stb/stb_leakcheck.h>

#endif

void* Lumos::Memory::NewFunc(std::size_t size, const char *file, int line)
{
#ifdef USE_STB_LEAKCHECK
	void* p = stb_leakcheck_malloc(size, file, line);
#else
	void* p = malloc(size);
#endif
	return p;
}

void Lumos::Memory::DeleteFunc(void* p)
{
#ifdef USE_STB_LEAKCHECK
	stb_leakcheck_free(p);
#else
	free(p);
#endif

}

void Lumos::Memory::LogMemoryInformation()
{
#ifdef USE_STB_LEAKCHECK
	stb_leakcheck_dumpmem();
#endif
}

#ifdef LUMOS_LEAK_CHECK

const char* __file__ = "unknown";
size_t __line__ = 0;

#undef new

void* operator new(std::size_t size)
{
	void* result = Lumos::Memory::NewFunc(size, __file__, __line__);
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
	return Lumos::Memory::NewFunc(size, __file__, __line__);
}

void operator delete(void * p) throw()
{
	Lumos::Memory::DeleteFunc(p);
}

void* operator new[](std::size_t size)
{
	void* result = Lumos::Memory::NewFunc(size, __file__, __line__);
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

#endif