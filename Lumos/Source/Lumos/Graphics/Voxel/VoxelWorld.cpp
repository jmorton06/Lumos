#include "Precompiled.h"
#include "VoxelWorld.h"
#include "Graphics/Material.h"
#include "Graphics/RHI/Texture.h"
#include "Core/Application.h"
#include "Core/Asset/AssetManager.h"
#include "Core/JobSystem.h"
#include "Core/DataStructures/TDArray.h"

namespace Lumos
{
    namespace Graphics
    {
        VoxelWorld::VoxelWorld()
        {
        }

        VoxelWorld::~VoxelWorld()
        {
            for(auto& [key, chunk] : m_Chunks)
                delete chunk;
            m_Chunks.clear();
        }

        static void AtlasAlbedo(uint8_t tile, int tx, int ty, int TS, int& r, int& g, int& b)
        {
            switch(tile)
            {
            case Tile_GrassTop: r = 86;  g = 150; b = 58;  break;
            case Tile_Dirt:     r = 112; g = 78;  b = 50;  break;
            case Tile_Stone:    r = 124; g = 124; b = 128; break;
            case Tile_GrassSide:
                if(ty < 4)       { r = 86;  g = 150; b = 58; }
                else if(ty == 4) { r = 70;  g = 110; b = 46; }
                else             { r = 112; g = 78;  b = 50; }
                break;
            case Tile_Sand: r = 214; g = 200; b = 140; break;
            case Tile_Snow: r = 236; g = 240; b = 245; break;
            case Tile_WoodSide:
            {
                int s = ((tx * 5) / TS) & 1; // vertical bark streaks
                r = s ? 104 : 88; g = s ? 72 : 60; b = s ? 44 : 36;
                break;
            }
            case Tile_WoodTop:
            {
                float cx = TS * 0.5f - 0.5f, cy = TS * 0.5f - 0.5f;
                float d  = sqrtf((tx - cx) * (tx - cx) + (ty - cy) * (ty - cy));
                int ring = ((int)d) & 1; // concentric rings
                r = ring ? 158 : 132; g = ring ? 116 : 94; b = ring ? 74 : 58;
                break;
            }
            case Tile_Leaves:
            {
                int v = ((tx * 7 + ty * 13) % 5) * 6;
                r = 48 + v; g = 110 + v; b = 40 + v / 2;
                break;
            }
            case Tile_Water: r = 48; g = 98; b = 176; break;
            case Tile_Coal:
            case Tile_Iron:
            {
                // Stone with scattered ore specks.
                r = 124; g = 124; b = 128;
                uint32_t h = (uint32_t)(tx * 49157) ^ (uint32_t)(ty * 98317);
                if((h % 5) == 0)
                {
                    if(tile == Tile_Coal) { r = 32;  g = 32;  b = 34;  } // dark coal
                    else                  { r = 188; g = 150; b = 110; } // tan iron
                }
                break;
            }
            default: r = 124; g = 124; b = 128; break;
            }
        }

        // Per-tile PBR params: roughness, metallic, and normal-map relief strength.
        static void AtlasMaterial(uint8_t tile, float& rough, float& metal, float& relief)
        {
            metal = 0.0f; rough = 0.9f; relief = 0.5f;
            switch(tile)
            {
            case Tile_Water:    rough = 0.06f; relief = 0.0f; break; // wet & glossy, flat
            case Tile_Snow:     rough = 0.55f; relief = 0.2f; break;
            case Tile_Sand:     rough = 1.00f; relief = 0.3f; break;
            case Tile_Stone:    rough = 0.88f; relief = 1.0f; break;
            case Tile_Dirt:     rough = 0.97f; relief = 0.7f; break;
            case Tile_GrassTop: rough = 0.95f; relief = 0.5f; break;
            case Tile_GrassSide:rough = 0.95f; relief = 0.5f; break;
            case Tile_WoodSide: rough = 0.85f; relief = 0.8f; break;
            case Tile_WoodTop:  rough = 0.85f; relief = 0.6f; break;
            case Tile_Leaves:   rough = 0.95f; relief = 0.6f; break;
            case Tile_Coal:     rough = 0.80f; relief = 1.0f; break;
            case Tile_Iron:     rough = 0.65f; relief = 1.0f; metal = 0.3f; break; // faint metallic
            default: break;
            }
        }

