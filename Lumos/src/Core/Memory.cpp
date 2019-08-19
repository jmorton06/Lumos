#include "LM.h"
#include "Memory.h"
#include <new>

//#define USE_STB_LEAKCHECK
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

