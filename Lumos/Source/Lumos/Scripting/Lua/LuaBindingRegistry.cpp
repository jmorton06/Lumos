#include "Precompiled.h"
#include "LuaBindingRegistry.h"
#include "Core/OS/Memory.h"

namespace Lumos::LuaRegistry
{
    static Arena*        s_Arena   = nullptr;
    static TDArray<Entry>* s_Entries = nullptr;

    static String8 Intern(const char* s)
    {
        if(!s || !*s) return {};
        return PushStr8Copy(s_Arena, s);
    }

    static bool Str8Eq(String8 a, String8 b)
    {
        if(a.size != b.size) return false;
        if(a.size == 0)      return true;
        return memcmp(a.str, b.str, a.size) == 0;
    }

    void Init()
    {
        if(s_Arena) return;
        s_Arena   = ArenaAlloc(1024 * 64);
        s_Entries = new TDArray<Entry>();
        s_Entries->Reserve(512);
    }

    void Shutdown()
    {
        if(s_Entries) { delete s_Entries; s_Entries = nullptr; }
        if(s_Arena)   { ArenaRelease(s_Arena); s_Arena = nullptr; }
    }

    void Clear()
    {
        if(s_Entries) s_Entries->Clear();
        if(s_Arena)   ArenaClear(s_Arena);
    }

    void AddMethod(const char* owner, const char* name, const char* returnType, const char* signature)
    {
        if(!s_Entries) Init();
        Entry e;
        e.Owner      = Intern(owner);
        e.Name       = Intern(name);
        e.ReturnType = Intern(returnType);
        e.Signature  = Intern(signature);
        e.Kind       = EntryKind::Method;
        s_Entries->PushBack(e);
    }

    void AddField(const char* owner, const char* name, const char* returnType)
    {
        if(!s_Entries) Init();
        Entry e;
        e.Owner      = Intern(owner);
        e.Name       = Intern(name);
        e.ReturnType = Intern(returnType);
        e.Kind       = EntryKind::Field;
        s_Entries->PushBack(e);
    }

    void AddGlobal(const char* name, const char* returnType, const char* signature)
    {
        if(!s_Entries) Init();
        Entry e;
        e.Name       = Intern(name);
        e.ReturnType = Intern(returnType);
        e.Signature  = Intern(signature);
        e.Kind       = EntryKind::Global;
        s_Entries->PushBack(e);
    }

    void AddConstructor(const char* typeName, const char* signature)
    {
        if(!s_Entries) Init();
        Entry e;
        e.Owner      = Intern(typeName);
        e.Name       = Intern(typeName);
        e.ReturnType = e.Owner;
        e.Signature  = Intern(signature);
        e.Kind       = EntryKind::Constructor;
        s_Entries->PushBack(e);
    }

    void AddEnum(const char* enumName, const char* valueName)
    {
        if(!s_Entries) Init();
        Entry e;
        e.Owner      = Intern(enumName);
        e.Name       = Intern(valueName);
        e.ReturnType = Intern("int");
        e.Kind       = EntryKind::EnumValue;
        s_Entries->PushBack(e);
    }

    const TDArray<Entry>& All()
    {
        if(!s_Entries) Init();
        return *s_Entries;
    }

    const Entry* FindMember(String8 owner, String8 name)
    {
        if(!s_Entries) return nullptr;
        for(size_t i = 0; i < s_Entries->Size(); ++i)
        {
            const Entry& e = (*s_Entries)[i];
            if(Str8Eq(e.Owner, owner) && Str8Eq(e.Name, name))
                return &e;
        }
        return nullptr;
    }

    bool IsType(String8 name)
    {
        if(!s_Entries) return false;
        for(size_t i = 0; i < s_Entries->Size(); ++i)
        {
            const Entry& e = (*s_Entries)[i];
            if(e.Kind == EntryKind::Constructor && Str8Eq(e.Owner, name))
                return true;
            if(e.Kind == EntryKind::EnumValue && Str8Eq(e.Owner, name))
                return true;
        }
        return false;
    }
}
