#pragma once

namespace Lumos
{
    class Asset;
    enum class AssetType : uint16_t;

    struct AssetMetaData
    {
        float TimeSinceReload = 0.0f;
        float LastAccessed    = 0.0f;
        SharedPtr<Asset> Data = nullptr;
        bool bEmbeddedAsset   = false;
        bool Expire           = true;
        AssetType Type;
        bool IsDataLoaded       = false;
        bool IsMemoryAsset      = false;
        bool IsEngineAsset      = false;
        uint64_t ParameterCache = 0;

        // Import pipeline fields
        std::string SourcePath;   // Original source file (e.g. .glb/.fbx)
        std::string ImportedPath; // Imported asset path (e.g. .lmesh)
        uint64_t SourceModTime = 0; // Source file modification time
    };
}
