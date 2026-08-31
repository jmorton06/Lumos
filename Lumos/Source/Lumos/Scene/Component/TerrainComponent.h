#pragma once
#include "Core/DataStructures/TDArray.h"
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <string>

namespace Lumos
{
    class Terrain;
    class TerrainCollisionShape;

    struct TerrainComponent
    {
        TDArray<float> Heights;       // GridW * GridH floats, row-major: heights[x * GridW + z]
        TDArray<uint8_t> SplatWeights; // GridW * GridH * 4 (RGBA) — stub for future splat phase

        int GridW = 0;
        int GridH = 0;
        float ScaleXZ = 1.0f;
        float ScaleY  = 1.0f;
        int TileOriginX = 0;
        int TileOriginZ = 0;

        bool HasCustomEdits = false; // false → procedurally regenerable; true → load from HeightmapPath
        std::string HeightmapPath;   // VFS path to .lhmap when HasCustomEdits, else empty

        bool Dirty = false;

        static bool SaveHeightmap(const std::string& vfsPath, const TerrainComponent& comp);
        static bool LoadHeightmap(const std::string& vfsPath, TerrainComponent& outComp);

        // Allocate / zero splat buffer (4 bytes per vertex).
        void InitSplatWeights();

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(cereal::make_nvp("GridW", GridW),
                    cereal::make_nvp("GridH", GridH),
                    cereal::make_nvp("ScaleXZ", ScaleXZ),
                    cereal::make_nvp("ScaleY", ScaleY),
                    cereal::make_nvp("TileOriginX", TileOriginX),
                    cereal::make_nvp("TileOriginZ", TileOriginZ),
                    cereal::make_nvp("HasCustomEdits", HasCustomEdits),
                    cereal::make_nvp("HeightmapPath", HeightmapPath));
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            archive(cereal::make_nvp("GridW", GridW),
                    cereal::make_nvp("GridH", GridH),
                    cereal::make_nvp("ScaleXZ", ScaleXZ),
                    cereal::make_nvp("ScaleY", ScaleY),
                    cereal::make_nvp("TileOriginX", TileOriginX),
                    cereal::make_nvp("TileOriginZ", TileOriginZ),
                    cereal::make_nvp("HasCustomEdits", HasCustomEdits),
                    cereal::make_nvp("HeightmapPath", HeightmapPath));

            if(HasCustomEdits && !HeightmapPath.empty())
                LoadHeightmap(HeightmapPath, *this);
        }
    };
}
