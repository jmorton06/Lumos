#pragma once
#include "Core/DataStructures/TDArray.h"
#include "Utilities/TSingleton.h"
#include "Core/String.h"

namespace Lumos
{
    enum class FileOpenFlags
    {
        READ,
        WRITE,
        READ_WRITE,
        WRITE_READ
    };

    class FileSystem : public ThreadSafeSingleton<FileSystem>
    {
        friend class ThreadSafeSingleton<FileSystem>;

    public:
        bool ResolvePhysicalPath(const std::string& path, std::string& outPhysicalPath, bool folder = false);
        bool AbsolutePathToFileSystem(const std::string& path, std::string& outFileSystemPath, bool folder = false);
        std::string AbsolutePathToFileSystem(const std::string& path, bool folder = false);

        uint8_t* ReadFileVFS(const std::string& path);
        std::string ReadTextFileVFS(const std::string& path);

        bool WriteFileVFS(const std::string& path, uint8_t* buffer, uint32_t size);
        bool WriteTextFileVFS(const std::string& path, const std::string& text);

        void SetAssetRoot(String8 root) { m_AssetRootPath = root; };

    private:
        String8 m_AssetRootPath;

    public:
        // Static Helpers. Implemented in OS specific Files
        static bool FileExists(const std::string& path);
        static bool FolderExists(const std::string& path);
        static void CreateFolderIfDoesntExist(const std::string& path);
        static int64_t GetFileSize(const std::string& path);

        static uint8_t* ReadFile(const std::string& path);
        static bool ReadFile(const std::string& path, void* buffer, int64_t size = -1);
        static std::string ReadTextFile(const std::string& path);

        static bool WriteFile(const std::string& path, uint8_t* buffer, uint32_t size);
        static bool WriteTextFile(const std::string& path, const std::string& text);

        static std::string GetWorkingDirectory();

        static bool IsRelativePath(const char* path);
        static bool IsAbsolutePath(const char* path);
        static const char* GetFileOpenModeString(FileOpenFlags flag);

        static void IterateFolder(const char* path, void (*f)(const char*));
    };
}