        static void BuildBlockTextures(PBRMataterialTextures& out)
        {
            const int TS = ATLAS_TILE_PX;
            const int W  = ATLAS_COLS * TS;
            const int H  = ATLAS_ROWS * TS;

            TDArray<uint8_t> albedo, normal, metalRough, emissive;
            albedo.Resize((uint32_t)(W * H * 4));
            normal.Resize((uint32_t)(W * H * 4));
            metalRough.Resize((uint32_t)(W * H * 4));
            emissive.Resize((uint32_t)(W * H * 4));

            auto clampB = [](float v) -> uint8_t { return (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v)); };
            auto grain  = [](int x, int y) -> int
            { uint32_t h = (uint32_t)(x * 73856093) ^ (uint32_t)(y * 19349663); return (int)(h % 19) - 9; };

            // Luminance of the (clamped) albedo texel — used as the heightfield.
            auto lum = [TS](uint8_t tile, int tx, int ty) -> float
            {
                if(tx < 0) tx = 0; if(tx >= TS) tx = TS - 1;
                if(ty < 0) ty = 0; if(ty >= TS) ty = TS - 1;
                int r, g, b; AtlasAlbedo(tile, tx, ty, TS, r, g, b);
                return (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;
            };

            for(uint8_t tile = 0; tile < (uint8_t)(ATLAS_COLS * ATLAS_ROWS); ++tile)
            {
                const int cx = (tile % ATLAS_COLS) * TS;
                const int cy = (tile / ATLAS_COLS) * TS;
                float rough, metal, relief;
                AtlasMaterial(tile, rough, metal, relief);

                for(int ty = 0; ty < TS; ++ty)
                    for(int tx = 0; tx < TS; ++tx)
                    {
                        const int i = ((cy + ty) * W + (cx + tx)) * 4;

                        // Albedo (+ grain).
                        int r, g, b; AtlasAlbedo(tile, tx, ty, TS, r, g, b);
                        const int n   = grain(cx + tx, cy + ty);
                        albedo[i + 0] = clampB((float)(r + n));
                        albedo[i + 1] = clampB((float)(g + n));
                        albedo[i + 2] = clampB((float)(b + n));
                        albedo[i + 3] = 255;

                        float dx = (lum(tile, tx - 1, ty) - lum(tile, tx + 1, ty)) * relief * 2.0f;
                        float dy = (lum(tile, tx, ty - 1) - lum(tile, tx, ty + 1)) * relief * 2.0f;
                        float nz = 1.0f;
                        float inv = 1.0f / sqrtf(dx * dx + dy * dy + nz * nz);
                        normal[i + 0] = clampB((dx * inv * 0.5f + 0.5f) * 255.0f);
                        normal[i + 1] = clampB((dy * inv * 0.5f + 0.5f) * 255.0f);
                        normal[i + 2] = clampB((nz * inv * 0.5f + 0.5f) * 255.0f);
                        normal[i + 3] = 255;

                        // Metallic-roughness packed: G = roughness, B = metallic.
                        metalRough[i + 0] = 0;
                        metalRough[i + 1] = clampB(rough * 255.0f);
                        metalRough[i + 2] = clampB(metal * 255.0f);
                        metalRough[i + 3] = 255;

                        emissive[i + 0] = 0;
                        emissive[i + 1] = 0;
                        emissive[i + 2] = 0;
                        emissive[i + 3] = 255;
                    }
            }

            auto makeTex = [&](TDArray<uint8_t>& buf, bool srgb) -> SharedPtr<Texture2D>
            {
                auto d                = TextureDesc(TextureFilter::NEAREST, TextureFilter::NEAREST, TextureWrap::CLAMP);
                d.flags               = TextureFlags::Texture_Sampled;
                d.generateMipMaps     = false;
                d.anisotropicFiltering = false; // anisotropy bleeds across tiles at grazing angles
                d.srgb                = srgb;
                return SharedPtr<Texture2D>(Texture2D::CreateFromSource((uint32_t)W, (uint32_t)H, buf.Data(), d));
            };

            out.albedo   = makeTex(albedo, true);       // colour -> sRGB
            out.normal   = makeTex(normal, false);      // data
            out.metallic = makeTex(metalRough, false);  // data (packed MR)
            out.emissive = makeTex(emissive, true);     // colour -> sRGB
        }

        void VoxelWorld::EnsureMaterial()
        {
            if(m_BlockMaterial)
                return;

            auto shader     = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("ForwardPBR")).As<Graphics::Shader>();
            m_BlockMaterial = CreateSharedPtr<Material>(shader);

            PBRMataterialTextures textures;
            BuildBlockTextures(textures);
            m_BlockMaterial->SetTextures(textures);

