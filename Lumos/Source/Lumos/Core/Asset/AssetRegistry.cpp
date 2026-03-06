#include "Precompiled.h"
#include "AssetRegistry.h"
#include "Asset.h"
#include "Core/Mutex.h"
#include "Utilities/StringPool.h"
#include "Utilities/Hash.h"

namespace Lumos
{

    AssetRegistry::AssetRegistry()
    {
        m_Arena = ArenaAlloc(Megabytes(8));
        HashMapInit(&m_AssetRegistry);
        m_AssetRegistry.arena = m_Arena;

        HashMapInit(&m_NameMap);
        m_NameMap.arena = m_Arena;

        HashMapInit(&m_UUIDNameMap);
        m_UUIDNameMap.arena = m_Arena;

        m_Mutex = PushArray(m_Arena, Mutex, 1);
        MutexInit(m_Mutex);

        m_StringPool = new StringPool(m_Arena);
    }

    AssetRegistry::~AssetRegistry()
    {
        ForHashMapEach(UUID, AssetMetaData, &m_AssetRegistry, it)
        {
            it.value->Data = nullptr;
        }

        delete m_StringPool;

        MutexDestroy(m_Mutex);
        ArenaRelease(m_Arena);
    }

    void AssetRegistry::Update(float elapsedSeconds)
    {
        static UUID keysToDelete[256];
        u32 keysToDeleteCount = 0;

        ForHashMapEach(UUID, AssetMetaData, &m_AssetRegistry, it)
        {
            UUID key             = *it.key;
            AssetMetaData& value = *it.value;

            if(value.Expire && value.IsDataLoaded && value.Data.GetCounter()->GetReferenceCount() == 1
               && m_ExpirationTime < (elapsedSeconds - value.LastAccessed))
            {
                keysToDelete[keysToDeleteCount++] = key;
            }

            if(keysToDeleteCount >= 256)
                break;
        }

        for(u32 i = 0; i < keysToDeleteCount; i++)
        {
            AssetMetaData* meta = (AssetMetaData*)HashMapFindPtr(&m_AssetRegistry, keysToDelete[i]);
            if(meta)
                meta->Data = nullptr;
            HashMapRemove(&m_AssetRegistry, keysToDelete[i]);
        }
    }

    void AssetRegistry::ReplaceID(UUID current, UUID newID)
    {
        AssetMetaData* metaData = (AssetMetaData*)HashMapFindPtr(&m_AssetRegistry, current);
        if(metaData)
        {
            AssetMetaData& data = *metaData;
            HashMapInsert(&m_AssetRegistry, newID, data);
            HashMapRemove(&m_AssetRegistry, current);
        }

        String8 name;

        if(GetName(current, name))
        {
            u64 StringHash = MurmurHash64A(name.str, (i32)name.size, 0);

            HashMapInsert(&m_NameMap, StringHash, newID);
            HashMapInsert(&m_UUIDNameMap, newID, name);
            HashMapRemove(&m_UUIDNameMap, current);
        }
    }

    void AssetRegistry::AddName(const String8& name, UUID ID)
    {
        String8 StringCopy = m_StringPool->Allocate((const char*)name.str);
        u64 StringHash     = MurmurHash64A(name.str, (i32)name.size, 0);

        HashMapInsert(&m_NameMap, StringHash, ID);
        HashMapInsert(&m_UUIDNameMap, ID, StringCopy);
    }

    bool AssetRegistry::GetID(const String8& name, UUID& ID)
    {
        u64 StringHash = MurmurHash64A(name.str, (i32)name.size, 0);
        UUID* idPtr    = (UUID*)HashMapFindPtr(&m_NameMap, StringHash);
        if(idPtr)
        {
            ID = *idPtr;
            return true;
        }
        return false;
    }

    bool AssetRegistry::GetName(UUID ID, String8& name) const
    {
        String8* namePtr = (String8*)HashMapFindPtr(&m_UUIDNameMap, ID);
        if(namePtr)
        {
            name = *namePtr;
            return true;
        }
        return false;
    }

    AssetMetaData& AssetRegistry::operator[](UUID handle)
    {
        if(!Contains(handle))
        {
            ScopedMutex mutex(m_Mutex);
            AssetMetaData data;
            HashMapInsert(&m_AssetRegistry, handle, data);
        }
        return Get(handle);
    }

    const AssetMetaData& AssetRegistry::Get(UUID handle) const
    {
        if(!Contains(handle))
        {
            AssetMetaData data;
            ScopedMutex mutex(m_Mutex);
            HashMapInsert(&m_AssetRegistry, handle, data);
        }

        return *(AssetMetaData*)HashMapFindPtr(&m_AssetRegistry, handle);
    }

    AssetMetaData& AssetRegistry::Get(UUID handle)
    {
        ScopedMutex mutex(m_Mutex);
        AssetMetaData* dataPtr = (AssetMetaData*)HashMapFindPtr(&m_AssetRegistry, handle);

        return *dataPtr;
    }

    bool AssetRegistry::Contains(UUID handle) const
    {
        ScopedMutex mutex(m_Mutex);
        return HashMapFindPtr(&m_AssetRegistry, handle) != nullptr;
    }

    void AssetRegistry::Remove(UUID handle)
    {
        ScopedMutex mutex(m_Mutex);
        AssetMetaData* meta = (AssetMetaData*)HashMapFindPtr(&m_AssetRegistry, handle);
        if(meta)
            meta->Data = nullptr;
        HashMapRemove(&m_AssetRegistry, handle);
    }

    void AssetRegistry::Clear()
    {
        ScopedMutex mutex(m_Mutex);

        ForHashMapEach(UUID, AssetMetaData, &m_AssetRegistry, it)
        {
            it.value->Data = nullptr;
        }

        HashMapClear(&m_AssetRegistry);
    }

    void AssetRegistry::ClearProjectAssets()
    {
        ScopedMutex mutex(m_Mutex);

        static UUID keysToDelete[1024];
        u32 keysToDeleteCount = 0;

        ForHashMapEach(UUID, AssetMetaData, &m_AssetRegistry, it)
        {
            if(!it.value->IsEngineAsset)
            {
                keysToDelete[keysToDeleteCount++] = *it.key;
                if(keysToDeleteCount >= 1024)
                    break;
            }
        }

        for(u32 i = 0; i < keysToDeleteCount; i++)
        {
            // Remove name mappings for this asset
            String8 name;
            if(GetName(keysToDelete[i], name))
            {
                u64 nameHash = MurmurHash64A(name.str, (i32)name.size, 0);
                HashMapRemove(&m_NameMap, nameHash);
                HashMapRemove(&m_UUIDNameMap, keysToDelete[i]);
            }
            AssetMetaData* meta = (AssetMetaData*)HashMapFindPtr(&m_AssetRegistry, keysToDelete[i]);
            if(meta)
                meta->Data = nullptr;
            HashMapRemove(&m_AssetRegistry, keysToDelete[i]);
        }
    }

}
