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

	void* NewFunc(std::size_t size, const char *file, int line);
	void DeleteFunc(void* p);
	LUMOS_EXPORT void LogMemoryInformation();
}

//#define LUMOS_LEAK_CHECK

#ifdef LUMOS_LEAK_CHECK
extern const char* __file__;
extern size_t __line__;
#define new (__file__=__FILE__,__line__=__LINE__) && 0 ? NULL : new
#endif
