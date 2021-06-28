#pragma once

#include "Core/Core.h"
#include <random>
#include <cstdint>

namespace Lumos
{

    class LUMOS_EXPORT Random32
    {
    public:
        inline Random32()
            : m_GenerationCount(0)
        {
        }
        inline Random32(uint32_t seed)
            : m_Engine(seed)
            , m_GenerationCount(0)
        {
        }
        inline Random32(uint32_t seed, uint64_t skip)
            : m_Engine(seed)
            , m_GenerationCount(skip)
        {
            m_Engine.discard(skip);
        }

        inline float operator()(float min, float max)
        {
            ++m_GenerationCount;
            std::uniform_real_distribution<float> realDist(min, max);
            return realDist(m_Engine);
        }

        inline int32_t operator()(int32_t min, int32_t max)
        {
            ++m_GenerationCount;
            std::uniform_int_distribution<int32_t> intDist(min, max);
            return intDist(m_Engine);
        }

        inline uint32_t operator()(uint32_t min, uint32_t max)
        {
            ++m_GenerationCount;
            std::uniform_int_distribution<uint32_t> uintDist(min, max);
            return uintDist(m_Engine);
        }

        inline uint64_t GetNumbersGenerated() const
        {
            return m_GenerationCount;
        }

        inline void Discard(uint64_t steps)
        {
            m_Engine.discard(steps);
            m_GenerationCount += steps;
        }

        static uint32_t RandSeed();
        static Random32 Rand;

    private:
        std::mt19937 m_Engine;

        uint64_t m_GenerationCount;
    };

    class LUMOS_EXPORT Random64
    {
    public:
        Random64()
            : m_GenerationCount(0)
        {
        }
        Random64(uint64_t seed)
            : m_Engine(seed)
            , m_GenerationCount(0)
        {
        }
        Random64(uint64_t seed, uint64_t skip)
            : m_Engine(seed)
            , m_GenerationCount(skip)
        {
            m_Engine.discard(skip);
        }

        inline double operator()(double min, double max)
        {
            ++m_GenerationCount;
            std::uniform_real_distribution<double> realDist(min, max);
            return realDist(m_Engine);
        }

        inline int64_t operator()(int64_t min, int64_t max)
        {
            ++m_GenerationCount;
            std::uniform_int_distribution<int64_t> intDist(min, max);
            return intDist(m_Engine);
        }

        inline uint64_t operator()(uint64_t min, uint64_t max)
        {
            ++m_GenerationCount;
            std::uniform_int_distribution<uint64_t> uintDist(min, max);
            return uintDist(m_Engine);
        }

        inline uint64_t GetNumbersGenerated() const
        {
            return m_GenerationCount;
        }

        inline void Discard(uint64_t steps)
        {
            m_Engine.discard(steps);
            m_GenerationCount += steps;
        }

        static uint64_t RandSeed();
        static Random64 Rand;

    private:
        std::mt19937_64 m_Engine;
        uint64_t m_GenerationCount;
    };
}
