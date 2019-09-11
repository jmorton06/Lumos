#include "lmpch.h"
#include "DefaultAllocator.h"

namespace Lumos
{
	void* DefaultAllocator::Malloc(size_t size, const char * file, int line)
	{
		return malloc(size);
	}

	void DefaultAllocator::Free(void* location)
	{
		free(location);
	}
}