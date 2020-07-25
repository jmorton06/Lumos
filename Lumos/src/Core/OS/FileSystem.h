#pragma once
#include "lmpch.h"

namespace Lumos
{
	class FileSystem
	{
	public:
		static bool FileExists(const std::string& path);
		static bool FolderExists(const std::string& path);
		static i64 GetFileSize(const std::string& path);

		static u8* ReadFile(const std::string& path);
		static bool ReadFile(const std::string& path, void* buffer, i64 size = -1);
		static std::string ReadTextFile(const std::string& path);

		static bool WriteFile(const std::string& path, u8* buffer);
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
		static i64 (*GetFileSizeFunc)(const std::string&);

		static u8* (*ReadFileFunc)(const std::string&);
		static bool (*ReadFileBufferFunc)(const std::string&, void*, i64);
		static std::string (*ReadTextFileFunc)(const std::string&);

		static bool (*WriteFileFunc)(const std::string&, u8*);
		static bool (*WriteTextFileFunc)(const std::string&, const std::string&);
	};

}
