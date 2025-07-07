#include "Precompiled.h"
#include "Core/OS/FileSystem.h"
#include "WindowsUtilities.h"

#ifdef LUMOS_PLATFORM_WINDOWS
#include <Windows.h>
#include <wtypes.h>
#include <fstream>

namespace Lumos
{

    /*void CALLBACK FileIOCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
        {
        }*/

    static HANDLE OpenFileForReading(const String8& path)
    {
        return CreateFile(WindowsUtilities::StringToWString(ToStdString(path)).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
    }

    static int64_t GetFileSizeInternal(const HANDLE file)
    {
        LARGE_INTEGER size;
        GetFileSizeEx(file, &size);
        return size.QuadPart;
    }

    static bool ReadFileInternal(const HANDLE file, void* buffer, const int64_t size)
    {
        OVERLAPPED ol = { 0 };
        return ReadFileEx(file, buffer, static_cast<DWORD>(size), &ol, nullptr) != 0;
    }

    bool FileSystem::FileExists(const String8& path)
    {
        ArenaTemp scratch = ScratchBegin(0, 0);
        String16 path16 = Str16From8(scratch.arena, path);
        DWORD attributes = GetFileAttributesW((WCHAR*)path16.str);
        bool exists = (attributes != INVALID_FILE_ATTRIBUTES) && !!(~attributes & FILE_ATTRIBUTE_DIRECTORY);
        ScratchEnd(scratch);
        return exists;
    }

    bool FileSystem::FolderExists(const String8& path)
    {
        ArenaTemp scratch = ScratchBegin(0, 0);
        String16 path16 = Str16From8(scratch.arena, path);
        DWORD attributes = GetFileAttributesW((WCHAR*)path16.str);
        bool      exists = (attributes != INVALID_FILE_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_DIRECTORY);
        ScratchEnd(scratch);
        return exists;

        DWORD dwAttrib = GetFileAttributes(WindowsUtilities::StringToWString(ToStdString(path)).c_str());
        return dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    int64_t FileSystem::GetFileSize(const String8& path)
    {
        const HANDLE file = OpenFileForReading(path);
        if(file == INVALID_HANDLE_VALUE)
            return -1;
        int64_t result = GetFileSizeInternal(file);
        CloseHandle(file);

        return result;
    }

    bool FileSystem::ReadFile(Arena* arena, const String8& path, void* buffer, int64_t size)
    {
        std::ifstream stream((const char*)path.str, std::ios::binary | std::ios::ate);

        auto end = stream.tellg();
        stream.seekg(0, std::ios::beg);
        size   = end - stream.tellg();
        buffer = new char[size];
        stream.read((char*)buffer, size);
        stream.close();

        return buffer;
    }

    uint8_t* FileSystem::ReadFile(Arena* arena, const String8& path)
    {
        if(!FileExists(path))
            return nullptr;

        std::ifstream stream((const char*)path.str, std::ios::binary | std::ios::ate);

        auto end = stream.tellg();
        stream.seekg(0, std::ios::beg);
        const int64_t size = end - stream.tellg();
        char* buffer       = new char[size];
        stream.read((char*)buffer, size);
        stream.close();

        return (uint8_t*)buffer;
    }

    String8 FileSystem::ReadTextFile(Arena* arena, const String8& path)
    {
        if(!FileExists(path))
            return String8();

        std::ifstream stream((const char*)path.str, std::ios::in | std::ios::ate);
        if (!stream.is_open())
            return String8();

        std::streamsize size = stream.tellg();
        stream.seekg(0, std::ios::beg);

        String8 result = PushStr8FillByte(arena, size, 0);
        if (stream.read((char*)(result.str), size))
        {
            stream.close();
            return result;
        }

        stream.close();
        return String8();
    }

    bool FileSystem::WriteFile(const String8& path, uint8_t* buffer, uint32_t size)
    {
        std::ofstream stream((const char*)path.str, std::ios::binary | std::ios::trunc);

        if(!stream)
        {
            stream.close();
            return false;
        }

        stream.write((char*)buffer, size);
        stream.close();

        return true;
    }

    bool FileSystem::WriteTextFile(const String8& path, const String8& text)
    {
        return WriteFile(path, text.str, (uint32_t)text.size);
    }
}

#endif
