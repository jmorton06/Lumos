#pragma once
#include "Allocator.h"

#define MEMORY_SIZE 10*1024*1024
#define BIN_SIZE 1024
#define NUM_BINS 128
#define BIN_SIZE_INC 16

namespace Lumos
{
	class LUMOS_HIDDEN BinAllocator : public Allocator
	{
	public:
		BinAllocator() : m_Max(0)
		{
			for (int i = 0; i < NUM_BINS; i++)
				m_Bins[i] = nullptr;
		}

		~BinAllocator() = default;

		void* Malloc(size_t size, const char* file, int line) override;
		void Free(void* location) override;

		void* operator new (size_t size) 
		{
			return ::malloc(MEMORY_SIZE);		}
	private:
		
		BinAllocator(const BinAllocator& other) {};		BinAllocator operator =(const BinAllocator & other)
		{
			return *this;		}
		void* Chunk(int num);
		int ChunkIndex(void* loc);
		int BinForSize(size_t size);
		size_t SizeForBin(int bin);
		int BinForLocation(void * location);
		void MakeChunk(size_t size);
		
		int m_Max;
		void* m_Bins[NUM_BINS];
	};

}

