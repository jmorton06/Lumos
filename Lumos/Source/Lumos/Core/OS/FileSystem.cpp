#include "Precompiled.h"
#include "FileSystem.h"

#if __has_include(<filesystem>)
#include <filesystem>
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
#endif

namespace Lumos
{
    bool FileSystem::IsRelativePath(const char* path)
    {
        if(!path || path[0] == '/' || path[0] == '\\')
        {
            return false;
        }

        if(strlen(path) >= 2 && isalpha(path[0]) && path[1] == ':')
        {
            return false;
        }

        return true;
    }

    bool FileSystem::IsAbsolutePath(const char* path)
    {
        if(!path)
        {
            return false;
        }

        return !IsRelativePath(path);
    }

    const char* FileSystem::GetFileOpenModeString(FileOpenFlags flag)
    {
        if(flag == FileOpenFlags::READ)
        {
            return "rb";
        }
        else if(flag == FileOpenFlags::WRITE)
        {
            return "wb";
        }
        else if(flag == FileOpenFlags::READ_WRITE)
        {
            return "rb+";
        }
        else if(flag == FileOpenFlags::WRITE_READ)
        {
            return "wb+";
        }
        else
        {
            LWARN("Invalid open flag");
            return "rb";
        }
    }

    bool FileSystem::ResolvePhysicalPath(const std::string& path, std::string& outPhysicalPath, bool folder)
    {
        LUMOS_PROFILE_FUNCTION();
        const std::string& updatedPath = path;

        if(!(path[0] == '/' && path[1] == '/'))
        {
            outPhysicalPath = path;
            return folder ? FileSystem::FolderExists(outPhysicalPath) : FileSystem::FileExists(outPhysicalPath);
        }

        // Assume path starts with //Assets
#ifndef LUMOS_PRODUCTION
        if(path.substr(2, 6) != "Assets")
        {
            // Previously paths saved in scenes could be like //Textures and then converted to .../Assets/Textures/...
            outPhysicalPath = ToStdString(m_AssetRootPath) + path.substr(1, path.size());
            return folder ? FileSystem::FolderExists(outPhysicalPath) : FileSystem::FileExists(outPhysicalPath);
        }
        else
#endif
        {
            outPhysicalPath = ToStdString(m_AssetRootPath) + path.substr(8, path.size());
            return folder ? FileSystem::FolderExists(outPhysicalPath) : FileSystem::FileExists(outPhysicalPath);
        }

        return false;
    }

    uint8_t* FileSystem::ReadFileVFS(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string physicalPath;
        return Get().ResolvePhysicalPath(path, physicalPath) ? FileSystem::ReadFile(physicalPath) : nullptr;
    }

    std::string FileSystem::ReadTextFileVFS(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string physicalPath;
        return Get().ResolvePhysicalPath(path, physicalPath) ? FileSystem::ReadTextFile(physicalPath) : "";
    }

    bool FileSystem::WriteFileVFS(const std::string& path, uint8_t* buffer, uint32_t size)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string physicalPath;
        return Get().ResolvePhysicalPath(path, physicalPath) ? FileSystem::WriteFile(physicalPath, buffer, size) : false;
    }

    bool FileSystem::WriteTextFileVFS(const std::string& path, const std::string& text)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string physicalPath;
        return Get().ResolvePhysicalPath(path, physicalPath) ? FileSystem::WriteTextFile(physicalPath, text) : false;
    }

    bool FileSystem::AbsolutePathToFileSystem(const std::string& path, std::string& outFileSystemPath, bool folder)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string updatedPath = path;
        std::replace(updatedPath.begin(), updatedPath.end(), '\\', '/');

        if(updatedPath.find(ToStdString(m_AssetRootPath)) != std::string::npos)
        {
            std::string newPath     = updatedPath;
            std::string newPartPath = "//Assets";
            newPath.replace(0, m_AssetRootPath.size, newPartPath);
            outFileSystemPath = newPath;
            return true;
        }

        outFileSystemPath = updatedPath;
        return false;
    }

    std::string FileSystem::AbsolutePathToFileSystem(const std::string& path, bool folder)
    {
        std::string outPath;
        AbsolutePathToFileSystem(path, outPath, folder);
        return outPath;
    }

    void FileSystem::CreateFolderIfDoesntExist(const std::string& path)
    {
        if(!FileSystem::FolderExists(path))
        {
            const char* home = std::getenv("HOME");
            if(!home)
            {
                throw std::runtime_error("Can't obtain HOME");
            }

            std::filesystem::create_directory(std::filesystem::path(home) / "Documents/Lumos" / path);
            LINFO("Creating folder &s", path.c_str());
        }
    }

    void FileSystem::IterateFolder(const char* path, void (*f)(const char*))
    {
        auto folderPath = std::filesystem::path(path);

        if(std::filesystem::is_directory(folderPath))
        {
            for(auto entry : std::filesystem::directory_iterator(folderPath))
            {
                f(entry.path().string().c_str());
            }
        }
    }
}
