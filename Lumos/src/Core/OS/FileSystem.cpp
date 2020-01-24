#include "lmpch.h"
#include "FileSystem.h"

namespace Lumos
{
    bool (*FileSystem::FileExistsFunc)(const String&) = NULL;
    bool (*FileSystem::FolderExistsFunc)(const String&) = NULL;
	i64 (*FileSystem::GetFileSizeFunc)(const String&) = NULL;
	u8* (*FileSystem::ReadFileFunc)(const String&) = NULL;
	bool (*FileSystem::ReadFileFunc)(const String&, void*, i64) = NULL;
    bool (*FileSystem::WriteFileFunc)(const String&, u8*) = NULL;
	bool (*FileSystem::WriteTextFileFunc)(const String&, const String&) = NULL;
}