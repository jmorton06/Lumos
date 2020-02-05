#pragma once
#include "lmpch.h"

namespace Lumos
{
	class FileSystem
	{
	public:
		static bool FileExists(const String& path);
		static bool FolderExists(const String& path);
		static i64 GetFileSize(const String& path);

		static u8* ReadFile(const String& path);
		static bool ReadFile(const String& path, void* buffer, i64 size = -1);
		static String ReadTextFile(const String& path);

		static bool WriteFile(const String& path, u8* buffer);
		static bool WriteTextFile(const String& path, const String& text);
        
        static bool IsRelativePath(const char *path)
        {
            if (!path || path[0] == '/' || path[0] == '\\')
            {
                return false;
            }
            
            if (strlen(path) >= 2 && isalpha(path[0]) && path[1] == ':')
            {
                return false;
            }
            
            return true;
        }

        static bool IsAbsolutePath(const char *path)
        {
            if (!path)
            {
                return false;
            }
            
            return !IsRelativePath(path);
        }

	private:
		static bool (*FileExistsFunc)(const String&);
		static bool (*FolderExistsFunc)(const String&);
		static i64 (*GetFileSizeFunc)(const String&);
		
		static u8* (*ReadFileFunc)(const String&);
		static bool (*ReadFileBufferFunc)(const String&, void*, i64);
		static String (*ReadTextFileFunc)(const String&);

		static bool (*WriteFileFunc)(const String&, u8*);
		static bool (*WriteTextFileFunc)(const String&, const String&);
	};

}
