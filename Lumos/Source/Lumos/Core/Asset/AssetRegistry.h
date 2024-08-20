#pragma once
#include "AssetMetaData.h"

namespace Lumos
{
    class AssetRegistry
    {
        template <typename Archive>
        friend void save(Archive& archive, const AssetRegistry& registry);

        template <typename Archive>
        friend void load(Archive& archive, AssetRegistry& registry);

    public:
        AssetMetaData& operator[](const UUID handle);
        AssetMetaData& Get(const UUID handle);
        const AssetMetaData& Get(const UUID handle) const;

        void Update(float elapsedSeconds);

        size_t Count() const { return m_AssetRegistry.size(); }
        bool Contains(const UUID handle) const;
        size_t Remove(const UUID handle);
        void Clear();

        auto begin() { return m_AssetRegistry.begin(); }
        auto end() { return m_AssetRegistry.end(); }
        auto begin() const { return m_AssetRegistry.cbegin(); }
        auto end() const { return m_AssetRegistry.cend(); }

        void AddName(const std::string& name, UUID ID)
        {
            m_NameMap.emplace(name, ID);
#ifndef LUMOS_PRODUCTION
            m_UUIDNameMap.emplace(ID, name);
#endif
        }
        bool GetID(const std::string& name, UUID& ID)
        {
            if(m_NameMap.find(name) != m_NameMap.end())
            {
                ID = m_NameMap[name];
                return true;
            }

            return false;
        }

#ifndef LUMOS_PRODUCTION
        bool GetName(UUID ID, std::string& name)
        {
            if(m_UUIDNameMap.find(ID) != m_UUIDNameMap.end())
            {
                name = m_UUIDNameMap[ID];
                return true;
            }

            return false;
        }
#endif

        void ReplaceID(UUID current, UUID newID);

    private:
        std::unordered_map<UUID, AssetMetaData> m_AssetRegistry;
        std::unordered_map<std::string, UUID> m_NameMap;
#ifndef LUMOS_PRODUCTION
        std::unordered_map<UUID, std::string> m_UUIDNameMap; // Debug Only
#endif

        float m_ExpirationTime = 3.0f;
    };
}