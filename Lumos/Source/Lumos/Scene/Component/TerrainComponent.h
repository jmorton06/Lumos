#pragma once
#include "Core/DataStructures/TDArray.h"
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <string>

namespace Lumos
{
    class Terrain;
    class TerrainCollisionShape;

    // Authoring data for a terrain entity. Always present on terrain entities
    // created via EntityFactory::AddTerrain*. The mesh + collision shape are
    // rebuilt from this when it becomes dirty (sculpt edits, height load).
    //
    // Storage model: if HasCustomEdits is false the heights can be regenerated
    // from (Seed, TileOriginX/Z, GridW/H) and we skip writing a .lhmap. Once
    // edited the heights diverge from the procedural function, so on save we
    // dump them to a sidecar `.lhmap` and store the relative VFS path here.
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

        // Set by the editor when sculpting modifies heights. Cleared after the
        // mesh + collision rebuild step. Not serialized.
        bool Dirty = false;

        // -----------------------------------------------------------------
        // .lhmap binary format (little-endian, versioned):
        //   u32 magic ('LHMP')   = 0x504D484C
        //   u32 version          = 1
        //   u32 gridW, gridH
        //   f32 scaleXZ, scaleY
        //   i32 tileOriginX, tileOriginZ
        //   f32 heights[gridW * gridH]
        //   u8  splatWeights[gridW * gridH * 4]   // present iff payload byte budget remains
        // -----------------------------------------------------------------
        static bool SaveHeightmap(const std::string& vfsPath, const TerrainComponent& comp);
        static bool LoadHeightmap(const std::string& vfsPath, TerrainComponent& outComp);

        // Allocate / zero splat buffer (4 bytes per vertex).
        void InitSplatWeights();

        // Cereal serialisation. Persists metadata + heightmap path; heights live in the .lhmap
        // (only when HasCustomEdits is set — pure procedural terrains regenerate on load).
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

            // Try to populate Heights from the sidecar — only meaningful when the
            // user actually authored heights. Otherwise leave Heights empty and let
            // the post-load step regenerate procedurally from (Seed/TileOrigin).
            if(HasCustomEdits && !HeightmapPath.empty())
                LoadHeightmap(HeightmapPath, *this);
        }
    };
}
