#pragma once
#include "AssetMetaData.h"
#include "Core/UUID.h"
#include "Core/DataStructures/Map.h"

namespace Lumos
{
    struct Mutex;
    class StringPool;
    class AssetRegistry
    {
        friend class AssetManagerPanel;
        template <typename Archive>
        friend void save(Archive& archive, const AssetRegistry& registry);

        template <typename Archive>
        friend void load(Archive& archive, AssetRegistry& registry);

    public:
        AssetRegistry();
        ~AssetRegistry();

        AssetMetaData& operator[](const UUID handle);
        AssetMetaData& Get(const UUID handle);
        const AssetMetaData& Get(const UUID handle) const;

        void Update(float elapsedSeconds);

        bool Contains(const UUID handle) const;
        void Remove(const UUID handle);
        void Clear();

        void AddName(const String8& name, UUID ID);
        bool GetID(const String8& name, UUID& ID);

        bool GetName(UUID ID, String8& name) const;

        void ReplaceID(UUID current, UUID newID);

    private:
        HashMap(UUID, AssetMetaData) m_AssetRegistry;
        HashMap(u64, UUID) m_NameMap;
        HashMap(UUID, String8) m_UUIDNameMap;

        Arena* m_Arena;
        Mutex* m_Mutex;

        StringPool* m_StringPool;

        float m_ExpirationTime = 3.0f;
    };
}
