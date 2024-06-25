#include "Precompiled.h"
#include "Core/OS/FileSystem.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

namespace Lumos
{
    static bool ReadFileInternal(FILE* file, void* buffer, int64_t size, bool readbytemode)
    {
        int64_t read_size;
        if(readbytemode)
            read_size = fread(buffer, sizeof(uint8_t), size, file);
        else
            read_size = fread(buffer, sizeof(char), size, file);

        if(size != read_size)
        {
            // delete[] buffer;
            // buffer = NULL;
            return false;
        }
        else
            return true;
    }

    bool FileSystem::FileExists(const std::string& path)
    {
        struct stat buffer;
        return (stat(path.c_str(), &buffer) == 0);
        // return access( path.c_str(), F_OK ) == 0
    }

    bool FileSystem::FolderExists(const std::string& path)
    {
        struct stat buffer;
        return (stat(path.c_str(), &buffer) == 0);
    }

    int64_t FileSystem::GetFileSize(const std::string& path)
    {
        if(!FileExists(path))
            return -1;
        struct stat buffer;
        stat(path.c_str(), &buffer);
        return buffer.st_size;
    }

    bool FileSystem::ReadFile(const std::string& path, void* buffer, int64_t size)
    {
        if(!FileExists(path))
            return false;
        if(size < 0)
            size = GetFileSize(path);
        buffer      = new uint8_t[size + 1];
        FILE* file  = fopen(path.c_str(), FileSystem::GetFileOpenModeString(FileOpenFlags::READ));
        bool result = false;
        if(file)
        {
            result = ReadFileInternal(file, buffer, size, true);
            fclose(file);
        }
        //        else
        //        {
        //            delete[] buffer;
        //            return false;
        //        }
        return result;
    }

    uint8_t* FileSystem::ReadFile(const std::string& path)
    {
        if(!FileExists(path))
            return nullptr;
        int64_t size    = GetFileSize(path);
        FILE* file      = fopen(path.c_str(), FileSystem::GetFileOpenModeString(FileOpenFlags::READ));
        uint8_t* buffer = new uint8_t[size];
        bool result     = ReadFileInternal(file, buffer, size, true);
        fclose(file);
        if(!result && buffer)
            delete[] buffer;
        return result ? buffer : nullptr;
    }

    std::string FileSystem::ReadTextFile(const std::string& path)
    {
        if(!FileExists(path))
            return std::string();
        int64_t size = GetFileSize(path);
        FILE* file   = fopen(path.c_str(), FileSystem::GetFileOpenModeString(FileOpenFlags::READ));
        std::string result(size, 0);
        bool success = ReadFileInternal(file, &result[0], size, false);
        fclose(file);
        if(success)
        {
            // Strip carriage returns
            result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
        }
        return success ? result : std::string();
    }

    bool FileSystem::WriteFile(const std::string& path, uint8_t* buffer, uint32_t size)
    {
        FILE* file = fopen(path.c_str(), FileSystem::GetFileOpenModeString(FileOpenFlags::WRITE));
        if(file == NULL) // if file does not exist, create it
        {
            file = fopen(path.c_str(), FileSystem::GetFileOpenModeString(FileOpenFlags::WRITE_READ));
        }

        if(file == nullptr)
        {
            switch(errno)
            {
            case ENOENT:
            {
                LUMOS_LOG_ERROR("File not found : {0}", path);
            }
            break;
            default:
            {
                LUMOS_LOG_ERROR("File can't open : {0}", path);
            }
            break;
            }
            return false;
        }

        size_t output = 0;
        if(buffer)
        {
            output = fwrite(buffer, 1, size, file);
        }
        fclose(file);
        return output > 0;
    }

    bool FileSystem::WriteTextFile(const std::string& path, const std::string& text)
    {
        FILE* file = fopen(path.c_str(), FileSystem::GetFileOpenModeString(FileOpenFlags::WRITE_READ));
        if(file == nullptr)
        {
            switch(errno)
            {
            case ENOENT:
            {
                LUMOS_LOG_ERROR("File not found : {0}", path);
            }
            break;
            default:
            {
                LUMOS_LOG_ERROR("File can't open : {0}", path);
            }
            break;
            }
            return false;
        }
        else
        {
            size_t size = fwrite(text.c_str(), 1, text.size(), file);
            fclose(file);
            return size == text.size();
        }
    }

    std::string FileSystem::GetWorkingDirectory()
    {
        const size_t pathSize = 4096;
        char currentDir[pathSize];
        if(getcwd(currentDir, pathSize) != NULL)
            LUMOS_LOG_INFO(currentDir);

        return std::string(currentDir);
    }
}
