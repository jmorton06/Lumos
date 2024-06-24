#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
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
}
