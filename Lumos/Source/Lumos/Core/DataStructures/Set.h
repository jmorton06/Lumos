#pragma once
#include "Map.h"

namespace Lumos
{
    //
    // A HashSet is just a HashMap without values.
    //
    // HashSet example:
    //   Arena *arena = ...
    //
    //   HashSet(int) set = {0};
    //   set.arena = arena; // This is optional, otherwise the heap allocator will be used.
    //
    //   int foo = 20;
    //   if (HashSetAdd(&set, &foo)) { }
    //   if (HashSetAdd(&set, &foo)) { }
    //
    //   if (HashSetContains(&set, &foo)) { }
    //
    //   /* Iterate through the keys */
    //   ForHashSetEach(int, &set, it)
    //   {
    //      int key = *it.key;
    //      ...
    //   }
    //
    //   if (HashSetRemove(&set, &foo)) { }
    //   if (HashSetRemove(&set, &foo)) { }
    //
    //   HashSetDeinit(&set); // Reset the set and free the memory if using the heap allocator
    //

#define HashSet(K)         \
    struct                 \
    {                      \
        Arena* arena;      \
        struct             \
        {                  \
            uint32_t hash; \
            K key;         \
        }* data;           \
        int length;        \
        int capacity;      \
    }
    typedef HashSet(char) HashSetRaw;

#define HashSetInit(SET) HashMapInitRaw((HashMapRaw*)(SET))

#define HashSetContains(SET, KEY)    \
    (HashMapTypecheckK(SET, &(KEY)), \
     HashMapFindRaw((HashMapRaw*)SET, &(KEY), NULL, HashMapKSize(SET), 0, HashMapElemSize(SET), HashMapKOffset(SET), 0))

#define HashSetAdd(SET, KEY)         \
    (HashMapTypecheckK(SET, &(KEY)), \
     HashMapInsertRaw((HashMapRaw*)(SET), &(KEY), NULL, HashMapKSize(SET), 0, HashMapElemSize(SET), HashMapKOffset(SET), 0))

#define HashSetRemove(SET, KEY)      \
    (HashMapTypecheckK(SET, &(KEY)), \
     HashMapRemoveRaw((HashMapRaw*)(SET), &(KEY), HashMapKSize(SET), 0, HashMapElemSize(SET), HashMapKOffset(SET), 0))

#define ForHashSetEach(K, SET, IT)   \
    struct Concat(_dummy_, __LINE__) \
    {                                \
        int i_next;                  \
        K* elem;                     \
    } IT;                            \
    if((SET)->length > 0)            \
        for(IT = { 0 };              \
            HashMapIter((HashMapRaw*)(SET), &IT.i_next, (void**)&IT.elem, NULL, HashMapKOffset(SET), 0, HashMapElemSize(SET));)

#define HashSetClear(SET) \
    HashMapClearRaw((HashMapRaw*)(SET), HashMapElemSize(SET))

#define HashSetDeinit(SET) \
    HashMapDeinitRaw((HashMapRaw*)(SET), HashMapElemSize(SET))
}
