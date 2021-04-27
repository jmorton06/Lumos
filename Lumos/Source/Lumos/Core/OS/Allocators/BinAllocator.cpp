#include "Precompiled.h"
#include "BinAllocator.h"

namespace Lumos
{
    void* BinAllocator::Malloc(size_t size, const char* file, int line)
    {
        int bin = BinForSize(size);
        if(m_Bins[bin] == nullptr)
            MakeChunk(size);
        if(m_Bins[bin] == nullptr)
            return nullptr;
        void* result = m_Bins[bin];
        m_Bins[bin] = *(void**)result;
        return result;
    }

    void BinAllocator::Free(void* location)
    {
        int bin = BinForLocation(location);
        *(void**)location = m_Bins[bin];
        m_Bins[bin] = location;
    }

    void* BinAllocator::Chunk(int num)
    {
        return num * BIN_SIZE + sizeof(BinAllocator) + (char*)this;
    }

    int BinAllocator::ChunkIndex(void* loc)
    {
        return (int)((char*)loc - sizeof(BinAllocator) - (char*)this) / BIN_SIZE;
    }

    int BinAllocator::BinForSize(size_t size)
    {
        return (int)((size - 1) / BIN_SIZE_INC);
    }

    size_t BinAllocator::SizeForBin(int bin)
    {
        return (bin + 1) * BIN_SIZE_INC;
    }

    int BinAllocator::BinForLocation(void* location)
    {
        int index = ChunkIndex(location);
        void* chunkStart = Chunk(index);
        return *(int*)chunkStart;
    }

    void BinAllocator::MakeChunk(size_t size)
    {
        if((m_Max + 2) * BIN_SIZE + sizeof(BinAllocator) > MEMORY_SIZE)
            return;

        int bin = BinForSize(size);
        size_t actualSize = SizeForBin(bin);

        char* chunkPtr = (char*)Chunk(m_Max++);
        *(int*)chunkPtr = bin;
        chunkPtr += actualSize;

        while(chunkPtr + actualSize <= Chunk(m_Max))
        {
            Free(chunkPtr);
            chunkPtr += actualSize;
        }
    }
}
