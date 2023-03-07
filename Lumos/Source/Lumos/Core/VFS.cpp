#include "Precompiled.h"
#include "VFS.h"
#include "StringUtilities.h"
#include "OS/FileSystem.h"

namespace Lumos
{
    void VFS::Mount(const std::string& virtualPath, const std::string& physicalPath, bool replace)
    {
        LUMOS_PROFILE_FUNCTION();

        if(replace)
            m_MountPoints[virtualPath].clear();

        m_MountPoints[virtualPath].push_back(physicalPath);
    }

    void VFS::Unmount(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();
        m_MountPoints[path].clear();
    }

    bool VFS::ResolvePhysicalPath(const std::string& path, std::string& outPhysicalPath, bool folder)
    {
        LUMOS_PROFILE_FUNCTION();
        const std::string& updatedPath = path;
        // std::replace(updatedPath.begin(), updatedPath.end(), '\\', '/');

        if(!(path[0] == '/' && path[1] == '/'))
        {
            outPhysicalPath = path;
            return folder ? FileSystem ::FolderExists(updatedPath) : FileSystem::FileExists(updatedPath);
        }

        static std::string delimiter = "/";
        auto slash                   = updatedPath.find_first_of(delimiter.c_str(), 2);
        std::string_view virtualDir  = std::string_view(updatedPath);
        virtualDir                   = virtualDir.substr(2, slash - 2);

        auto it = m_MountPoints.find(virtualDir);
        if(it == m_MountPoints.end() || it->second.empty())
        {
            outPhysicalPath = updatedPath;
            return folder ? FileSystem::FolderExists(updatedPath) : FileSystem::FileExists(updatedPath);
        }

        const std::string remainder = updatedPath.substr(virtualDir.size() + 2, updatedPath.size() - virtualDir.size());
        for(const std::string& physicalPath : it->second)
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
        LUMOS_PROFILE_FUNCTION();
        std::string physicalPath;
        return ResolvePhysicalPath(path, physicalPath) ? FileSystem::ReadFile(physicalPath) : nullptr;
    }

    std::string VFS::ReadTextFile(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string physicalPath;
        return ResolvePhysicalPath(path, physicalPath) ? FileSystem::ReadTextFile(physicalPath) : "";
    }

    bool VFS::WriteFile(const std::string& path, uint8_t* buffer, uint32_t size)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string physicalPath;
        return ResolvePhysicalPath(path, physicalPath) ? FileSystem::WriteFile(physicalPath, buffer, size) : false;
    }

    bool VFS::WriteTextFile(const std::string& path, const std::string& text)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string physicalPath;
        return ResolvePhysicalPath(path, physicalPath) ? FileSystem::WriteTextFile(physicalPath, text) : false;
    }

    bool VFS::AbsoulePathToVFS(const std::string& path, std::string& outVFSPath, bool folder)
    {
        LUMOS_PROFILE_FUNCTION();
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
