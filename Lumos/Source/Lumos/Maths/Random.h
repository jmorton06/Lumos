#pragma once

#include "Core/Core.h"
#include <cstdint>

namespace Lumos
{
    class RandomImp32;
    class RandomImp64;

    class LUMOS_EXPORT Random32
    {
    public:
        Random32();
        Random32(uint32_t seed);
        Random32(uint32_t seed, uint32_t skip);

        double operator()(float min, float max);
        int32_t operator()(int32_t min, int32_t max);
        uint32_t operator()(uint32_t min, uint32_t max);
        void Discard(uint32_t steps);

        static uint32_t RandSeed();
        static Random32 Rand;

    private:
        UniquePtr<RandomImp32> m_Impl;
    };

    class LUMOS_EXPORT Random64
    {
    public:
        Random64();
        Random64(uint64_t seed);
        Random64(uint64_t seed, uint64_t skip);

        double operator()(double min, double max);
        int64_t operator()(int64_t min, int64_t max);
        uint64_t operator()(uint64_t min, uint64_t max);
        void Discard(uint64_t steps);

        static uint64_t RandSeed();
        static Random64 Rand;

    private:
        UniquePtr<RandomImp64> m_Impl;
    };
}
