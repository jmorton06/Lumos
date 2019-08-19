#include "LM.h"
#include "Core/FileSystem.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace Lumos
{
    static bool ReadFileInternal(FILE* file, void* buffer, i64 size, bool readbytemode)
    {
        i64 read_size;
        if(readbytemode)
            read_size = fread(buffer, sizeof(u8), size, file);
        else
            read_size = fread(buffer, sizeof(char), size, file);

        if(size != read_size)
        {
            //delete[] buffer;
            //buffer = NULL;
            return false;
        }
        else
            return true;
    }

    bool FileSystem::FileExists(const String& path)
    {
        struct stat buffer;
        return (stat (path.c_str(), &buffer) == 0);
    }

    i64 FileSystem::GetFileSize(const String& path)
    {
        if (!FileExists(path))
            return -1;
        struct stat buffer;
        stat(path.c_str(), &buffer);
        return buffer.st_size;
    }

    bool FileSystem::ReadFile(const String& path, void* buffer, i64 size)
    {
        if(!FileExists(path))
            return false;
        if(size < 0)
            size = GetFileSize(path);
        buffer = new u8[size + 1];
        FILE* file = fopen(path.c_str(), "r");
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

    u8* FileSystem::ReadFile(const String& path)
    {
        if(!FileExists(path))
            return nullptr;
        i64 size = GetFileSize(path);
        FILE* file = fopen(path.c_str(), "rb");
        u8* buffer = new u8[size];
        bool result = ReadFileInternal(file, buffer, size, true);
        fclose(file);
        if (!result && buffer)
            delete[] buffer;
        return result ? buffer : nullptr;
    }

    String FileSystem::ReadTextFile(const String& path)
    {
        if(!FileExists(path))
            return String();
        i64 size = GetFileSize(path);
        FILE* file = fopen(path.c_str(), "r");
        String result(size, 0);
        bool success = ReadFileInternal(file, &result[0], size, false);
        fclose(file);
        if (success)
        {
            // Strip carriage returns
            result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
        }
        return success ? result : String();
    }

    bool FileSystem::WriteFile(const String& path, u8* buffer)
    {
        FILE* file = fopen(path.c_str(), "wb");
        size_t size = fwrite(buffer, 1, sizeof(buffer), file);
        fclose(file);
        return size > 0;
    }

    bool FileSystem::WriteTextFile(const String& path, const String& text)
    {
        FILE* file = fopen(path.c_str(), "w");
        size_t size = fwrite(text.c_str(), 1, sizeof(text), file);
        fclose(file);
        return size > 0;
    }
}
