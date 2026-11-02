#pragma once
#include "VoxelChunk.h"
#include "Core/Core.h"
#include <unordered_map> // TODO: migrate chunk registry to engine HashMap

namespace Lumos
{
    namespace Graphics
    {
        class Material;

        class LUMOS_EXPORT VoxelWorld
        {
        public:
            VoxelWorld();
            ~VoxelWorld();

            void Update(const Vec3& focusWorldPos);

            BlockID GetBlock(int gx, int gy, int gz) const;
            void SetBlock(int gx, int gy, int gz, BlockID id);

            struct RayHit
            {
                int bx, by, bz;    // hit block (global coords)
                int nx, ny, nz;    // face normal of the entered face
                float distance;
            };

            // Voxel DDA traversal (Amanatides & Woo). origin/dir in world space.
            bool Raycast(const Vec3& origin, const Vec3& dir, float maxDist, RayHit& out) const;

            bool OverlapAABB(const Vec3& min, const Vec3& max) const;

            bool PlaceBlock(const Vec3& origin, const Vec3& dir, float maxDist, BlockID id);
            bool RemoveBlock(const Vec3& origin, const Vec3& dir, float maxDist);

            int GetViewRadius() const { return m_ViewRadius; }
            void SetViewRadius(int r) { m_ViewRadius = r; }
            size_t GetLoadedChunkCount() const { return m_Chunks.size(); }

            // Drop all chunks; they re-stream from the noise function next Update.
            void Clear();

            const std::unordered_map<uint64_t, VoxelChunk*>& GetChunks() const { return m_Chunks; }

            // World position offset of a chunk (its local origin in world space).
            static Vec3 ChunkWorldOrigin(int chunkX, int chunkZ)
            {
                return Vec3((float)(chunkX * CHUNK_W), 0.0f, (float)(chunkZ * CHUNK_D));
            }

            static uint64_t ChunkKey(int cx, int cz)
            {
                return ((uint64_t)(uint32_t)cx << 32) | (uint64_t)(uint32_t)cz;
            }
            static int FloorDiv(int a, int b)
            {
                int q = a / b;
                if((a % b != 0) && ((a < 0) != (b < 0)))
                    --q;
                return q;
            }

        private:
            VoxelChunk* FindChunk(int cx, int cz) const;
            void MarkDirty(int cx, int cz);
            void EnsureMaterial();

            std::unordered_map<uint64_t, VoxelChunk*> m_Chunks;
            SharedPtr<Material> m_BlockMaterial;
            int m_ViewRadius = 4; // chunks (9x9 loaded)
        };
    }
}
