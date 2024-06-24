#pragma once

namespace Lumos
{
    class Asset;
    enum class AssetType : uint16_t;

    struct AssetMetaData
    {
        float timeSinceReload = 0.0f;
        float lastAccessed    = 0.0f;
        SharedPtr<Asset> data = nullptr;
        bool onDisk           = false;
        bool Expire           = true;
        AssetType Type;
        bool IsDataLoaded       = false;
        bool IsMemoryAsset      = false;
        uint64_t ParameterCache = 0;
    };
}