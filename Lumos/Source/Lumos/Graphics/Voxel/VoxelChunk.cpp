#include "Precompiled.h"
#include "VoxelChunk.h"
#include "VoxelNoise.h"
#include "Graphics/RHI/IndexBuffer.h"
#include "Graphics/RHI/VertexBuffer.h"
#include "Maths/BoundingBox.h"

namespace Lumos
{
    namespace Graphics
    {
        namespace
        {
            // Per-face data, CCW winding viewed from outside (cross(U,V) == normal,
            // VK default front face). corners walk origin, +U, +U+V, +V.
            struct FaceDef
            {
                int nx, ny, nz;       // neighbour offset to test for culling
                Vec3 normal;
                Vec3 origin;          // face corner in unit-cube space
                Vec3 u, v;            // face axes
            };

            const FaceDef kFaces[6] = {
                // +X
                { 1, 0, 0, Vec3(1, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0), Vec3(0, 0, 1) },
                // -X
                { -1, 0, 0, Vec3(-1, 0, 0), Vec3(0, 0, 0), Vec3(0, 0, 1), Vec3(0, 1, 0) },
                // +Y
                { 0, 1, 0, Vec3(0, 1, 0), Vec3(0, 1, 0), Vec3(0, 0, 1), Vec3(1, 0, 0) },
                // -Y
                { 0, -1, 0, Vec3(0, -1, 0), Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 0, 1) },
                // +Z
                { 0, 0, 1, Vec3(0, 0, 1), Vec3(0, 0, 1), Vec3(1, 0, 0), Vec3(0, 1, 0) },
                // -Z
                { 0, 0, -1, Vec3(0, 0, -1), Vec3(0, 0, 0), Vec3(0, 1, 0), Vec3(1, 0, 0) },
            };
        }

        VoxelChunk::VoxelChunk(int chunkX, int chunkZ)
            : m_ChunkX(chunkX)
            , m_ChunkZ(chunkZ)
        {
        }

