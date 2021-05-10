#include "Precompiled.h"
#include "FileSystem.h"

namespace Lumos
{
    bool (*FileSystem::FileExistsFunc)(const std::string&) = NULL;
    bool (*FileSystem::FolderExistsFunc)(const std::string&) = NULL;
    int64_t (*FileSystem::GetFileSizeFunc)(const std::string&) = NULL;
    uint8_t* (*FileSystem::ReadFileFunc)(const std::string&) = NULL;
    bool (*FileSystem::ReadFileBufferFunc)(const std::string&, void*, int64_t) = NULL;
    bool (*FileSystem::WriteFileFunc)(const std::string&, uint8_t*) = NULL;
    bool (*FileSystem::WriteTextFileFunc)(const std::string&, const std::string&) = NULL;
}