            MaterialProperties props;
            props.albedoColour       = Vec4(1.0f);
            props.albedoMapFactor    = 1.0f;
            props.normalMapFactor    = 1.0f;
            props.metallicMapFactor  = 1.0f; // drives both metallic & roughness in this workflow
            props.emissiveMapFactor  = 1.0f;
            props.emissiveColour     = Vec4(1.0f); // white tint so the emissive map drives emission (multiplicative)
            props.metallic           = 0.0f;
            props.roughness          = 0.95f;
            props.workflow           = PBR_WORKFLOW_METALLIC_ROUGHNESS;
            m_BlockMaterial->SetMaterialProperites(props);
        }

        void VoxelWorld::Clear()
        {
            for(auto& [key, chunk] : m_Chunks)
                delete chunk;
            m_Chunks.clear();
        }

        VoxelChunk* VoxelWorld::FindChunk(int cx, int cz) const
        {
            auto it = m_Chunks.find(ChunkKey(cx, cz));
            return it == m_Chunks.end() ? nullptr : it->second;
        }

        void VoxelWorld::MarkDirty(int cx, int cz)
        {
            if(VoxelChunk* c = FindChunk(cx, cz))
                c->MarkDirty();
        }

        BlockID VoxelWorld::GetBlock(int gx, int gy, int gz) const
        {
            if(gy < 0 || gy >= CHUNK_H)
                return Block_Air;
            int cx = FloorDiv(gx, CHUNK_W);
            int cz = FloorDiv(gz, CHUNK_D);
            VoxelChunk* c = FindChunk(cx, cz);
            if(!c)
                return Block_Air;
            return c->GetBlock(gx - cx * CHUNK_W, gy, gz - cz * CHUNK_D);
        }

        void VoxelWorld::SetBlock(int gx, int gy, int gz, BlockID id)
        {
            if(gy < 0 || gy >= CHUNK_H)
                return;
            int cx        = FloorDiv(gx, CHUNK_W);
            int cz        = FloorDiv(gz, CHUNK_D);
            VoxelChunk* c = FindChunk(cx, cz);
            if(!c)
                return;
            int lx = gx - cx * CHUNK_W;
            int lz = gz - cz * CHUNK_D;
            c->SetBlock(lx, gy, lz, id);

            // Touching a border block invalidates the neighbour's culling too.
            if(lx == 0)
                MarkDirty(cx - 1, cz);
            if(lx == CHUNK_W - 1)
                MarkDirty(cx + 1, cz);
            if(lz == 0)
                MarkDirty(cx, cz - 1);
            if(lz == CHUNK_D - 1)
                MarkDirty(cx, cz + 1);
        }

        bool VoxelWorld::Raycast(const Vec3& origin, const Vec3& dir, float maxDist, RayHit& out) const
        {
            float len = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
            if(len < 1e-6f)
                return false;
            Vec3 d(dir.x / len, dir.y / len, dir.z / len);

            int x = (int)floorf(origin.x);
            int y = (int)floorf(origin.y);
            int z = (int)floorf(origin.z);

            const float INF = 1e30f;
            int stepX = d.x > 0 ? 1 : (d.x < 0 ? -1 : 0);
            int stepY = d.y > 0 ? 1 : (d.y < 0 ? -1 : 0);
            int stepZ = d.z > 0 ? 1 : (d.z < 0 ? -1 : 0);

            float tMaxX = stepX != 0 ? ((stepX > 0 ? (x + 1 - origin.x) : (origin.x - x)) / fabsf(d.x)) : INF;
            float tMaxY = stepY != 0 ? ((stepY > 0 ? (y + 1 - origin.y) : (origin.y - y)) / fabsf(d.y)) : INF;
            float tMaxZ = stepZ != 0 ? ((stepZ > 0 ? (z + 1 - origin.z) : (origin.z - z)) / fabsf(d.z)) : INF;
            float tDeltaX = stepX != 0 ? fabsf(1.0f / d.x) : INF;
            float tDeltaY = stepY != 0 ? fabsf(1.0f / d.y) : INF;
            float tDeltaZ = stepZ != 0 ? fabsf(1.0f / d.z) : INF;

            int lastAxis = -1;
            float t      = 0.0f;

            while(t <= maxDist)
            {
                // Water is passable for editing — break/place targets solids behind it.
                if(BlockCollides(GetBlock(x, y, z)))
                {
                    out.bx = x; out.by = y; out.bz = z;
                    out.nx = lastAxis == 0 ? -stepX : 0;
                    out.ny = lastAxis == 1 ? -stepY : 0;
                    out.nz = lastAxis == 2 ? -stepZ : 0;
                    out.distance = t;
                    return true;
                }

                if(tMaxX < tMaxY && tMaxX < tMaxZ)
                {
                    x += stepX; t = tMaxX; tMaxX += tDeltaX; lastAxis = 0;
                }
                else if(tMaxY < tMaxZ)
                {
                    y += stepY; t = tMaxY; tMaxY += tDeltaY; lastAxis = 1;
                }
                else
                {
                    z += stepZ; t = tMaxZ; tMaxZ += tDeltaZ; lastAxis = 2;
                }
            }
            return false;
        }