        void VoxelChunk::Generate()
        {
            const int baseX    = m_ChunkX * CHUNK_W;
            const int baseZ    = m_ChunkZ * CHUNK_D;
            const int seaLevel = VoxelSeaLevel();

            auto set = [&](int x, int y, int z, BlockID id)
            {
                if(InBounds(x, y, z))
                    m_Blocks[Index(x, y, z)] = id;
            };

            // Pass 1: terrain columns + water fill.
            for(int x = 0; x < CHUNK_W; ++x)
            {
                for(int z = 0; z < CHUNK_D; ++z)
                {
                    VoxelColumn col = VoxelSampleColumn(baseX + x, baseZ + z);
                    int height      = col.height;
                    if(height < 1)        height = 1;
                    if(height > CHUNK_H)  height = CHUNK_H;

                    // Surface + subsurface block depend on biome.
                    BlockID surface = Block_Grass, subsurface = Block_Dirt;
                    switch(col.biome)
                    {
                    case Biome_Ocean:
                    case Biome_Beach:    surface = Block_Sand;  subsurface = Block_Sand;  break;
                    case Biome_Desert:   surface = Block_Sand;  subsurface = Block_Sand;  break;
                    case Biome_Snow:     surface = Block_Snow;  subsurface = Block_Dirt;  break;
                    case Biome_Mountain: surface = Block_Stone; subsurface = Block_Stone; break;
                    default:             surface = Block_Grass; subsurface = Block_Dirt;  break;
                    }
                    // Snow cap on tall mountains.
                    if(col.biome == Biome_Mountain && height > seaLevel + 60)
                        surface = Block_Snow;

                    const int gx = baseX + x, gz = baseZ + z;

                    for(int y = 0; y < height; ++y)
                    {
                        BlockID id;
                        if(y == height - 1)      id = surface;
                        else if(y >= height - 4) id = subsurface;
                        else                     id = Block_Stone;

                        // Embed ores in stone.
                        if(id == Block_Stone)
                        {
                            int ore = VoxelOreAt(gx, y, gz);
                            if(ore == 1)      id = Block_Coal;
                            else if(ore == 2) id = Block_Iron;
                        }
                        m_Blocks[Index(x, y, z)] = id;
                    }

                    // Carve caves out of the solid column (keep the top crust + a
                    // bedrock floor so the world isn't holed through).
                    for(int y = 1; y < height - 1; ++y)
                        if(VoxelIsCave(gx, y, gz))
                            m_Blocks[Index(x, y, z)] = Block_Air;

                    // Flood air up to sea level with water.
                    for(int y = height; y <= seaLevel && y < CHUNK_H; ++y)
                        if(m_Blocks[Index(x, y, z)] == Block_Air)
                            m_Blocks[Index(x, y, z)] = Block_Water;
                }
            }

            // Pass 2: scatter trees on grassy land above water. Kept clear of the
            // chunk border so the canopy never needs to write into a neighbour.
            for(int x = 2; x < CHUNK_W - 2; ++x)
            {
                for(int z = 2; z < CHUNK_D - 2; ++z)
                {
                    VoxelColumn col = VoxelSampleColumn(baseX + x, baseZ + z);
                    if(col.biome != Biome_Plains && col.biome != Biome_Forest)
                        continue;
                    if(col.height <= seaLevel)
                        continue;

                    float density = (col.biome == Biome_Forest) ? 0.06f : 0.012f;
                    if(VoxelColumnHash(baseX + x, baseZ + z) >= density)
                        continue;

                    int groundY = col.height - 1; // surface block
                    if(m_Blocks[Index(x, groundY, z)] != Block_Grass)
                        continue;

                    // Trunk height varies a little per tree.
                    int trunkH = 4 + (int)(VoxelColumnHash(baseX - x, baseZ - z) * 3.0f);
                    int topY   = groundY + trunkH;
                    if(topY + 2 >= CHUNK_H)
                        continue;

                    for(int y = groundY + 1; y <= topY; ++y)
                        set(x, y, z, Block_Wood);

                    // Canopy: two wide layers then a small cap.
                    for(int dy = -1; dy <= 0; ++dy)
                        for(int dx = -2; dx <= 2; ++dx)
                            for(int dz = -2; dz <= 2; ++dz)
                            {
                                if(abs(dx) == 2 && abs(dz) == 2)
                                    continue; // round the corners
                                int lx = x + dx, ly = topY + dy, lz = z + dz;
                                if(GetBlock(lx, ly, lz) == Block_Air)
                                    set(lx, ly, lz, Block_Leaves);
                            }
                    for(int dx = -1; dx <= 1; ++dx)
                        for(int dz = -1; dz <= 1; ++dz)
                        {
                            if(abs(dx) == 1 && abs(dz) == 1)
                                continue;
                            if(GetBlock(x + dx, topY + 1, z + dz) == Block_Air)
                                set(x + dx, topY + 1, z + dz, Block_Leaves);
                        }
                }
            }

            m_Dirty = true;
        }

