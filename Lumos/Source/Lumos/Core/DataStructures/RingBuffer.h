#pragma once
#include "Core/Core.h"
#include <atomic>

namespace Lumos
{
    // Single-producer single-consumer lock-free ring buffer.
    // N must be a power of 2.
    template <typename T, uint32_t N>
    class RingBuffer
    {
        static_assert((N & (N - 1)) == 0, "RingBuffer size must be a power of 2");
        static_assert(N > 0, "RingBuffer size must be > 0");

    public:
        RingBuffer()
            : m_ReadPos(0)
            , m_WritePos(0)
        {
        }

        bool PushBack(const T& item)
        {
            uint32_t w = m_WritePos.load(std::memory_order_relaxed);
            uint32_t r = m_ReadPos.load(std::memory_order_acquire);
            if(w - r >= N)
                return false;
            m_Data[w & (N - 1)] = item;
            m_WritePos.store(w + 1, std::memory_order_release);
            return true;
        }

        bool PushBack(T&& item)
        {
            uint32_t w = m_WritePos.load(std::memory_order_relaxed);
            uint32_t r = m_ReadPos.load(std::memory_order_acquire);
            if(w - r >= N)
                return false;
            m_Data[w & (N - 1)] = Move(item);
            m_WritePos.store(w + 1, std::memory_order_release);
            return true;
        }

        bool PopFront(T& out)
        {
            uint32_t r = m_ReadPos.load(std::memory_order_relaxed);
            uint32_t w = m_WritePos.load(std::memory_order_acquire);
            if(r == w)
                return false;
            out = Move(m_Data[r & (N - 1)]);
            m_ReadPos.store(r + 1, std::memory_order_release);
            return true;
        }

        bool Empty() const
        {
            uint32_t r = m_ReadPos.load(std::memory_order_acquire);
            uint32_t w = m_WritePos.load(std::memory_order_acquire);
            return r == w;
        }

        bool Full() const
        {
            uint32_t r = m_ReadPos.load(std::memory_order_acquire);
            uint32_t w = m_WritePos.load(std::memory_order_acquire);
            return w - r >= N;
        }

        uint32_t Size() const
        {
            uint32_t r = m_ReadPos.load(std::memory_order_acquire);
            uint32_t w = m_WritePos.load(std::memory_order_acquire);
            return w - r;
        }

    private:
        T m_Data[N];
        alignas(64) std::atomic<uint32_t> m_ReadPos;
        alignas(64) std::atomic<uint32_t> m_WritePos;
    };
}
