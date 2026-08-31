#pragma once
#include "Graphics/Mesh.h"
#include "Core/Core.h"

namespace Lumos
{
    namespace Graphics
    {
        // Block ids. 0 = air (empty). Each solid id picks atlas tiles per face.
        using BlockID = uint8_t;
        enum : BlockID
        {
            Block_Air    = 0,
            Block_Grass  = 1,
            Block_Dirt   = 2,
            Block_Stone  = 3,
            Block_Sand   = 4,
            Block_Snow   = 5,
            Block_Wood   = 6,
            Block_Leaves = 7,
            Block_Water  = 8,
            Block_Coal   = 9,
            Block_Iron   = 10,
            Block_Count,
            Block_Solid = Block_Grass, // back-compat alias (Lua PLACE_ID = 1)
        };

        // Water is rendered but passable; everything else solid blocks the player.
        inline bool BlockCollides(BlockID id) { return id != Block_Air && id != Block_Water; }

        static constexpr int ATLAS_TILE_PX = 16;
        static constexpr int ATLAS_COLS    = 4;
        static constexpr int ATLAS_ROWS    = 4;
        enum : uint8_t
        {
            Tile_GrassTop  = 0,
            Tile_Dirt      = 1,
            Tile_Stone     = 2,
            Tile_GrassSide = 3,
            Tile_Sand      = 4,
            Tile_Snow      = 5,
            Tile_WoodSide  = 6,
            Tile_WoodTop   = 7,
            Tile_Leaves    = 8,
            Tile_Water     = 9,
            Tile_Coal      = 10,
            Tile_Iron      = 11,
        };

        // Atlas tile for a block id on a given face (kFaces order: +X,-X,+Y,-Y,+Z,-Z).
        inline uint8_t BlockFaceTile(BlockID id, int faceIndex)
        {
            const bool top    = (faceIndex == 2);
            const bool bottom = (faceIndex == 3);
            switch(id)
            {
            case Block_Grass:
                if(top)    return Tile_GrassTop;
                if(bottom) return Tile_Dirt;
                return Tile_GrassSide;
            case Block_Dirt:   return Tile_Dirt;
            case Block_Stone:  return Tile_Stone;
            case Block_Sand:   return Tile_Sand;
            case Block_Snow:   return Tile_Snow;
            case Block_Wood:   return (top || bottom) ? Tile_WoodTop : Tile_WoodSide;
            case Block_Leaves: return Tile_Leaves;
            case Block_Water:  return Tile_Water;
            case Block_Coal:   return Tile_Coal;
            case Block_Iron:   return Tile_Iron;
            default:           return Tile_Stone;
            }
        }

        static constexpr int CHUNK_W = 16;
        static constexpr int CHUNK_H = 256;
        static constexpr int CHUNK_D = 16;
        static constexpr int CHUNK_BLOCK_COUNT = CHUNK_W * CHUNK_H * CHUNK_D;

        class LUMOS_EXPORT VoxelChunk : public Mesh
        {
        public:
            VoxelChunk(int chunkX, int chunkZ);

            BlockID GetBlock(int x, int y, int z) const
            {
                if(!InBounds(x, y, z))
                    return Block_Air;
                return m_Blocks[Index(x, y, z)];
            }

            void SetBlock(int x, int y, int z, BlockID id)
            {
                if(!InBounds(x, y, z))
                    return;
                m_Blocks[Index(x, y, z)] = id;
                m_Dirty                  = true;
            }

            void Generate();

            void BuildMeshData(const VoxelChunk* negX, const VoxelChunk* posX,
                               const VoxelChunk* negZ, const VoxelChunk* posZ);

            bool UploadMesh();

            bool HasPendingMesh() const { return m_HasPendingMesh; }

            // Convenience single-threaded path: build + upload in one call.
            void Rebuild(const VoxelChunk* negX, const VoxelChunk* posX,
                         const VoxelChunk* negZ, const VoxelChunk* posZ)
            {
                BuildMeshData(negX, posX, negZ, posZ);
                UploadMesh();
            }

            int GetChunkX() const { return m_ChunkX; }
            int GetChunkZ() const { return m_ChunkZ; }
            bool IsDirty() const { return m_Dirty; }
            void MarkDirty() { m_Dirty = true; }
            void ClearDirty() { m_Dirty = false; }

            static int Index(int x, int y, int z) { return (y * CHUNK_D + z) * CHUNK_W + x; }
            static bool InBounds(int x, int y, int z)
            {
                return x >= 0 && x < CHUNK_W && y >= 0 && y < CHUNK_H && z >= 0 && z < CHUNK_D;
            }

        private:
            BlockID m_Blocks[CHUNK_BLOCK_COUNT] = { Block_Air };
            int m_ChunkX = 0;
            int m_ChunkZ = 0;
            bool m_Dirty = true;

            TDArray<Vertex> m_PendingVertices;
            TDArray<uint32_t> m_PendingIndices;
            Maths::BoundingBox m_PendingBox;
            bool m_HasPendingMesh = false;
        };
    }
}