        void VoxelChunk::BuildMeshData(const VoxelChunk* negX, const VoxelChunk* posX,
                                       const VoxelChunk* negZ, const VoxelChunk* posZ)
        {
            LUMOS_PROFILE_FUNCTION();

            // Sample a block, crossing into a neighbour chunk when the coord leaves
            // local bounds. Out-of-world (above/below, or null neighbour) = air.
            auto solidAt = [&](int x, int y, int z) -> bool
            {
                if(y < 0 || y >= CHUNK_H)
                    return false;
                if(x < 0)
                    return negX && negX->GetBlock(CHUNK_W - 1, y, z) != Block_Air;
                if(x >= CHUNK_W)
                    return posX && posX->GetBlock(0, y, z) != Block_Air;
                if(z < 0)
                    return negZ && negZ->GetBlock(x, y, CHUNK_D - 1) != Block_Air;
                if(z >= CHUNK_D)
                    return posZ && posZ->GetBlock(x, y, 0) != Block_Air;
                return m_Blocks[Index(x, y, z)] != Block_Air;
            };

            m_PendingVertices.Clear();
            m_PendingIndices.Clear();
            m_PendingBox = {};
            TDArray<Vertex>& vertices  = m_PendingVertices;
            TDArray<uint32_t>& indices = m_PendingIndices;

            for(int y = 0; y < CHUNK_H; ++y)
            {
                for(int z = 0; z < CHUNK_D; ++z)
                {
                    for(int x = 0; x < CHUNK_W; ++x)
                    {
                        const BlockID block = m_Blocks[Index(x, y, z)];
                        if(block == Block_Air)
                            continue;

                        Vec3 cell((float)x, (float)y, (float)z);

                        for(int fi = 0; fi < 6; ++fi)
                        {
                            const FaceDef& f = kFaces[fi];
                            if(solidAt(x + f.nx, y + f.ny, z + f.nz))
                                continue; // neighbour solid → face hidden

                            uint32_t base = (uint32_t)vertices.Size();
                            Vec3 corners[4] = {
                                cell + f.origin,
                                cell + f.origin + f.u,
                                cell + f.origin + f.u + f.v,
                                cell + f.origin + f.v,
                            };

                            // Atlas tile rect, inset half a texel so NEAREST never
                            // grabs a neighbouring tile.
                            const uint8_t tile = BlockFaceTile(block, fi);
                            const float tw = 1.0f / ATLAS_COLS, th = 1.0f / ATLAS_ROWS;
                            const float u0 = (tile % ATLAS_COLS) * tw;
                            const float v0 = (tile / ATLAS_COLS) * th;
                            const float ex = 0.5f / (ATLAS_COLS * ATLAS_TILE_PX);
                            const float ey = 0.5f / (ATLAS_ROWS * ATLAS_TILE_PX);

                            // The grass-side tile has its grassy lip in the atlas top
                            // rows, so atlas-V must follow world height (top -> V0).
                            // Derive UVs from each corner's cell-local position rather
                            // than the face u/v axes, whose up-axis isn't consistent.
                            const bool sideFace = (f.normal.y == 0.0f);
                            for(int i = 0; i < 4; ++i)
                            {
                                Vec3 local = corners[i] - cell; // components are 0 or 1
                                float fu, fv;
                                if(sideFace)
                                {
                                    fv = 1.0f - local.y;                              // top of block -> green lip
                                    fu = (f.normal.x != 0.0f) ? local.z : local.x;    // horizontal axis
                                }
                                else
                                {
                                    fu = local.x; // top/bottom tiles are uniform, orientation is moot
                                    fv = local.z;
                                }

                                Vertex vert;
                                vert.Position  = corners[i];
                                vert.Normal    = f.normal;
                                vert.Tangent   = f.u;
                                vert.Bitangent = Maths::Vector3::Cross(f.normal, f.u); // TBN needs all three
                                vert.TexCoords = Vec2(u0 + ex + fu * (tw - 2.0f * ex),
                                                      v0 + ey + fv * (th - 2.0f * ey));
                                vert.Colours   = Vec4(1.0f);
                                vertices.PushBack(vert);
                            }

                            indices.PushBack(base + 0);
                            indices.PushBack(base + 1);
                            indices.PushBack(base + 2);
                            indices.PushBack(base + 0);
                            indices.PushBack(base + 2);
                            indices.PushBack(base + 3);
                        }
                    }
                }
            }

            for(const Vertex& v : vertices)
                m_PendingBox.Merge(v.Position);

            m_HasPendingMesh = true;
        }

        bool VoxelChunk::UploadMesh()
        {
            if(!m_HasPendingMesh)
                return false;

            m_BoundingBox = m_PendingBox;
            m_CPUPositions.Clear();
            m_CPUIndices.Clear();

            if(m_PendingVertices.Empty())
            {
                // Fully empty chunk — drop GPU buffers so the renderer skips it.
                m_VertexBuffer.reset();
                m_IndexBuffer.reset();
            }
            else
            {
                m_CPUPositions.Reserve((uint32_t)m_PendingVertices.Size());
                for(const Vertex& v : m_PendingVertices)
                    m_CPUPositions.PushBack(v.Position);
                for(uint32_t i : m_PendingIndices)
                    m_CPUIndices.PushBack(i);

                m_VertexBuffer = SharedPtr<VertexBuffer>(VertexBuffer::Create((uint32_t)(sizeof(Vertex) * m_PendingVertices.Size()), m_PendingVertices.Data(), BufferUsage::STATIC));
                m_IndexBuffer  = SharedPtr<IndexBuffer>(IndexBuffer::Create(m_PendingIndices.Data(), (uint32_t)m_PendingIndices.Size()));
            }

            // Free staging memory.
            m_PendingVertices.Clear();
            m_PendingIndices.Clear();
            m_HasPendingMesh = false;
            m_Dirty          = false;
            return true;
        }
    }
}
