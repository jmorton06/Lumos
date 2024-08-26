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
        FileSystem::WriteTextFile((const char*)path.str, storage.str());
    }

    void DeserialiseAssetRegistry(const String8& path, AssetRegistry& registry)
    {
        std::string data = FileSystem::ReadTextFile((const char*)path.str);
        std::istringstream istr;
        istr.str(data);
        try
        {
            cereal::JSONInputArchive input(istr);
            input(registry);
        }
        catch(...)
        {
            LWARN("Failed to load asset registry %s", (const char*)path.str);
        }
    }
}
