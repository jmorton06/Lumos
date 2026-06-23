#pragma once
#include "Core/String.h"
#include "Core/DataStructures/TDArray.h"

// Typed catalogue of every binding exposed to Lua. Populated once at startup;
// the editor uses it to drive autocomplete + type-chain inference.
//
// Owner is the type name on which the entry hangs:
//   ""       -> global symbol (function, value, table)
//   "Vec3"   -> method/field of Vec3
//   "Entity" -> Entity method (AddX / GetX / HasX / etc.)
//
// ReturnType is the Lua-side type name (matches Owner of subsequent chained
// entries). Use "" for void / no return.
//
// All strings are interned in the registry arena; Entry copies are cheap.

namespace Lumos::LuaRegistry
{
    enum class EntryKind
    {
        Global,        // free function or global value
        Method,        // owner-bound function
        Field,         // owner-bound data field (read/write)
        Constructor,   // typename callable like Vec3.new / Vec3()
        EnumValue,     // member of a sol::new_enum table
    };

    struct Entry
    {
        String8   Owner      = {};
        String8   Name       = {};
        String8   ReturnType = {};
        String8   Signature  = {};
        EntryKind Kind       = EntryKind::Global;
    };

    void Init();
    void Shutdown();
    void Clear();

    void AddMethod(const char* owner, const char* name, const char* returnType, const char* signature = "");
    void AddField(const char* owner, const char* name, const char* returnType);
    void AddGlobal(const char* name, const char* returnType, const char* signature = "");
    void AddConstructor(const char* typeName, const char* signature = "");
    void AddEnum(const char* enumName, const char* valueName);

    const TDArray<Entry>& All();
    const Entry*          FindMember(String8 owner, String8 name);
    bool                  IsType(String8 name);
}
