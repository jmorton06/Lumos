#include "LM.h"
#include "Memory.h"
#include <new>

#ifdef LUMOS_LEAK_CHECK
#define STB_LEAKCHECK_IMPLEMENTATION
#include <stb/stb_leakcheck.h>

static void* newFunc(std::size_t size, const char *file, int line)
{
	void* p = stb_leakcheck_malloc(size, file, line);
	return p;
}

static void deleteFunc(void* p)
{
	stb_leakcheck_free(p);
}

void* operator new(std::size_t size)
{
	void* result = newFunc(size, __FILE__, __LINE__);
	if (result == nullptr)
	{
		throw std::bad_alloc();
	}
	return result;
}

void* operator new(std::size_t size, const char *file, int line)
{
	void* result = newFunc(size, file, line);
	if (result == nullptr)
	{
		throw std::bad_alloc();
	}
	return result;
}

void* operator new[](std::size_t size, const char *file, int line)
{
	void* result = newFunc(size, file, line);
	if (result == nullptr)
	{
		throw std::bad_alloc();
	}
	return result;
}

void* operator new (std::size_t size, const std::nothrow_t& nothrow_value) noexcept
{
	return newFunc(size, __FILE__, __LINE__);
}

void operator delete(void * p) throw()
{
	deleteFunc(p);
}

void* operator new[](std::size_t size)
{
	void* result = newFunc(size, __FILE__, __LINE__);
	if (result == nullptr)
	{
		throw std::bad_alloc();
	}
	return result;
}
	
void operator delete[](void *p) throw()
{
	deleteFunc(p);
}

#endif