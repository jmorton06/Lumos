#pragma once

#include "Core/Core.h"
#include <random>
#include <cstdint>

namespace Lumos
{

	class LUMOS_EXPORT RandomNumberGenerator32
	{
	private:

		std::mt19937 engine;

		u64 generationCount;

	public:

		_FORCE_INLINE_ RandomNumberGenerator32() : generationCount(0) {}
		_FORCE_INLINE_ RandomNumberGenerator32(u32 seed) : engine(seed), generationCount(0) {}
		_FORCE_INLINE_ RandomNumberGenerator32(u32 seed, u64 skip) : engine(seed), generationCount(skip)
		{
			engine.discard(skip);
		}

		_FORCE_INLINE_ float operator()(float min, float max)
		{
			++generationCount;
			std::uniform_real_distribution<float> realDist(min, max);
			return realDist(engine);
		}

		_FORCE_INLINE_ i32 operator()(i32 min, i32 max)
		{
			++generationCount;
			std::uniform_int_distribution<i32> intDist(min, max);
			return intDist(engine);
		}

		_FORCE_INLINE_ u32 operator()(u32 min, u32 max)
		{
			++generationCount;
			std::uniform_int_distribution<u32> uintDist(min, max);
			return uintDist(engine);
		}

		_FORCE_INLINE_ uint64_t GetNumbersGenerated() const
		{
			return generationCount;
		}

		_FORCE_INLINE_ void Discard(uint64_t steps)
		{
			engine.discard(steps);
			generationCount += steps;
		}

		static uint32_t RandSeed();
		static RandomNumberGenerator32 Rand;
	};

	class LUMOS_EXPORT RandomNumberGenerator64
	{
	private:
		std::mt19937_64 engine;
		u64 generationCount;

	public:

		RandomNumberGenerator64() : generationCount(0) {}
		RandomNumberGenerator64(u64 seed) : engine(seed), generationCount(0) {}
		RandomNumberGenerator64(u64 seed, u64 skip) : engine(seed), generationCount(skip)
		{
			engine.discard(skip);
		}

		_FORCE_INLINE_ double operator()(double min, double max)
		{
			++generationCount;
			std::uniform_real_distribution<double> realDist(min, max);
			return realDist(engine);
		}

		_FORCE_INLINE_ int64_t operator()(i64 min, i64 max)
		{
			++generationCount;
			std::uniform_int_distribution<i64> intDist(min, max);
			return intDist(engine);
		}

		_FORCE_INLINE_ u64 operator()(u64 min, u64 max)
		{
			++generationCount;
			std::uniform_int_distribution<u64> uintDist(min, max);
			return uintDist(engine);
		}

		_FORCE_INLINE_ uint64_t GetNumbersGenerated() const
		{
			return generationCount;
		}

		_FORCE_INLINE_ void Discard(u64 steps)
		{
			engine.discard(steps);
			generationCount += steps;
		}

		static u64 RandSeed();
		static RandomNumberGenerator64 Rand;
	};
}
