#pragma once
#include "Core/Core.h"
#include "UUID.h"

#define SET_ASSET_TYPE(type)                        \
    static AssetType GetStaticType()                \
    {                                               \
        return type;                                \
    }                                               \
    virtual AssetType GetAssetType() const override \
    {                                               \
        return GetStaticType();                     \
    }

namespace Lumos
{
    enum class AssetFlag : uint16_t
    {
        None     = 0,
        Missing  = BIT(0),
        Invalid  = BIT(1),
        Loaded   = BIT(2),
        UnLoaded = BIT(3)
    };

    enum class AssetType : uint16_t
    {
        Unkown              = 0,
        Texture             = 1,
        Mesh                = 2,
        Scene               = 3,
        Audio               = 4,
        Font                = 5,
        Shader              = 6,
        Material            = 7,
        PhysicsMaterial     = 8,
        Model               = 9,
        Skeleton            = 10,
        Animation           = 11,
        AnimationController = 12,
        Prefab              = 13,
        Script              = 14
    };

    inline const char* AssetTypeToString(AssetType assetType)
    {
        switch(assetType)
        {
        case AssetType::Unkown:
            return "Unkown";
        case AssetType::Scene:
            return "Scene";
        case AssetType::Prefab:
            return "Prefab";
        case AssetType::Mesh:
            return "Mesh";
        case AssetType::Model:
            return "Model";
        case AssetType::Material:
            return "Material";
        case AssetType::Texture:
            return "Texture";
        case AssetType::Audio:
            return "Audio";
        case AssetType::Font:
            return "Font";
        case AssetType::Script:
            return "Script";
        case AssetType::Skeleton:
            return "Skeleton";
        case AssetType::Animation:
            return "Animation";
        case AssetType::Shader:
            return "Shader";
        }

        LUMOS_ASSERT(false, "Unknown Asset Type");
        return "None";
    }

    class LUMOS_EXPORT Asset
    {
    public:
        uint16_t Flags = (uint16_t)AssetFlag::None;

        virtual ~Asset() { }

        static AssetType GetStaticType() { return AssetType::Unkown; }
        virtual AssetType GetAssetType() const { return AssetType::Unkown; }

        bool IsValid() const { return ((Flags & (uint16_t)AssetFlag::Missing) | (Flags & (uint16_t)AssetFlag::Invalid)) == 0; }

        virtual bool operator==(const Asset& other) const
        {
            return Handle == other.Handle;
        }

        virtual bool operator!=(const Asset& other) const
        {
            return !(*this == other);
        }

        bool IsFlagSet(AssetFlag flag) const { return (uint16_t)flag & Flags; }
        void SetFlag(AssetFlag flag, bool value = true)
        {
            if(value)
                Flags |= (uint16_t)flag;
            else
                Flags &= ~(uint16_t)flag;
        }

        UUID Handle;
    };

}
