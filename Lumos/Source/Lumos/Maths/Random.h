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
        explicit Random32();
        explicit Random32(uint32_t seed);
        Random32(uint32_t seed, uint32_t skip);

        ~Random32();

        Random32(const Random32&)            = delete;
        Random32& operator=(const Random32&) = delete;

        Random32(Random32&&) noexcept            = default;
        Random32& operator=(Random32&&) noexcept = default;

        [[nodiscard]] float operator()(float min, float max);
        [[nodiscard]] int32_t operator()(int32_t min, int32_t max);
        [[nodiscard]] uint32_t operator()(uint32_t min, uint32_t max);

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

        ~Random64();

        Random64(const Random64&)            = delete;
        Random64& operator=(const Random64&) = delete;

        Random64(Random64&&) noexcept            = default;
        Random64& operator=(Random64&&) noexcept = default;

        [[nodiscard]] double operator()(double min, double max);
        [[nodiscard]] int64_t operator()(int64_t min, int64_t max);
        [[nodiscard]] uint64_t operator()(uint64_t min, uint64_t max);

        void Discard(uint64_t steps);

        static uint64_t RandSeed();
        static Random64 Rand;

    private:
        UniquePtr<RandomImp64> m_Impl;
    };
}
