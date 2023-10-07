#pragma once
#include "Core/OS/Memory.h"

namespace Lumos
{
    // Pool Allocator
    template <typename T>
    class PoolAllocator
    {
    public:
        explicit PoolAllocator(Arena* arena = nullptr, size_t poolSize = Megabytes(50))
            : m_Arena(arena)
            , m_PoolSize(poolSize)
            , m_NextAvailable(nullptr)
        {
            m_AlignSize = (sizeof(T) + alignof(T) - 1) & ~(alignof(T) - 1);

            if(!arena)
            {
                m_ArenaOwned = true;
                m_Arena      = ArenaAlloc(poolSize * 2);
            }

            LUMOS_ASSERT(m_Arena, "Arena not allocated");
            LUMOS_ASSERT(m_PoolSize >= sizeof(Node), "Pool size too small for type");

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
