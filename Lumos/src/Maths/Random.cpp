#include "Precompiled.h"
#include "Maths/Random.h"

namespace Lumos::Maths
{
    static unsigned randomSeed = 1;

    void SetRandomSeed(unsigned seed)
    {
        randomSeed = seed;
    }

    unsigned GetRandomSeed()
    {
        return randomSeed;
    }

    int Rand()
    {
        randomSeed = randomSeed * 214013 + 2531011;
        return (randomSeed >> 16u) & 32767u;
    }

    float RandStandardNormalized()
    {
        float val = 0.0f;
        for (int i = 0; i < 12; i++)
            val += Rand() / 32768.0f;
        val -= 6.0f;

        // Now val is approximatly standard normal distributed
        return val;
    }
}
