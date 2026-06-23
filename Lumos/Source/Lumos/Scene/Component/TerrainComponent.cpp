#include "Precompiled.h"
#include "TerrainComponent.h"
#include "Core/OS/FileSystem.h"
#include "Core/OS/Memory.h"
#include "Core/Thread.h"
#include "Core/String.h"

namespace Lumos
{
    static constexpr uint32_t LHMAP_MAGIC   = 0x504D484C; // 'LHMP'
    static constexpr uint32_t LHMAP_VERSION = 1;

    void TerrainComponent::InitSplatWeights()
    {
        const uint32_t count = (uint32_t)(GridW * GridH * 4);
        SplatWeights.Resize(count);
        // Default: layer 0 weight = 255 (full), layers 1..3 = 0.
        for(uint32_t i = 0; i < count; i += 4)
        {
            SplatWeights[i + 0] = 255;
            SplatWeights[i + 1] = 0;
            SplatWeights[i + 2] = 0;
            SplatWeights[i + 3] = 0;
        }
    }

    bool TerrainComponent::SaveHeightmap(const std::string& vfsPath, const TerrainComponent& comp)
    {
        if(comp.GridW <= 0 || comp.GridH <= 0 || comp.Heights.Size() != (size_t)(comp.GridW * comp.GridH))
            return false;

        const uint32_t headerBytes = 4 * sizeof(uint32_t) + 2 * sizeof(float) + 2 * sizeof(int32_t);
        const uint32_t heightBytes = (uint32_t)(comp.GridW * comp.GridH * sizeof(float));
        const bool hasSplat = comp.SplatWeights.Size() == (size_t)(comp.GridW * comp.GridH * 4);
        const uint32_t splatBytes  = hasSplat ? (uint32_t)comp.SplatWeights.Size() : 0u;
        const uint32_t totalBytes  = headerBytes + heightBytes + splatBytes;

        ArenaTemp scratch = ScratchBegin(0, 0);
        uint8_t* buffer = (uint8_t*)ArenaPush(scratch.arena, totalBytes);
        uint8_t* p = buffer;

        auto WriteU32 = [&](uint32_t v) { memcpy(p, &v, sizeof(v)); p += sizeof(v); };
        auto WriteI32 = [&](int32_t v)  { memcpy(p, &v, sizeof(v)); p += sizeof(v); };
        auto WriteF32 = [&](float v)    { memcpy(p, &v, sizeof(v)); p += sizeof(v); };

        WriteU32(LHMAP_MAGIC);
        WriteU32(LHMAP_VERSION);
        WriteU32((uint32_t)comp.GridW);
        WriteU32((uint32_t)comp.GridH);
        WriteF32(comp.ScaleXZ);
        WriteF32(comp.ScaleY);
        WriteI32(comp.TileOriginX);
        WriteI32(comp.TileOriginZ);

        memcpy(p, comp.Heights.Data(), heightBytes);
        p += heightBytes;

        if(hasSplat)
        {
            memcpy(p, comp.SplatWeights.Data(), splatBytes);
            p += splatBytes;
        }

        String8 path = Str8StdS(vfsPath);
        bool ok = FileSystem::Get().WriteFileVFS(path, buffer, totalBytes);
        ScratchEnd(scratch);
        return ok;
    }

    bool TerrainComponent::LoadHeightmap(const std::string& vfsPath, TerrainComponent& outComp)
    {
        ArenaTemp scratch = ScratchBegin(0, 0);
        String8 path = Str8StdS(vfsPath);
        if(!FileSystem::Get().FileExistsVFS(path))
        {
            ScratchEnd(scratch);
            return false;
        }
        int64_t size = FileSystem::Get().GetFileSizeVFS(path);
        if(size <= 0)
        {
            ScratchEnd(scratch);
            return false;
        }
        uint8_t* data = FileSystem::Get().ReadFileVFS(scratch.arena, path);
        if(!data)
        {
            ScratchEnd(scratch);
            return false;
        }

        uint8_t* p = data;
        auto ReadU32 = [&](uint32_t& v) { memcpy(&v, p, sizeof(v)); p += sizeof(v); };
        auto ReadI32 = [&](int32_t& v)  { memcpy(&v, p, sizeof(v)); p += sizeof(v); };
        auto ReadF32 = [&](float& v)    { memcpy(&v, p, sizeof(v)); p += sizeof(v); };

        uint32_t magic = 0, version = 0;
        ReadU32(magic);
        ReadU32(version);
        if(magic != LHMAP_MAGIC || version != LHMAP_VERSION)
        {
            ScratchEnd(scratch);
            return false;
        }

        uint32_t w = 0, h = 0;
        ReadU32(w);
        ReadU32(h);
        float sxz = 1.0f, sy = 1.0f;
        ReadF32(sxz);
        ReadF32(sy);
        int32_t tox = 0, toz = 0;
        ReadI32(tox);
        ReadI32(toz);

        outComp.GridW       = (int)w;
        outComp.GridH       = (int)h;
        outComp.ScaleXZ     = sxz;
        outComp.ScaleY      = sy;
        outComp.TileOriginX = tox;
        outComp.TileOriginZ = toz;

        const uint32_t numVerts   = w * h;
        const uint32_t heightBytes = numVerts * (uint32_t)sizeof(float);
        outComp.Heights.Resize(numVerts);
        memcpy(outComp.Heights.Data(), p, heightBytes);
        p += heightBytes;

        // Splat is optional — older files might not have it.
        const uint32_t consumed   = (uint32_t)(p - data);
        const uint32_t remaining  = (uint32_t)size - consumed;
        const uint32_t splatBytes = numVerts * 4u;
        if(remaining >= splatBytes)
        {
            outComp.SplatWeights.Resize(splatBytes);
            memcpy(outComp.SplatWeights.Data(), p, splatBytes);
        }
        else
        {
            outComp.InitSplatWeights();
        }

        outComp.HasCustomEdits = true;
        outComp.HeightmapPath  = vfsPath;
        ScratchEnd(scratch);
        return true;
    }
}
