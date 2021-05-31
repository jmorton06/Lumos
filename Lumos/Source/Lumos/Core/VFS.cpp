#include "Precompiled.h"
#include "VFS.h"
#include "StringUtilities.h"
#include "OS/FileSystem.h"

namespace Lumos
{

    VFS* VFS::s_Instance = nullptr;

    void VFS::OnInit()
    {
        s_Instance = new VFS();
    }

    void VFS::OnShutdown()
    {
        delete s_Instance;
    }

    void VFS::Mount(const std::string& virtualPath, const std::string& physicalPath)
    {
        LUMOS_ASSERT(s_Instance, "");
        m_MountPoints[virtualPath].push_back(physicalPath);
    }

    void VFS::Unmount(const std::string& path)
    {
        LUMOS_ASSERT(s_Instance, "");
        m_MountPoints[path].clear();
    }

    bool VFS::ResolvePhysicalPath(const std::string& path, std::string& outPhysicalPath, bool folder)
    {
        if(!(path[0] == '/' && path[1] == '/'))
        {
            outPhysicalPath = path;
            return folder ? FileSystem ::FolderExists(path) : FileSystem::FileExists(path);
        }

        static std::string delimiter = "/";
        std::vector<std::string> dirs = StringUtilities::SplitString(path, delimiter);
        const std::string& virtualDir = dirs.front();

        if(m_MountPoints.find(virtualDir) == m_MountPoints.end() || m_MountPoints[virtualDir].empty())
        {
            outPhysicalPath = path;
            return folder ? FileSystem::FolderExists(path) : FileSystem::FileExists(path);
        }

        const std::string remainder = path.substr(virtualDir.size() + 2, path.size() - virtualDir.size());
        for(const std::string& physicalPath : m_MountPoints[virtualDir])
        {
            const std::string newPath = physicalPath + /* "/" +*/ remainder;
            if(folder ? FileSystem::FolderExists(newPath) : FileSystem::FileExists(newPath))
            {
                outPhysicalPath = newPath;
                return true;
            }
        }
        return false;
    }

    uint8_t* VFS::ReadFile(const std::string& path)
    {
        LUMOS_ASSERT(s_Instance, "");
        std::string physicalPath;
        return ResolvePhysicalPath(path, physicalPath) ? FileSystem::ReadFile(physicalPath) : nullptr;
    }

    std::string VFS::ReadTextFile(const std::string& path)
    {
        LUMOS_ASSERT(s_Instance, "");
        std::string physicalPath;
        return ResolvePhysicalPath(path, physicalPath) ? FileSystem::ReadTextFile(physicalPath) : nullptr;
    }

    bool VFS::WriteFile(const std::string& path, uint8_t* buffer)
    {
        LUMOS_ASSERT(s_Instance, "");
        std::string physicalPath;
        return ResolvePhysicalPath(path, physicalPath) ? FileSystem::WriteFile(physicalPath, buffer) : false;
    }

    bool VFS::WriteTextFile(const std::string& path, const std::string& text)
    {
        LUMOS_ASSERT(s_Instance, "");
        std::string physicalPath;
        return ResolvePhysicalPath(path, physicalPath) ? FileSystem::WriteTextFile(physicalPath, text) : false;
    }

    bool VFS::AbsoulePathToVFS(const std::string& path, std::string& outVFSPath, bool folder)
    {
        for(auto const& [key, val] : m_MountPoints)
        {
            for(auto& vfsPath : val)
            {
                if(path.find(vfsPath) != std::string::npos)
                {
                    std::string newPath = path;
                    std::string newPartPath = "//" + key;
                    newPath.replace(0, vfsPath.length(), newPartPath);
                    outVFSPath = newPath;
                    return true;
                }
            }
        }

        outVFSPath = path;
        return false;
    }
}
