#include "Precompiled.h"
#include "DefaultAllocator.h"

#include "Core/OS/MemoryManager.h"
//#define TRACK_ALLOCATIONS

#define LUMOS_MEMORY_ALIGNMENT 16
#define LUMOS_ALLOC(size)	_aligned_malloc(size, LUMOS_MEMORY_ALIGNMENT)
#define LUMOS_FREE(block)	_aligned_free(block);

namespace Lumos
{
	void* DefaultAllocator::Malloc(size_t size, const char * file, int line)
	{
#ifdef TRACK_ALLOCATIONS
		LUMOS_ASSERT(size < 1024 * 1024 * 1024, "Allocation more than max size");

		Lumos::MemoryManager::Get()->m_MemoryStats.totalAllocated += size;
		Lumos::MemoryManager::Get()->m_MemoryStats.currentUsed += size;
		Lumos::MemoryManager::Get()->m_MemoryStats.totalAllocations++;

		size_t actualSize = size + sizeof(size_t);
		void *mem = malloc(actualSize + sizeof(void*));

		uint8_t *result = (uint8_t*)mem;
		if (result == NULL)
		{
			LUMOS_LOG_ERROR("Aligned malloc failed");
			return NULL;
		}

		memset(result, 0, actualSize);
		memcpy(result, &size, sizeof(size_t));
		result += sizeof(size_t);

		return result;

#else
		return malloc(size);
#endif
	}

	void DefaultAllocator::Free(void* location)
	{
#ifdef TRACK_ALLOCATIONS
		u8* memory = ((u8*)location) - sizeof(size_t);
		if (location && memory)
		{
			uint8_t *memory = ((uint8_t*)location) - sizeof(size_t);
			size_t size = *(size_t*)memory;
			free(((void**)memory));
			Lumos::MemoryManager::Get()->m_MemoryStats.totalFreed += size;
			Lumos::MemoryManager::Get()->m_MemoryStats.currentUsed -= size;
		}
		else
		{
			free(location);
		}
	
#else
		free(location);
#endif
	}
}