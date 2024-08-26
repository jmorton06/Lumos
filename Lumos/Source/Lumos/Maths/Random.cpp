#include "Precompiled.h"
#include "Random.h"
#include <random>

#define SUPPORT_RANDOM_DEVICE LUMOS_PLATFORM_WINDOWS

#if !SUPPORT_RANDOM_DEVICE
#include <chrono>
#endif

namespace Lumos
{
    Random32 Random32::Rand = Random32(Random32::RandSeed());
    Random64 Random64::Rand = Random64(Random64::RandSeed());

    uint32_t Random32::RandSeed()
    {
#if SUPPORT_RANDOM_DEVICE
        std::random_device randDevice;
        return randDevice();
#else
        // crushto32 function from https://gist.github.com/imneme/540829265469e673d045
        uint64_t result = uint64_t(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        result *= 0xbc2ad017d719504d;
        return uint32_t(result ^ (result >> 32));
#endif
    }

    uint64_t Random64::RandSeed()
    {
#if SUPPORT_RANDOM_DEVICE
        std::random_device randDevice;
        uint64_t value = randDevice();
        return (value << 32) | randDevice();
#else
        return uint64_t(std::chrono::high_resolution_clock::now().time_since_epoch().count());
#endif
    }

    class LUMOS_EXPORT RandomImp32
    {
    public:
        RandomImp32() = default;
        RandomImp32(uint32_t seed)
            : m_Engine(seed)
        {
        }

        RandomImp32(uint32_t seed, uint64_t skip)
            : m_Engine(seed)
        {
            m_Engine.discard(skip);
        }
        inline float operator()(float min, float max)
        {
            std::uniform_real_distribution<float> realDist(min, max);
            return realDist(m_Engine);
        }

        inline int32_t operator()(int32_t min, int32_t max)
        {
            std::uniform_int_distribution<int32_t> intDist(min, max);
            return intDist(m_Engine);
        }

        inline uint32_t operator()(uint32_t min, uint32_t max)
        {
            std::uniform_int_distribution<uint32_t> uintDist(min, max);
            return uintDist(m_Engine);
        }

        inline void Discard(uint64_t steps)
        {
            m_Engine.discard(steps);
        }

    private:
        std::mt19937 m_Engine;
    };

    class LUMOS_EXPORT RandomImp64
    {
    public:
        RandomImp64() = default;
        RandomImp64(uint64_t seed)
            : m_Engine(seed)
        {
        }

        RandomImp64(uint64_t seed, uint64_t skip)
            : m_Engine(seed)
        {
            m_Engine.discard(skip);
        }

        inline double operator()(double min, double max)
        {
            std::uniform_real_distribution<double> realDist(min, max);
            return realDist(m_Engine);
        }

        inline int64_t operator()(int64_t min, int64_t max)
        {
            std::uniform_int_distribution<int64_t> intDist(min, max);
            return intDist(m_Engine);
        }

        inline uint64_t operator()(uint64_t min, uint64_t max)
        {
            std::uniform_int_distribution<uint64_t> uintDist(min, max);
            return uintDist(m_Engine);
        }

        inline void Discard(uint64_t steps)
        {
            m_Engine.discard(steps);
        }

    private:
        std::mt19937_64 m_Engine;
    };

    Random32::Random32()
    {
        m_Impl = CreateUniquePtr<RandomImp32>();
    }

    Random32::Random32(uint32_t seed)
    {
        m_Impl = CreateUniquePtr<RandomImp32>(seed);
    }
    Random32::Random32(uint32_t seed, uint32_t skip)
    {
        m_Impl = CreateUniquePtr<RandomImp32>();
        m_Impl->Discard(skip);
    }

    float Random32::operator()(float min, float max)
    {
        return (*m_Impl)(min, max);
    }

    int32_t Random32::operator()(int32_t min, int32_t max)
    {
        return (*m_Impl)(min, max);
    }

    uint32_t Random32::operator()(uint32_t min, uint32_t max)
    {
        return (*m_Impl)(min, max);
    }

    void Random32::Discard(uint32_t steps)
    {
        m_Impl->Discard(steps);
    }

    Random64::Random64()
    {
        m_Impl = CreateUniquePtr<RandomImp64>();
    }
    Random64::Random64(uint64_t seed)
    {
        m_Impl = CreateUniquePtr<RandomImp64>(seed);
    }
    Random64::Random64(uint64_t seed, uint64_t skip)
    {
        m_Impl = CreateUniquePtr<RandomImp64>();
        m_Impl->Discard(skip);
    }

    double Random64::operator()(double min, double max)
    {
        return (*m_Impl)(min, max);
    }

    int64_t Random64::operator()(int64_t min, int64_t max)
    {
        return (*m_Impl)(min, max);
    }

    uint64_t Random64::operator()(uint64_t min, uint64_t max)
    {
        return (*m_Impl)(min, max);
    }

    void Random64::Discard(uint64_t steps)
    {
        m_Impl->Discard(steps);
    }
}
