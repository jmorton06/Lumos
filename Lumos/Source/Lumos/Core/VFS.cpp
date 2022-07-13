#include "Precompiled.h"
#include "VFS.h"
#include "StringUtilities.h"
#include "OS/FileSystem.h"

namespace Lumos
{
    void VFS::Mount(const std::string& virtualPath, const std::string& physicalPath)
    {
        m_MountPoints[virtualPath].push_back(physicalPath);
    }

    void VFS::Unmount(const std::string& path)
    {
        m_MountPoints[path].clear();
    }

    bool VFS::ResolvePhysicalPath(const std::string& path, std::string& outPhysicalPath, bool folder)
    {
        std::string updatedPath = path;
        std::replace(updatedPath.begin(), updatedPath.end(), '\\', '/');

        if(!(path[0] == '/' && path[1] == '/'))
        {
            outPhysicalPath = path;
            return folder ? FileSystem ::FolderExists(updatedPath) : FileSystem::FileExists(updatedPath);
        }

        static std::string delimiter  = "/";
        std::vector<std::string> dirs = StringUtilities::SplitString(updatedPath, delimiter);
        const std::string& virtualDir = dirs.front();

        if(m_MountPoints.find(virtualDir) == m_MountPoints.end() || m_MountPoints[virtualDir].empty())
        {
            outPhysicalPath = updatedPath;
            return folder ? FileSystem::FolderExists(updatedPath) : FileSystem::FileExists(updatedPath);
        }

        const std::string remainder = updatedPath.substr(virtualDir.size() + 2, updatedPath.size() - virtualDir.size());
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
        std::string physicalPath;
        return ResolvePhysicalPath(path, physicalPath) ? FileSystem::ReadFile(physicalPath) : nullptr;
    }

    std::string VFS::ReadTextFile(const std::string& path)
    {
        std::string physicalPath;
        return ResolvePhysicalPath(path, physicalPath) ? FileSystem::ReadTextFile(physicalPath) : "";
    }

    bool VFS::WriteFile(const std::string& path, uint8_t* buffer, uint32_t size)
    {
        std::string physicalPath;
        return ResolvePhysicalPath(path, physicalPath) ? FileSystem::WriteFile(physicalPath, buffer, size) : false;
    }

    bool VFS::WriteTextFile(const std::string& path, const std::string& text)
    {
        std::string physicalPath;
        return ResolvePhysicalPath(path, physicalPath) ? FileSystem::WriteTextFile(physicalPath, text) : false;
    }

    bool VFS::AbsoulePathToVFS(const std::string& path, std::string& outVFSPath, bool folder)
    {
        std::string updatedPath = path;
        std::replace(updatedPath.begin(), updatedPath.end(), '\\', '/');

        for(auto const& [key, val] : m_MountPoints)
        {
            for(auto& vfsPath : val)
            {
                if(updatedPath.find(vfsPath) != std::string::npos)
                {
                    std::string newPath     = updatedPath;
                    std::string newPartPath = "//" + key;
                    newPath.replace(0, vfsPath.length(), newPartPath);
                    outVFSPath = newPath;
                    return true;
                }
            }
        }

        outVFSPath = updatedPath;
        return false;
    }
}
