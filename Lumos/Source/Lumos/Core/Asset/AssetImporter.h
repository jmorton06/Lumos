#pragma once
#include "Core/String.h"
#include "Core/Types.h"

namespace Lumos
{
    struct ImportSettings
    {
        bool ImportMaterials         = true;
        bool ImportAnimations        = true;
        bool GenerateCollision       = false;
        float CollisionSimplifyRatio = 0.1f;
        bool OptVertexCache          = true;
        bool OptOverdraw             = true;
        bool OptVertexFetch          = true;
        u32 MaxTextureWidth          = 0; // 0 = no resize
        u32 MaxTextureHeight         = 0;
        bool CopySourceToProject     = true;
        bool SplitMeshes             = false;
    };

    struct ImportMeta
    {
        ImportSettings Settings;
        u64 OutputUUID = 0;
    };

    class AssetImporter
    {
    public:
        static std::string Import(const std::string& sourcePath, const ImportSettings& settings);

        // Check if imported .lmesh is missing or has outdated format version
        static bool NeedsImport(const std::string& sourcePath);

        // Load .meta sidecar file
        static bool LoadMeta(const std::string& sourcePath, ImportMeta& outMeta);

        // Save .meta sidecar file
        static bool SaveMeta(const std::string& sourcePath, const ImportMeta& meta);

        // Get the imported .lmesh path for a source file (may not exist)
        static std::string GetImportedPath(const std::string& sourcePath);

        static std::string NormalizeAssetPath(const std::string& path);

        // Scan Imported/ directory and delete orphaned files that don't match any known source
        static void CleanOrphanedImports();
    };
}
