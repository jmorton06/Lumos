#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "Core/OS/FileSystem.h"
#include "WindowsUtilities.h"

#ifdef LUMOS_PLATFORM_WINDOWS
#include <Windows.h>
#include <wtypes.h>

namespace Lumos
{

    /*void CALLBACK FileIOCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
        {
        }*/

    static HANDLE OpenFileForReading(const std::string& path)
    {
        return CreateFile(WindowsUtilities::StringToWString(path).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
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

    bool FileSystem::FileExists(const std::string& path)
    {
        DWORD dwAttrib = GetFileAttributes(WindowsUtilities::StringToWString(path).c_str());
        return (dwAttrib != INVALID_FILE_ATTRIBUTES) && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0;
    }

    bool FileSystem::FolderExists(const std::string& path)
    {
        DWORD dwAttrib = GetFileAttributes(WindowsUtilities::StringToWString(path).c_str());
        return dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    int64_t FileSystem::GetFileSize(const std::string& path)
    {
        const HANDLE file = OpenFileForReading(path);
        if(file == INVALID_HANDLE_VALUE)
            return -1;
        int64_t result = GetFileSizeInternal(file);
        CloseHandle(file);

        return result;
    }

    bool FileSystem::ReadFile(const std::string& path, void* buffer, int64_t size)
    {
        std::ifstream stream(path, std::ios::binary | std::ios::ate);

        auto end = stream.tellg();
        stream.seekg(0, std::ios::beg);
        size   = end - stream.tellg();
        buffer = new char[size];
        stream.read((char*)buffer, size);
        stream.close();

        return buffer;
    }

    uint8_t* FileSystem::ReadFile(const std::string& path)
    {
        if(!FileExists(path))
            return nullptr;

        std::ifstream stream(path, std::ios::binary | std::ios::ate);

        auto end = stream.tellg();
        stream.seekg(0, std::ios::beg);
        const int64_t size = end - stream.tellg();
        char* buffer       = new char[size];
        stream.read((char*)buffer, size);
        stream.close();

        return (uint8_t*)buffer;
    }

    std::string FileSystem::ReadTextFile(const std::string& path)
    {
        if(!FileExists(path))
            return std::string();

        std::ifstream stream(path);

        std::string fileContent;
        std::string line;
        while(std::getline(stream, line))
        {                               // Read file line by line
            fileContent += line + "\n"; // Append each line to fileContent
        }

        stream.close();

        return fileContent;
    }

    bool FileSystem::WriteFile(const std::string& path, uint8_t* buffer, uint32_t size)
    {
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);

        if(!stream)
        {
            stream.close();
            return false;
        }

        stream.write((char*)buffer, size);
        stream.close();

        return true;
    }

    bool FileSystem::WriteTextFile(const std::string& path, const std::string& text)
    {
        return WriteFile(path, (uint8_t*)&text[0], (uint32_t)text.size());
    }
}

#endif
