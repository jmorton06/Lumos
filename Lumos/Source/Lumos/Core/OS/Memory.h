#pragma once

#include "Core/Core.h"
#include "Allocators/Allocator.h"

#include <cstdint>
#include <cstdlib>
#include <cassert>

namespace Lumos
{
    class Memory
    {
    public:
        static void* AlignedAlloc(size_t size, size_t alignment);
        static void AlignedFree(void* data);

        static void* NewFunc(std::size_t size, const char* file, int line);
        static void DeleteFunc(void* p);
        static void LogMemoryInformation();

        static Allocator* const MemoryAllocator;
    };

    struct Arena
    {
        uint64_t Position;
        uint64_t CommitPosition;
        uint64_t Align;
        uint64_t Size;
        Arena* Ptr;
        uint64_t _unused_[3];
    };

    struct ArenaTemp
    {
        Arena* arena;
        uint64_t pos;
    };

    Arena* ArenaAlloc(uint64_t size);
    Arena* ArenaAllocDefault();
    void ArenaRelease(Arena* arena);
    void* ArenaPushNoZero(Arena* arena, uint64_t size);
    void* ArenaPushAligner(Arena* arena, uint64_t alignment);
    void* ArenaPush(Arena* arena, uint64_t size);
    void ArenaPopTo(Arena* arena, uint64_t pos);
    void ArenaSetAutoAlign(Arena* arena, uint64_t align);
    void ArenaPop(Arena* arena, uint64_t size);
    void ArenaClear(Arena* arena);
    uint64_t ArenaPos(Arena* arena);
    ArenaTemp ArenaTempBegin(Arena* arena);
    void ArenaTempEnd(ArenaTemp temp);

#define ArenaTempBlock(arena, name) \
    ArenaTemp name = { 0 };         \
    DeferLoop(name = ArenaTempBegin(arena), ArenaTempEnd(name))

    // Pool Allocator
    template <typename T>
    class PoolAllocator
    {
    public:
        explicit PoolAllocator(Arena* arena = nullptr, size_t poolSize = 4096)
            : m_Arena(arena)
            , m_PoolSize(poolSize)
            , m_NextAvailable(nullptr)
        {
            assert(m_Arena);
            assert(m_PoolSize >= sizeof(Node));

            if(!arena)
            {
                m_ArenaOwned = true;
                m_Arena      = ArenaAlloc(Megabytes(50));
            }

            m_AlignSize = (sizeof(T) + alignof(T) - 1) & ~(alignof(T) - 1);

            // Calculate the number of elements that fit in a pool
            m_ElementsPerPool = (m_PoolSize - sizeof(Node)) / m_AlignSize;

            AllocateNewPool();
        }

        ~PoolAllocator()
        {
            while(m_HeadPool)
            {
                Node* nextPool    = m_HeadPool->next;
                m_Arena->Position = reinterpret_cast<uint64_t>(m_HeadPool);
                m_HeadPool        = nextPool;
            }

            if(m_ArenaOwned)
                ArenaRelease(m_Arena);
        }

        T* Allocate()
        {
            if(!m_NextAvailable)
            {
                AllocateNewPool();
            }

            Node* node      = m_NextAvailable;
            m_NextAvailable = m_NextAvailable->next;

            return reinterpret_cast<T*>(node);
        }

        void Deallocate(T* ptr)
        {
            Node* node      = reinterpret_cast<Node*>(ptr);
            node->next      = m_NextAvailable;
            m_NextAvailable = node;
        }

    private:
        struct Node
        {
            Node* next;
        };

        Arena* m_Arena;
        size_t m_PoolSize;
        size_t m_AlignSize;
        size_t m_ElementsPerPool;
        Node* m_HeadPool;
        Node* m_NextAvailable;
        bool m_ArenaOwned = false;

        void AllocateNewPool()
        {
            void* poolMemory = ArenaPush(m_Arena, m_PoolSize);
            Node* pool       = reinterpret_cast<Node*>(poolMemory);
            m_HeadPool       = pool;

            // Create the linked list of available nodes in the pool
            for(size_t i = 0; i < m_ElementsPerPool - 1; ++i)
            {
                pool->next = reinterpret_cast<Node*>(reinterpret_cast<uintptr_t>(pool) + m_AlignSize);
                pool       = pool->next;
            }

            pool->next      = nullptr;
            m_NextAvailable = m_HeadPool;
        }
    };

}

#define CUSTOM_MEMORY_ALLOCATOR
#if defined(CUSTOM_MEMORY_ALLOCATOR) && defined(LUMOS_ENGINE)

void* operator new(std::size_t size);
// void* operator new(std::size_t size, const char *file, int line);
// void* operator new[](std::size_t size, const char *file, int line);
// void* operator new (std::size_t size, const std::nothrow_t& nothrow_value) noexcept;
void* operator new[](std::size_t size);

void operator delete(void* p) throw();
void operator delete[](void* p) throw();
// void operator delete(void* block, const char* file, int line);
// void operator delete[](void* block, const char* file, int line);

#endif
