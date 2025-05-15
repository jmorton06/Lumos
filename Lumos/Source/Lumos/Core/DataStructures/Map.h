#pragma once
#include "Core/OS/Memory.h"
#include "Utilities/Hash.h"
#include <cstring>

namespace Lumos
{
    // Based on https://github.com/EeroMutka/Fire
    //
    // HashHashMap example:
    //   HashMap(int, float) map = {0};
    //   map.arena = arena; // If no arena is set, heap allocator used
    //
    //   int a = 2;
    //   float b = 1.0f;
    //   if (HashMapInsert(&map, &a, &b)) { }
    //   if (HashMapInsert(&map, &a, &b)) { }
    //
    //   float result_1;
    //   if (HashMapFind(&map, &a, &result_1)) { }
    //
    //   float *result_1_ptr;
    //   if (HashMapFindPtr(&map, &a, &result_1_ptr)) {  }
    //
    //   /* Iterate through the key-value pairs */
    //   ForHashMapEach(int, float, &map, it)
    //   {
    //      int key = *it.key;
    //      float value = *it.value;
    //      ...
    //   }
    //
    //   if (HashMapRemove(&map, &foo)) { }
    //   if (HashMapRemove(&map, &foo)) { }
    //
    //   int key = -100;
    //   float* value;
    //   if (HashMapGetOrAddPtr(&map, &key, &value))
    //   {
    //      *value = 123.4f;
    //   }
    //   else
    //   {
    //   }
    //
    //   HashMapDeinit(&map); // Reset the map and free the memory if using the heap allocator
    //

#ifndef MAX_MAP_SLOT_SIZE
#define MAX_MAP_SLOT_SIZE 4096
#endif

#define HashMapTypecheckK(MAP, PTR) ((PTR) == &(MAP)->data->key)
#define HashMapTypecheckV(MAP, PTR) ((PTR) == &(MAP)->data->value)
#define HashMapKSize(MAP) sizeof((MAP)->data->key)
#define HashMapVSize(MAP) sizeof((MAP)->data->value)
#define HashMapElemSize(MAP) sizeof(*(MAP)->data)
#define HashMapKOffset(MAP) (int)((uintptr_t)&(MAP)->data->key - (uintptr_t)(MAP)->data)
#define HashMapVOffset(MAP) (int)((uintptr_t)&(MAP)->data->value - (uintptr_t)(MAP)->data)

#define HashMap(K, V)      \
    struct                 \
    {                      \
        Arena* arena;      \
        struct             \
        {                  \
            uint32_t hash; \
            K key;         \
            V value;       \
        }* data;           \
        int length;        \
        int capacity;      \
    }
    typedef HashMap(char, char) HashMapRaw;

#define HashMapInit(MAP) HashMapInitRaw((HashMapRaw*)(MAP))

#define HashMapFind(MAP, KEY, OUT_VALUE)                                  \
    (HashMapTypecheckK(MAP, &(KEY)) && HashMapTypecheckV(MAP, OUT_VALUE), \
     HashMapFindRaw((HashMapRaw*)MAP, &(KEY), OUT_VALUE, HashMapKSize(MAP), HashMapVSize(MAP), HashMapElemSize(MAP), HashMapKOffset(MAP), HashMapVOffset(MAP)))

#define HashMapFindPtr(MAP, KEY)     \
    (HashMapTypecheckK(MAP, &(KEY)), \
     HashMapFindPtrRaw((HashMapRaw*)MAP, &(KEY), HashMapKSize(MAP), HashMapVSize(MAP), HashMapElemSize(MAP), HashMapKOffset(MAP), HashMapVOffset(MAP)))

#define HashMapInsert(MAP, KEY, VALUE)                                   \
    (HashMapTypecheckK(MAP, &(KEY)) && HashMapTypecheckV(MAP, &(VALUE)), \
     HashMapInsertRaw((HashMapRaw*)(MAP), &(KEY), &(VALUE), HashMapKSize(MAP), HashMapVSize(MAP), HashMapElemSize(MAP), HashMapKOffset(MAP), HashMapVOffset(MAP)))

#define HashMapRemove(MAP, KEY)      \
    (HashMapTypecheckK(MAP, &(KEY)), \
     HashMapRemoveRaw((HashMapRaw*)(MAP), &(KEY), HashMapKSize(MAP), HashMapVSize(MAP), HashMapElemSize(MAP), HashMapKOffset(MAP), HashMapVOffset(MAP)))

#define HashMapGetOrAddPtr(MAP, KEY, OUT_VALUE)                              \
    (HashMapTypecheckK(MAP, &(KEY)) && HashMapTypecheckV(MAP, *(OUT_VALUE)), \
     HashMapGetOrAddRaw((HashMapRaw*)(MAP), &(KEY), (void**)OUT_VALUE, HashMapKSize(MAP), HashMapVSize(MAP), HashMapElemSize(MAP), HashMapKOffset(MAP), HashMapVOffset(MAP)))

#define HashMapClear(MAP) \
    HashMapClearRaw((HashMapRaw*)(MAP), HashMapElemSize(MAP))

#define HashMapDeinit(MAP) \
    HashMapDeinitRaw((HashMapRaw*)(MAP), HashMapElemSize(MAP))

#define ForHashMapEach(K, V, MAP, IT)                    \
    struct Concat(_dummy_, __LINE__)                     \
    {                                                    \
        int i_next;                                      \
        K* key;                                          \
        V* value;                                        \
    };                                                   \
    if((MAP)->length > 0)                                \
        for(struct Concat(_dummy_, __LINE__) IT = { 0 }; \
            HashMapIter((HashMapRaw*)(MAP), &IT.i_next, (void**)&IT.key, (void**)&IT.value, HashMapKOffset(MAP), HashMapVOffset(MAP), HashMapElemSize(MAP));)

    bool HashMapGetOrAddRawEx(HashMapRaw* map, const void* key, void** out_val_ptr, int K_size, int V_size, int elem_size, int key_offset, int val_offset, uint32_t hash);
    bool HashMapRemoveRaw(HashMapRaw* map, const void* key, int K_size, int V_size, int elem_size, int key_offset, int val_offset);
    void* HashMapFindPtrRaw(HashMapRaw* map, const void* key, int K_size, int V_size, int elem_size, int key_offset, int val_offset);
    bool HashMapGetOrAddRaw(HashMapRaw* map, const void* key, void** out_val_ptr, int K_size, int V_size, int elem_size, int key_offset, int val_offset);
    bool HashMapInsertRaw(HashMapRaw* map, const void* key, void* val, int K_size, int V_size, int elem_size, int key_offset, int val_offset);
    bool HashMapFindRaw(HashMapRaw* map, const void* key, void* out_val, int K_size, int V_size, int elem_size, int key_offset, int val_offset);
    bool HashMapIter(HashMapRaw* map, int* i, void** out_key, void** out_value, int key_offset, int val_offset, int elem_size);

    static inline void HashMapInitRaw(HashMapRaw* map)
    {
        HashMapRaw empty = { 0 };
        *map             = empty;
    }

    static inline void HashMapClearRaw(HashMapRaw* map, int elem_size)
    {
        memset(map->data, 0, map->capacity * elem_size);
        map->length = 0;
    }

    static inline void HashMapDeinitRaw(HashMapRaw* map)
    {
        if(!map->arena && map->data)
        {
            delete[] map->data;
        }

        HashMapRaw empty = { 0 };
        *map             = empty;
    }

}
