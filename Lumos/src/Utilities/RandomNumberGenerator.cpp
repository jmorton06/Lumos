#include "lmpch.h"
#include "RandomNumberGenerator.h"

#define SUPPORT_RANDOM_DEVICE LUMOS_PLATFORM_WINDOWS

#if !SUPPORT_RANDOM_DEVICE
#include <chrono>
#endif

namespace Lumos
{
	RandomNumberGenerator32 RandomNumberGenerator32::Rand = RandomNumberGenerator32(RandomNumberGenerator32::RandSeed());
	RandomNumberGenerator64 RandomNumberGenerator64::Rand = RandomNumberGenerator64(RandomNumberGenerator64::RandSeed());

	u32 RandomNumberGenerator32::RandSeed()
	{
#if SUPPORT_RANDOM_DEVICE
		std::random_device randDevice;
		return randDevice();
#else
		//crushto32 function from https://gist.github.com/imneme/540829265469e673d045
		u64 result = u64(std::chrono::high_resolution_clock::now().time_since_epoch().count());
		result *= 0xbc2ad017d719504d;
		return uint32_t(result ^ (result >> 32));
#endif
	}

	u64 RandomNumberGenerator64::RandSeed()
	{
#if SUPPORT_RANDOM_DEVICE
		std::random_device randDevice;
		u64 value = randDevice();
		return (value << 32) | randDevice();
#else
		return u64(std::chrono::high_resolution_clock::now().time_since_epoch().count());
#endif
	}
}