        bool VoxelWorld::OverlapAABB(const Vec3& min, const Vec3& max) const
        {
            int x0 = (int)floorf(min.x), x1 = (int)floorf(max.x);
            int y0 = (int)floorf(min.y), y1 = (int)floorf(max.y);
            int z0 = (int)floorf(min.z), z1 = (int)floorf(max.z);
            for(int y = y0; y <= y1; ++y)
                for(int z = z0; z <= z1; ++z)
                    for(int x = x0; x <= x1; ++x)
                        if(BlockCollides(GetBlock(x, y, z)))
                            return true;
            return false;
        }

        bool VoxelWorld::PlaceBlock(const Vec3& origin, const Vec3& dir, float maxDist, BlockID id)
        {
            RayHit hit;
            if(!Raycast(origin, dir, maxDist, hit))
                return false;
            SetBlock(hit.bx + hit.nx, hit.by + hit.ny, hit.bz + hit.nz, id);
            return true;
        }

        bool VoxelWorld::RemoveBlock(const Vec3& origin, const Vec3& dir, float maxDist)
        {
            RayHit hit;
            if(!Raycast(origin, dir, maxDist, hit))
                return false;
            SetBlock(hit.bx, hit.by, hit.bz, Block_Air);
            return true;
        }

        void VoxelWorld::Update(const Vec3& focusWorldPos)
        {
            LUMOS_PROFILE_FUNCTION();
            EnsureMaterial();

            const int fcx = FloorDiv((int)floorf(focusWorldPos.x), CHUNK_W);
            const int fcz = FloorDiv((int)floorf(focusWorldPos.z), CHUNK_D);
            const int r   = m_ViewRadius;

            // 1. Spawn chunks entering the radius (cheap: just allocate + register).
            TDArray<VoxelChunk*> toGenerate;
            for(int cz = fcz - r; cz <= fcz + r; ++cz)
            {
                for(int cx = fcx - r; cx <= fcx + r; ++cx)
                {
                    if(FindChunk(cx, cz))
                        continue;
                    VoxelChunk* chunk          = new VoxelChunk(cx, cz);
                    m_Chunks[ChunkKey(cx, cz)] = chunk;
                    toGenerate.PushBack(chunk);

                    MarkDirty(cx - 1, cz);
                    MarkDirty(cx + 1, cz);
                    MarkDirty(cx, cz - 1);
                    MarkDirty(cx, cz + 1);
                }
            }

            // 2. Generate new chunks in parallel (each touches only its own blocks).
            if(!toGenerate.Empty())
            {
                System::JobSystem::Context ctx;
                System::JobSystem::Dispatch(ctx, (uint32_t)toGenerate.Size(), 1,
                                            [&toGenerate](JobDispatchArgs args)
                                            { toGenerate[args.jobIndex]->Generate(); });
                System::JobSystem::Wait(ctx);
            }

            // 3. Evict chunks well outside the radius.
            const int evict = r + 2;
            for(auto it = m_Chunks.begin(); it != m_Chunks.end();)
            {
                VoxelChunk* c = it->second;
                if(abs(c->GetChunkX() - fcx) > evict || abs(c->GetChunkZ() - fcz) > evict)
                {
                    delete c;
                    it = m_Chunks.erase(it);
                }
                else
                    ++it;
            }

            TDArray<VoxelChunk*> toMesh;
            for(auto& [key, c] : m_Chunks)
                if(c->IsDirty())
                    toMesh.PushBack(c);

            if(!toMesh.Empty())
            {
                System::JobSystem::Context ctx;
                System::JobSystem::Dispatch(ctx, (uint32_t)toMesh.Size(), 1,
                                            [this, &toMesh](JobDispatchArgs args)
                                            {
                                                VoxelChunk* c = toMesh[args.jobIndex];
                                                c->BuildMeshData(FindChunk(c->GetChunkX() - 1, c->GetChunkZ()),
                                                                 FindChunk(c->GetChunkX() + 1, c->GetChunkZ()),
                                                                 FindChunk(c->GetChunkX(), c->GetChunkZ() - 1),
                                                                 FindChunk(c->GetChunkX(), c->GetChunkZ() + 1));
                                            });
                System::JobSystem::Wait(ctx);

                // 5. Upload to GPU on this (render) thread.
                for(VoxelChunk* c : toMesh)
                {
                    c->UploadMesh();
                    c->SetMaterial(m_BlockMaterial);
                }
            }
        }
    }
}
