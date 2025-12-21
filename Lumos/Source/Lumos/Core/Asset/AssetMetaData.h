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
        uint64_t ParameterCache = 0;
    };
}
