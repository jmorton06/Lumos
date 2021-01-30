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

		uint64_t generationCount;

	public:

		inline RandomNumberGenerator32() : generationCount(0) {}
		inline RandomNumberGenerator32(uint32_t seed) : engine(seed), generationCount(0) {}
		inline RandomNumberGenerator32(uint32_t seed, uint64_t skip) : engine(seed), generationCount(skip)
		{
			engine.discard(skip);
		}

		inline float operator()(float min, float max)
		{
			++generationCount;
			std::uniform_real_distribution<float> realDist(min, max);
			return realDist(engine);
		}

		inline int32_t operator()(int32_t min, int32_t max)
		{
			++generationCount;
			std::uniform_int_distribution<int32_t> intDist(min, max);
			return intDist(engine);
		}

		inline uint32_t operator()(uint32_t min, uint32_t max)
		{
			++generationCount;
			std::uniform_int_distribution<uint32_t> uintDist(min, max);
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
		uint64_t generationCount;

	public:

		RandomNumberGenerator64() : generationCount(0) {}
		RandomNumberGenerator64(uint64_t seed) : engine(seed), generationCount(0) {}
		RandomNumberGenerator64(uint64_t seed, uint64_t skip) : engine(seed), generationCount(skip)
		{
			engine.discard(skip);
		}

		inline double operator()(double min, double max)
		{
			++generationCount;
			std::uniform_real_distribution<double> realDist(min, max);
			return realDist(engine);
		}

		inline int64_t operator()(int64_t min, int64_t max)
		{
			++generationCount;
			std::uniform_int_distribution<int64_t> intDist(min, max);
			return intDist(engine);
		}

		inline uint64_t operator()(uint64_t min, uint64_t max)
		{
			++generationCount;
			std::uniform_int_distribution<uint64_t> uintDist(min, max);
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

		static uint64_t RandSeed();
		static RandomNumberGenerator64 Rand;
	};
}
