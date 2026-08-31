#pragma once
#include "Core/String.h"
#include "Core/DataStructures/TDArray.h"


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
