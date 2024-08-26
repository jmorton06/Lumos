#include "Precompiled.h"
#include "AssetRegistry.h"
#include "Asset.h"

namespace Lumos
{
    void AssetRegistry::Update(float elapsedSeconds)
    {
        static UUID keysToDelete[256];
        std::size_t keysToDeleteCount = 0;

        for(auto&& [key, value] : m_AssetRegistry)
        {
            if(value.Expire && value.IsDataLoaded && value.data.GetCounter()->GetReferenceCount() == 1 && m_ExpirationTime < (elapsedSeconds - value.lastAccessed))
            {
                keysToDelete[keysToDeleteCount] = key;
                keysToDeleteCount++;
            }
        }

        for(std::size_t i = 0; i < keysToDeleteCount; i++)
        {
            m_AssetRegistry.erase(keysToDelete[i]);
        }
    }

    void AssetRegistry::ReplaceID(UUID current, UUID newID)
    {
        auto metaData = m_AssetRegistry[current];
        m_AssetRegistry[newID] = metaData;
        m_AssetRegistry.erase(current);
        std::string name;

#ifndef LUMOS_PRODUCTION
        if(GetName(current, name))
        {
            m_NameMap[name] = newID;
            m_UUIDNameMap[newID] = name;
            m_UUIDNameMap.erase(current);
        }
#endif
    }
}
