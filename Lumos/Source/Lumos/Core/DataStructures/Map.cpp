#include "Precompiled.h"
#include "Map.h"

namespace Lumos
{
    bool HashMapGetOrAddRawEx(HashMapRaw* map, const void* key, void** out_val_ptr, int K_size, int V_size, int elem_size, int key_offset, int val_offset, uint32_t hash)
    {
        if(100 * (map->length + 1) > 70 * map->capacity)
        {
            char* old_data   = (char*)map->data;
            int old_capacity = map->capacity;

            map->capacity = old_capacity == 0 ? 8 : old_capacity * 2;
            map->length   = 0;

            void* new_data = map->arena ? ArenaPush(map->arena, (uint64_t)(map->capacity * elem_size)) : new char[map->capacity * elem_size]; // AllocatorFn(map->allocator, NULL, 0, map->capacity * elem_size, DEFAULT_ALIGNMENT);

            memcpy(&map->data, &new_data, sizeof(void*));
            memset(map->data, 0, map->capacity * elem_size); // set hash values to 0

            for(int i = 0; i < old_capacity; i++)
            {
                char* elem         = old_data + elem_size * i;
                uint32_t elem_hash = *(uint32_t*)elem;
                void* elem_key     = elem + key_offset;
                void* elem_val     = elem + val_offset;

                if(elem_hash != 0)
                {
                    void* new_val_ptr;
                    HashMapGetOrAddRawEx(map, elem_key, &new_val_ptr, K_size, V_size, elem_size, key_offset, val_offset, elem_hash);
                    memcpy(new_val_ptr, elem_val, V_size);
                }
            }

            // free the old data if not created with an arena
            if(!map->arena)
                delete[] old_data;
        }

        uint32_t mask = (uint32_t)map->capacity - 1;
        uint32_t idx  = hash & mask;

        bool added_new = false;

        for(;;)
        {
            char* elem_base = (char*)map->data + idx * elem_size;

            uint32_t* elem_hash = (uint32_t*)elem_base;
            char* elem_key      = elem_base + key_offset;
            char* elem_val      = elem_base + val_offset;

            if(*elem_hash == 0)
            {
                // We found an empty slot
                memcpy(elem_key, key, K_size);
                *elem_hash = hash;

                if(out_val_ptr)
                    *out_val_ptr = elem_val;
                map->length++;
                added_new = true;
                break;
            }

            if(hash == *elem_hash && memcmp(key, elem_key, K_size) == 0)
            {
                // This key already exists
                if(out_val_ptr)
                    *out_val_ptr = elem_val;
                break;
            }

            idx = (idx + 1) & mask;
        }

        return added_new;
    }

    bool HashMapRemoveRaw(HashMapRaw* map, const void* key, int K_size, int V_size, int elem_size, int key_offset, int val_offset)
    {
        if(map->capacity == 0)
            return false;

        uint32_t hash = MurmurHash3((char*)key, K_size, 989898);
        if(hash == 0)
            hash = 1;

        uint32_t mask  = (uint32_t)map->capacity - 1;
        uint32_t index = hash & mask;

        char temp[MAX_MAP_SLOT_SIZE];
        LUMOS_ASSERT(MAX_MAP_SLOT_SIZE >= elem_size);

        bool ok = true;
        for(;;)
        {
            char* elem_base    = (char*)map->data + index * elem_size;
            uint32_t elem_hash = *(uint32_t*)elem_base;

            if(elem_hash == 0)
            {
                // Empty slot, the key does not exist in the map
                ok = false;
                break;
            }

            if(hash == elem_hash && memcmp(key, elem_base + key_offset, K_size) == 0)
            {
                // Remove element
                memset(elem_base, 0, elem_size);
                map->length--;

                // backwards-shift deletion.
                // First remove all elements directly after this element from the map, then add them back to the map, starting from the first one to the right.
                for(;;)
                {
                    index = (index + 1) & mask;

                    char* shifting_elem_base    = (char*)map->data + index * elem_size;
                    uint32_t shifting_elem_hash = *(uint32_t*)shifting_elem_base;
                    if(shifting_elem_hash == 0)
                        break;

                    memcpy(temp, shifting_elem_base, elem_size);
                    memset(shifting_elem_base, 0, elem_size);
                    map->length--;

                    void* new_val_ptr;
                    HashMapGetOrAddRawEx(map, temp + key_offset, &new_val_ptr, K_size, V_size, elem_size, key_offset, val_offset, shifting_elem_hash);
                    memcpy(new_val_ptr, temp + val_offset, V_size);
                }

                break;
            }

            index = (index + 1) & mask;
        }

        return ok;
    }

    void* HashMapFindPtrRaw(HashMapRaw* map, const void* key, int K_size, int V_size, int elem_size, int key_offset, int val_offset)
    {
        if(map->capacity == 0)
            return NULL;

        uint32_t hash = MurmurHash3(key, K_size, 989898);
        if(hash == 0)
            hash = 1;

        uint32_t mask  = (uint32_t)map->capacity - 1;
        uint32_t index = hash & mask;

        void* found = NULL;
        for(;;)
        {
            char* elem         = (char*)map->data + index * elem_size;
            uint32_t elem_hash = *(uint32_t*)elem;
            if(elem_hash == 0)
                break;

            if(hash == elem_hash && memcmp(key, elem + key_offset, K_size) == 0)
            {
                found = elem + val_offset;
                break;
            }

            index = (index + 1) & mask;
        }

        return found;
    }

    bool HashMapGetOrAddRaw(HashMapRaw* map, const void* key, void** out_val_ptr, int K_size, int V_size, int elem_size, int key_offset, int val_offset)
    {
        uint32_t hash = MurmurHash3((char*)key, K_size, 989898);
        if(hash == 0)
            hash = 1;
        bool result = HashMapGetOrAddRawEx(map, key, out_val_ptr, K_size, V_size, elem_size, key_offset, val_offset, hash);
        return result;
    }

    bool HashMapInsertRaw(HashMapRaw* map, const void* key, void* val, int K_size, int V_size, int elem_size, int key_offset, int val_offset)
    {
        void* val_ptr;
        bool added = HashMapGetOrAddRaw(map, key, &val_ptr, K_size, V_size, elem_size, key_offset, val_offset);
        memcpy(val_ptr, val, V_size);
        return added;
    }

    bool HashMapFindRaw(HashMapRaw* map, const void* key, void* out_val, int K_size, int V_size, int elem_size, int key_offset, int val_offset)
    {
        void* ptr = HashMapFindPtrRaw(map, key, K_size, V_size, elem_size, key_offset, val_offset);
        if(ptr)
        {
            if(out_val)
                memcpy(out_val, ptr, V_size);
        }
        return ptr != NULL;
    }

    bool HashMapIter(HashMapRaw* map, int* i, void** out_key, void** out_value, int key_offset, int val_offset, int elem_size)
    {
        char* elem_base;
        for(;;)
        {
            if(*i >= map->capacity)
                return false;

            elem_base = (char*)map->data + (*i) * elem_size;
            if(*(uint32_t*)elem_base == 0)
            {
                *i = *i + 1;
                continue;
            }

            break;
        }
        *out_key = elem_base + key_offset;
        if(out_value)
            *out_value = elem_base + val_offset;
        *i = *i + 1;
        return true;
    }
}
