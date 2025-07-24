#include "Precompiled.h"
#include "Core/OS/FileSystem.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

namespace Lumos
{
    static bool ReadFileInternal(FILE* file, void* buffer, int64_t size, bool readbytemode)
    {
        int64_t read_size;
        if(readbytemode)
            read_size = fread(buffer, sizeof(uint8_t), size, file);
        else
            read_size = fread(buffer, sizeof(char), size, file);
		
        return (size == read_size);
    }
	
    bool FileSystem::FileExists(const String8& path)
    {
        struct stat buffer;
        return (stat(ToCChar(path), &buffer) == 0);
    }
	
    bool FileSystem::FolderExists(const String8& path)
    {
        struct stat buffer;
        return (stat(ToCChar(path), &buffer) == 0);
    }
	
    int64_t FileSystem::GetFileSize(const String8& path)
    {
        if(!FileExists(path))
            return -1;
        struct stat buffer;
        stat(ToCChar(path), &buffer);
        return buffer.st_size;
    }
	
    bool FileSystem::ReadFile(Arena* arena, const String8& path, void* buffer, int64_t size)
    {
        if(!FileExists(path))
            return false;
        if(size < 0)
            size = GetFileSize(path);
		
        FILE* file  = fopen(ToCChar(path), FileSystem::GetFileOpenModeString(FileOpenFlags::READ));
        bool result = false;
        if(file)
        {
            result = ReadFileInternal(file, buffer, size, true);
            fclose(file);
        }
        return result;
    }
	
    uint8_t* FileSystem::ReadFile(Arena* arena, const String8& path)
    {
        if(!FileExists(path))
            return nullptr;
		
        int64_t size = GetFileSize(path);
        FILE* file   = fopen(ToCChar(path), FileSystem::GetFileOpenModeString(FileOpenFlags::READ));
        if(!file)
            return nullptr;
		
        u8* buffer = PushArrayNoZero(arena, u8, size);
        bool result     = ReadFileInternal(file, buffer, size, true);
        fclose(file);
		
        return result ? buffer : nullptr;
    }
	
    String8 FileSystem::ReadTextFile(Arena* arena, const String8& path)
    {
        if(!FileExists(path))
            return Str8Lit("");
		
        int64_t size = GetFileSize(path);
        FILE* file   = fopen(ToCChar(path), FileSystem::GetFileOpenModeString(FileOpenFlags::READ));
        if(!file)
            return Str8Lit("");
		
        // Read into string
        String8 result = PushStr8FillByte(arena, size, 0);
        bool success = ReadFileInternal(file, result.str, size, false);
        fclose(file);
		
        if(success)
        {
            // Strip carriage returns in-place
            uint64_t j = 0;
            for(uint64_t i = 0; i < result.size; i++)
            {
                if(result.str[i] != '\r')
                    result.str[j++] = result.str[i];
            }
            result.size = j;
			NullTerminate(result);
        }
		
        return success ? result : Str8Lit("");
    }
	
    bool FileSystem::WriteFile(const String8& path, uint8_t* buffer, uint32_t size)
    {
        FILE* file = fopen(ToCChar(path), FileSystem::GetFileOpenModeString(FileOpenFlags::WRITE));
        if(file == nullptr)
        {
            file = fopen(ToCChar(path), FileSystem::GetFileOpenModeString(FileOpenFlags::WRITE_READ));
        }
		
        if(file == nullptr)
        {
            switch(errno)
            {
				case ENOENT:
                LERROR("File not found : %s", ToCChar(path));
                break;
				default:
                LERROR("File can't open : %s", ToCChar(path));
                break;
            }
            return false;
        }
		
        size_t output = 0;
        if(buffer)
            output = fwrite(buffer, 1, size, file);
        fclose(file);
		
        return output > 0;
    }
	
    bool FileSystem::WriteTextFile(const String8& path, const String8& text)
    {
        FILE* file = fopen(ToCChar(path), FileSystem::GetFileOpenModeString(FileOpenFlags::WRITE_READ));
        if(file == nullptr)
        {
            switch(errno)
            {
				case ENOENT:
                LERROR("File not found : %s", ToCChar(path));
                break;
				default:
                LERROR("File can't open : %s", ToCChar(path));
                break;
            }
            return false;
        }
		
        size_t size = fwrite(text.str, 1, text.size, file);
        fclose(file);
		
        return size == text.size;
    }
	
    String8 FileSystem::GetWorkingDirectory(Arena* arena)
    {
		String8 Path = PushStr8FillByte(arena, 4096, 0);
        if(getcwd((char*)Path.str, Path.size) != NULL)
            LINFO((const char*)Path.str);

		return Path;
    }
}
