#pragma once

#include "LM.h"
#include <random>
#include <cstdint>

namespace lumos
{

	class LUMOS_EXPORT RandomNumberGenerator32
	{
	private:

		std::mt19937 engine;

		uint64 generationCount;

	public:

		inline RandomNumberGenerator32() : generationCount(0) {}
		inline RandomNumberGenerator32(uint32 seed) : engine(seed), generationCount(0) {}
		inline RandomNumberGenerator32(uint32 seed, uint64 skip) : engine(seed), generationCount(skip)
		{
			engine.discard(skip);
		}

		inline float operator()(float min, float max)
		{
			++generationCount;
			std::uniform_real_distribution<float> realDist(min, max);
			return realDist(engine);
		}

		inline int32 operator()(int32 min, int32 max)
		{
			++generationCount;
			std::uniform_int_distribution<int32> intDist(min, max);
			return intDist(engine);
		}

		inline uint32 operator()(uint32 min, uint32 max)
		{
			++generationCount;
			std::uniform_int_distribution<uint32> uintDist(min, max);
			return uintDist(engine);
		}

		inline uint64_t GetNumbersGenerated() const
		{
			return generationCount;
		}

		inline void Discard(uint64_t steps)
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
		uint64 generationCount;

	public:

		RandomNumberGenerator64() : generationCount(0) {}
		RandomNumberGenerator64(uint64 seed) : engine(seed), generationCount(0) {}
		RandomNumberGenerator64(uint64 seed, uint64 skip) : engine(seed), generationCount(skip)
		{
			engine.discard(skip);
		}

		inline double operator()(double min, double max)
		{
			++generationCount;
			std::uniform_real_distribution<double> realDist(min, max);
			return realDist(engine);
		}

		inline int64_t operator()(int64 min, int64 max)
		{
			++generationCount;
			std::uniform_int_distribution<int64> intDist(min, max);
			return intDist(engine);
		}

		inline uint64 operator()(uint64 min, uint64 max)
		{
			++generationCount;
			std::uniform_int_distribution<uint64> uintDist(min, max);
			return uintDist(engine);
		}

		inline uint64_t GetNumbersGenerated() const
		{
			return generationCount;
		}

		inline void Discard(uint64 steps)
		{
			engine.discard(steps);
			generationCount += steps;
		}

		static uint64 RandSeed();
		static RandomNumberGenerator64 Rand;
	};
}