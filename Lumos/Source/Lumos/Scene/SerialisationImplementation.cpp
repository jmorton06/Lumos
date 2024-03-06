#include "Precompiled.h"
#include "SerialisationImplementation.h"
#include <cereal/archives/json.hpp>

namespace Lumos
{
    void SerialialiseAssetRegistry(const String8& path, const AssetRegistry& registry)
    {
        std::stringstream storage;
        {
            // output finishes flushing its contents when it goes out of scope
            cereal::JSONOutputArchive output { storage };
            output(registry);
        }

        LUMOS_LOG_INFO("Serialising Asset Registry {0}", (const char*)path.str);
        FileSystem::WriteTextFile((const char*)path.str, storage.str());
    }

    void DeserialialiseAssetRegistry(const String8& path, AssetRegistry& registry)
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
            LUMOS_LOG_WARN("Failed to load asset registry {0}", (const char*)path.str);
        }
    }
}
