#include "Precompiled.h"
#include "RandomNumberGenerator.h"

#define SUPPORT_RANDOM_DEVICE LUMOS_PLATFORM_WINDOWS

#if !SUPPORT_RANDOM_DEVICE
#include <chrono>
#endif

namespace Lumos
{
    RandomNumberGenerator32 RandomNumberGenerator32::Rand = RandomNumberGenerator32(RandomNumberGenerator32::RandSeed());
    RandomNumberGenerator64 RandomNumberGenerator64::Rand = RandomNumberGenerator64(RandomNumberGenerator64::RandSeed());

    uint32_t RandomNumberGenerator32::RandSeed()
    {
#if SUPPORT_RANDOM_DEVICE
        std::random_device randDevice;
        return randDevice();
#else
        //crushto32 function from https://gist.github.com/imneme/540829265469e673d045
        uint64_t result = uint64_t(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        result *= 0xbc2ad017d719504d;
        return uint32_t(result ^ (result >> 32));
#endif
    }

    uint64_t RandomNumberGenerator64::RandSeed()
    {
#if SUPPORT_RANDOM_DEVICE
        std::random_device randDevice;
        uint64_t value = randDevice();
        return (value << 32) | randDevice();
#else
        return uint64_t(std::chrono::high_resolution_clock::now().time_since_epoch().count());
#endif
    }
}
