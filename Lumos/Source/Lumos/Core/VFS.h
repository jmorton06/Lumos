#pragma once

#include <unordered_map>

namespace Lumos
{
    class LUMOS_EXPORT VFS
    {
    private:
        static VFS* s_Instance;

    private:
        std::unordered_map<std::string, std::vector<std::string>> m_MountPoints;

    public:
        void Mount(const std::string& virtualPath, const std::string& physicalPath);
        void Unmount(const std::string& path);
        bool ResolvePhysicalPath(const std::string& path, std::string& outPhysicalPath, bool folder = false);
        bool AbsoulePathToVFS(const std::string& path, std::string& outVFSPath, bool folder = false);

        uint8_t* ReadFile(const std::string& path);
        std::string ReadTextFile(const std::string& path);

        bool WriteFile(const std::string& path, uint8_t* buffer);
        bool WriteTextFile(const std::string& path, const std::string& text);

    public:
        static void OnInit();
        static void OnShutdown();

        inline static VFS* Get()
        {
            return s_Instance;
        }
    };
}
