#pragma once

namespace Lumos
{
    class FileSystem
    {
    public:
        static bool FileExists(const std::string& path);
        static bool FolderExists(const std::string& path);
        static int64_t GetFileSize(const std::string& path);

        static uint8_t* ReadFile(const std::string& path);
        static bool ReadFile(const std::string& path, void* buffer, int64_t size = -1);
        static std::string ReadTextFile(const std::string& path);

        static bool WriteFile(const std::string& path, uint8_t* buffer);
        static bool WriteTextFile(const std::string& path, const std::string& text);

        static bool IsRelativePath(const char* path)
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

        static bool IsAbsolutePath(const char* path)
        {
            if(!path)
            {
                return false;
            }

            return !IsRelativePath(path);
        }

    private:
        static bool (*FileExistsFunc)(const std::string&);
        static bool (*FolderExistsFunc)(const std::string&);
        static int64_t (*GetFileSizeFunc)(const std::string&);

        static uint8_t* (*ReadFileFunc)(const std::string&);
        static bool (*ReadFileBufferFunc)(const std::string&, void*, int64_t);
        static std::string (*ReadTextFileFunc)(const std::string&);

        static bool (*WriteFileFunc)(const std::string&, uint8_t*);
        static bool (*WriteTextFileFunc)(const std::string&, const std::string&);
    };

}
