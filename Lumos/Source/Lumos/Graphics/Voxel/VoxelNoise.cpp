#include "Precompiled.h"
#include "VoxelNoise.h"
#include <stb/stb_perlin.h>

namespace Lumos
{
    namespace Graphics
    {
        static constexpr int SEA_LEVEL = 60;

        int VoxelSeaLevel() { return SEA_LEVEL; }

        static float Perlin01(float x, float z, float freq)
        {
            return stb_perlin_noise3(x * freq, z * freq, 0.0f, 0, 0, 0) * 0.5f + 0.5f;
        }

        float VoxelColumnHash(int gx, int gz)
        {
            uint32_t h = (uint32_t)(gx * 374761393) + (uint32_t)(gz * 668265263);
            h          = (h ^ (h >> 13)) * 1274126177u;
            h          = h ^ (h >> 16);
            return (h & 0xffffff) / (float)0xffffff;
        }

        VoxelColumn VoxelSampleColumn(int gx, int gz)
        {
            const float x = (float)gx;
            const float z = (float)gz;

            // Broad continents decide land vs sea; fbm adds hills; ridged term lifts
            // mountains. All sampled in global coords so chunk seams line up.
            float continent = Perlin01(x, z, 1.0f / 640.0f);          // 0..1 slow
            float hills     = Perlin01(x, z, 1.0f / 96.0f) * 0.6f
                            + Perlin01(x, z, 1.0f / 32.0f) * 0.3f
                            + Perlin01(x, z, 1.0f / 12.0f) * 0.1f;     // ~0..1

            // Climate fields, independent low-frequency noise.
            float temperature = Perlin01(x + 1000.0f, z - 1000.0f, 1.0f / 512.0f);
            float mountainous = Perlin01(x - 5000.0f, z + 5000.0f, 1.0f / 384.0f);

            // Base elevation: continents pull up/down around sea level.
            float elevation = SEA_LEVEL - 18.0f + continent * 40.0f; // ~42..82
            elevation += (hills - 0.5f) * 14.0f;

            // Mountains: ridged noise, strongest in high-mountain regions on land.
            if(mountainous > 0.55f && elevation > SEA_LEVEL)
            {
                float ridge = 1.0f - fabsf(stb_perlin_noise3(x / 160.0f, z / 160.0f, 0, 0, 0, 0));
                float m     = (mountainous - 0.55f) / 0.45f; // 0..1
                elevation += ridge * ridge * m * 55.0f;
            }

            int height = (int)elevation;
            if(height < 1) height = 1;
            if(height > 250) height = 250;

            // Biome from elevation + climate.
            BiomeID biome;
            if(height < SEA_LEVEL - 1)
                biome = Biome_Ocean;
            else if(height <= SEA_LEVEL + 1)
                biome = Biome_Beach;
            else if(height > SEA_LEVEL + 42)
                biome = (temperature < 0.45f) ? Biome_Snow : Biome_Mountain;
            else if(temperature > 0.68f)
                biome = Biome_Desert;
            else if(temperature < 0.30f)
                biome = Biome_Snow;
            else
                biome = (Perlin01(x, z, 1.0f / 220.0f) > 0.52f) ? Biome_Forest : Biome_Plains;

            return { height, biome };
        }

        int VoxelTerrainHeight(int gx, int gz)
        {
            return VoxelSampleColumn(gx, gz).height;
        }

        bool VoxelIsCave(int gx, int gy, int gz)
        {
            // Two 3D noise fields; a cave runs where BOTH are near zero, which
            // traces thin worm-like tubes. Y is squashed so tunnels lie flatter.
            const float CF = 28.0f;
            float a = stb_perlin_noise3(gx / CF, gy / (CF * 0.55f), gz / CF, 0, 0, 0);
            float b = stb_perlin_noise3((gx + 311) / CF, (gy - 197) / (CF * 0.55f), (gz + 53) / CF, 0, 0, 0);
            return (a * a + b * b) < 0.022f;
        }

        static float Hash3(int x, int y, int z)
        {
            uint32_t h = (uint32_t)(x * 374761393) + (uint32_t)(y * 668265263) + (uint32_t)(z * 2246822519u);
            h          = (h ^ (h >> 13)) * 1274126177u;
            h          = h ^ (h >> 16);
            return (h & 0xffffff) / (float)0xffffff;
        }

        int VoxelOreAt(int gx, int gy, int gz)
        {
            // Iron: rarer, deep. Coal: commoner, shallower. Iron wins where both hit.
            if(gy >= 3 && gy <= 48 && Hash3(gx, gy, gz) < 0.010f)
                return 2; // iron
            if(gy >= 5 && gy <= 110 && Hash3(gx + 9173, gy, gz - 4211) < 0.016f)
                return 1; // coal
            return 0;
        }
    }
}
