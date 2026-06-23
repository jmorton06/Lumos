#pragma once
#include <cstdint>

namespace Lumos
{
    namespace Graphics
    {
        enum BiomeID : uint8_t
        {
            Biome_Ocean    = 0,
            Biome_Beach    = 1,
            Biome_Plains   = 2,
            Biome_Forest   = 3,
            Biome_Desert   = 4,
            Biome_Mountain = 5,
            Biome_Snow     = 6,
        };

        struct VoxelColumn
        {
            int height;     // surface height in blocks
            BiomeID biome;
        };

        // Sea level (block y). Columns below it flood with water down to the floor.
        int VoxelSeaLevel();

        // Surface height + biome for a global block column. Deterministic from world
        // coords so neighbouring chunks share seam values.
        VoxelColumn VoxelSampleColumn(int globalX, int globalZ);

        // Back-compat: just the height.
        int VoxelTerrainHeight(int globalX, int globalZ);

        // Stable 0..1 hash of a column, for scatter decisions (trees, etc).
        float VoxelColumnHash(int globalX, int globalZ);

        // True where an underground cave should be carved (3D worm noise).
        bool VoxelIsCave(int gx, int gy, int gz);

        // Ore at a voxel: 0 = none, 1 = coal, 2 = iron. Only meaningful in stone.
        int VoxelOreAt(int gx, int gy, int gz);
    }
}
