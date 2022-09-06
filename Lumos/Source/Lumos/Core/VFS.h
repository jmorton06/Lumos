#pragma once
#include "Utilities/TSingleton.h"
#include <map>

namespace Lumos
{
    class LUMOS_EXPORT VFS : public ThreadSafeSingleton<VFS>
    {
        friend class ThreadSafeSingleton<VFS>;

    public:
        void Mount(const std::string& virtualPath, const std::string& physicalPath);
        void Unmount(const std::string& path);
        bool ResolvePhysicalPath(const std::string& path, std::string& outPhysicalPath, bool folder = false);
        bool AbsoulePathToVFS(const std::string& path, std::string& outVFSPath, bool folder = false);
        inline std::string AbsoulePathToVFS(const std::string& path, bool folder = false)
        {
            std::string returnString;
            AbsoulePathToVFS(path, returnString, folder);
            return returnString;
        }

        uint8_t* ReadFile(const std::string& path);
        std::string ReadTextFile(const std::string& path);

        bool WriteFile(const std::string& path, uint8_t* buffer, uint32_t size);
        bool WriteTextFile(const std::string& path, const std::string& text);

    private:
        std::map<std::string, std::vector<std::string>, std::less<void>> m_MountPoints;
    };
}
