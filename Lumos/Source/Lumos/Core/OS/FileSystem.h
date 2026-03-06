#pragma once
#include "Core/DataStructures/TDArray.h"
#include "Utilities/TSingleton.h"
#include "Core/String.h"

namespace Lumos
{
    class PackedAssetReader;

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
        bool ResolvePhysicalPath(Arena* arena, const String8& path, String8* outPhysicalPath, bool folder = false);
        String8 ResolvePhysicalPath(Arena* arena, const String8& path, bool folder = false);
        bool AbsolutePathToFileSystem(Arena* arena, const String8& path, String8& outFileSystemPath, bool folder = false);
        String8 AbsolutePathToFileSystem(Arena* arena, const String8& path, bool folder = false);

        uint8_t* ReadFileVFS(Arena* arena, const String8& path);
        String8 ReadTextFileVFS(Arena* arena, const String8& path);

        bool WriteFileVFS(const String8& path, uint8_t* buffer, uint32_t size);
        bool WriteTextFileVFS(const String8& path, const String8& text);

        bool FileExistsVFS(const String8& path);
        bool FolderExistsVFS(const String8& path);
        int64_t GetFileSizeVFS(const String8& path);

        void SetAssetPath(const String8& Path)
        {
            m_AssetsPath = Path;
        }

        const String8& GetAssetPath() const { return m_AssetsPath; }

        bool MountPack(const String8& packPath);
        void UnmountPack();
        bool HasMountedPack() const { return m_PackReader != nullptr; }

        // Static Helpers. Implemented in OS specific Files
        static bool FileExists(const String8& path);
        static bool FolderExists(const String8& path);
        static void CreateFolderIfDoesntExist(const String8& path);
        static int64_t GetFileSize(const String8& path);

        static uint8_t* ReadFile(Arena* arena, const String8& path);
        static bool ReadFile(Arena* arena, const String8& path, void* buffer, int64_t size = -1);
        static String8 ReadTextFile(Arena* arena, const String8& path);

        static bool WriteFile(const String8& path, uint8_t* buffer, uint32_t size);
        static bool WriteTextFile(const String8& path, const String8& text);

        static String8 GetWorkingDirectory(Arena* arena);

        // Resolves "../" and "./" segments and normalises slashes
        static String8 NormalizePath(Arena* arena, const String8& path);

        static bool IsRelativePath(const char* path);
        static bool IsAbsolutePath(const char* path);
        static const char* GetFileOpenModeString(FileOpenFlags flag);

        static void IterateFolder(const char* path, void (*f)(const char*));
        static void IterateFolder(const String8& path, void (*f)(const char*));

        // Returns file modification time as Unix timestamp (seconds since epoch)
        // Returns 0 if file doesn't exist or on error
        static uint64_t GetFileModifiedTime(const String8& path);

    private:
        String8 m_AssetsPath;
        PackedAssetReader* m_PackReader = nullptr;
    };
}
