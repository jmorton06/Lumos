#pragma once
#include "Core/Core.h"

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace Lumos
{
    namespace Detail
    {
        inline int PopCount64(uint64_t x)
        {
#ifdef _MSC_VER
            return (int)__popcnt64(x);
#else
            return __builtin_popcountll(x);
#endif
        }

        inline int CountTrailingZeros64(uint64_t x)
        {
#ifdef _MSC_VER
            unsigned long idx;
            _BitScanForward64(&idx, x);
            return (int)idx;
#else
            return __builtin_ctzll(x);
#endif
        }
    }

    template <uint32_t N>
    class BitSet
    {
        static constexpr uint32_t NumWords = (N + 63) / 64;

    public:
        BitSet()
        {
            MemoryZeroArray(m_Words);
        }

        void Set(uint32_t bit)
        {
            ASSERT(bit < N, "BitSet index out of range");
            m_Words[bit / 64] |= (1ULL << (bit % 64));
        }

        void Clear(uint32_t bit)
        {
            ASSERT(bit < N, "BitSet index out of range");
            m_Words[bit / 64] &= ~(1ULL << (bit % 64));
        }

        void Toggle(uint32_t bit)
        {
            ASSERT(bit < N, "BitSet index out of range");
            m_Words[bit / 64] ^= (1ULL << (bit % 64));
        }

        bool Test(uint32_t bit) const
        {
            ASSERT(bit < N, "BitSet index out of range");
            return (m_Words[bit / 64] & (1ULL << (bit % 64))) != 0;
        }

        void ClearAll()
        {
            MemoryZeroArray(m_Words);
        }

        void SetAll()
        {
            MemorySet(m_Words, 0xFF, sizeof(m_Words));
            // Mask off unused bits in last word
            if constexpr(N % 64 != 0)
                m_Words[NumWords - 1] &= (1ULL << (N % 64)) - 1;
        }

        int Count() const
        {
            int c = 0;
            for(uint32_t i = 0; i < NumWords; ++i)
                c += Detail::PopCount64(m_Words[i]);
            return c;
        }

        bool Any() const
        {
            for(uint32_t i = 0; i < NumWords; ++i)
                if(m_Words[i])
                    return true;
            return false;
        }

        bool None() const { return !Any(); }

        bool All() const { return Count() == (int)N; }

        BitSet operator&(const BitSet& other) const
        {
            BitSet result;
            for(uint32_t i = 0; i < NumWords; ++i)
                result.m_Words[i] = m_Words[i] & other.m_Words[i];
            return result;
        }

        BitSet operator|(const BitSet& other) const
        {
            BitSet result;
            for(uint32_t i = 0; i < NumWords; ++i)
                result.m_Words[i] = m_Words[i] | other.m_Words[i];
            return result;
        }

        BitSet operator^(const BitSet& other) const
        {
            BitSet result;
            for(uint32_t i = 0; i < NumWords; ++i)
                result.m_Words[i] = m_Words[i] ^ other.m_Words[i];
            return result;
        }

        BitSet operator~() const
        {
            BitSet result;
            for(uint32_t i = 0; i < NumWords; ++i)
                result.m_Words[i] = ~m_Words[i];
            if constexpr(N % 64 != 0)
                result.m_Words[NumWords - 1] &= (1ULL << (N % 64)) - 1;
            return result;
        }

        BitSet& operator&=(const BitSet& other)
        {
            for(uint32_t i = 0; i < NumWords; ++i)
                m_Words[i] &= other.m_Words[i];
            return *this;
        }

        BitSet& operator|=(const BitSet& other)
        {
            for(uint32_t i = 0; i < NumWords; ++i)
                m_Words[i] |= other.m_Words[i];
            return *this;
        }

        BitSet& operator^=(const BitSet& other)
        {
            for(uint32_t i = 0; i < NumWords; ++i)
                m_Words[i] ^= other.m_Words[i];
            return *this;
        }

        bool operator==(const BitSet& other) const
        {
            for(uint32_t i = 0; i < NumWords; ++i)
                if(m_Words[i] != other.m_Words[i])
                    return false;
            return true;
        }

        bool operator!=(const BitSet& other) const { return !(*this == other); }

        // Iterates over set bit indices
        class SetBitIterator
        {
        public:
            SetBitIterator(const uint64_t* words, uint32_t wordIdx)
                : m_Words(words)
                , m_WordIdx(wordIdx)
                , m_Current(wordIdx < NumWords ? words[wordIdx] : 0)
            {
                Advance();
            }

            bool operator!=(const SetBitIterator& other) const
            {
                return m_WordIdx != other.m_WordIdx || m_Current != other.m_Current;
            }

            uint32_t operator*() const { return m_Bit; }

            SetBitIterator& operator++()
            {
                m_Current &= m_Current - 1; // clear lowest set bit
                Advance();
                return *this;
            }

        private:
            void Advance()
            {
                while(m_Current == 0 && m_WordIdx < NumWords - 1)
                {
                    ++m_WordIdx;
                    m_Current = m_Words[m_WordIdx];
                }
                if(m_Current != 0)
                    m_Bit = m_WordIdx * 64 + Detail::CountTrailingZeros64(m_Current);
                else
                    m_WordIdx = NumWords; // sentinel
            }

            const uint64_t* m_Words;
            uint32_t m_WordIdx;
            uint64_t m_Current;
            uint32_t m_Bit = 0;
        };

        SetBitIterator begin() const { return SetBitIterator(m_Words, 0); }
        SetBitIterator end() const { return SetBitIterator(m_Words, NumWords); }

    private:
        uint64_t m_Words[NumWords];
    };
}
