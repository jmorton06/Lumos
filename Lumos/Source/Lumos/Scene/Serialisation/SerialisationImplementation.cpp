#include "Precompiled.h"
#include "SerialisationImplementation.h"
#include <cereal/archives/json.hpp>

namespace Lumos
{
    void SerialiseAssetRegistry(const String8& path, const AssetRegistry& registry)
    {
        std::stringstream storage;
        {
            // output finishes flushing its contents when it goes out of scope
            cereal::JSONOutputArchive output { storage };
            output(registry);
        }

        LINFO("Serialising Asset Registry %s", (const char*)path.str);
        FileSystem::WriteTextFile(path, Str8StdS(storage.str()));
    }

    void DeserialiseAssetRegistry(const String8& path, AssetRegistry& registry)
    {
        ArenaTemp temp = ScratchBegin(nullptr, 0);
        String8 data   = FileSystem::ReadTextFile(temp.arena, path);
        std::istringstream istr;
        istr.str((const char*)data.str);
        try
        {
            cereal::JSONInputArchive input(istr);
            input(registry);
        }
        catch(...)
        {
            LWARN("Failed to load asset registry %s", (const char*)path.str);
        }
        ScratchEnd(temp);
    }
}
