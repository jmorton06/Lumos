#include "lmpch.h"
#include "FileSystem.h"

namespace Lumos
{
	bool (*FileSystem::FileExistsFunc)(const std::string&) = NULL;
	bool (*FileSystem::FolderExistsFunc)(const std::string&) = NULL;
	i64 (*FileSystem::GetFileSizeFunc)(const std::string&) = NULL;
	u8* (*FileSystem::ReadFileFunc)(const std::string&) = NULL;
	bool (*FileSystem::ReadFileBufferFunc)(const std::string&, void*, i64) = NULL;
	bool (*FileSystem::WriteFileFunc)(const std::string&, u8*) = NULL;
	bool (*FileSystem::WriteTextFileFunc)(const std::string&, const std::string&) = NULL;
}
