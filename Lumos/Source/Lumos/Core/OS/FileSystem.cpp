#include "Precompiled.h"
#include "FileSystem.h"
#include "Core/Application.h"
#include "Maths/MathsUtilities.h"
#include "Core/Thread.h"

#if __has_include(<filesystem>)
#include <filesystem>
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
#endif

namespace Lumos
{
    bool FileSystem::IsRelativePath(const char* path)
    {
        if(!path || path[0] == '/' || path[0] == '\\')
            return false;
		
        if(strlen(path) >= 2 && isalpha(path[0]) && path[1] == ':')
            return false;
		
        return true;
    }
	
    bool FileSystem::IsAbsolutePath(const char* path)
    {
        return path ? !IsRelativePath(path) : false;
    }
	
    const char* FileSystem::GetFileOpenModeString(FileOpenFlags flag)
    {
        switch(flag)
        {
            case FileOpenFlags::READ: return "rb";
            case FileOpenFlags::WRITE: return "wb";
            case FileOpenFlags::READ_WRITE: return "rb+";
            case FileOpenFlags::WRITE_READ: return "wb+";
            default:
			LWARN("Invalid open flag");
			return "rb";
        }
    }
	
    bool FileSystem::ResolvePhysicalPath(Arena* arena, const String8& path, String8* outPhysicalPath, bool folder)
    {
        LUMOS_PROFILE_FUNCTION();
		
        if(!(path.size >= 2 && path.str[0] == '/' && path.str[1] == '/'))
        {
            *outPhysicalPath = path;
            return folder ? FileSystem::FolderExists(*outPhysicalPath) : FileSystem::FileExists(*outPhysicalPath);
        }
		
#ifndef LUMOS_PRODUCTION
        String8 assetsStr = Str8Lit("Assets");
        String8 pathSub2 = Substr8(path, {2, Maths::Min(path.size, 8)});
        if(!Str8Match(pathSub2, assetsStr))
        {
            *outPhysicalPath = PushStr8F(Application::Get().GetFrameArena(), "%s%s", 
										 (char*)m_AssetsPath.str,
										 ToCChar(Substr8(path, {1, path.size})));
            NullTerminate((*outPhysicalPath));
            return folder ? FileSystem::FolderExists(*outPhysicalPath) : FileSystem::FileExists(*outPhysicalPath);
        }
        else
#endif
        {
            *outPhysicalPath = PushStr8F(Application::Get().GetFrameArena(), "%s%s",
										 (char*)m_AssetsPath.str,
										 ToCChar(Substr8(path, {8, path.size})));
            NullTerminate((*outPhysicalPath));
            return folder ? FileSystem::FolderExists(*outPhysicalPath) : FileSystem::FileExists(*outPhysicalPath);
        }
		
        return false;
    }
	
    uint8_t* FileSystem::ReadFileVFS(Arena* arena, const String8& path)
    {
        LUMOS_PROFILE_FUNCTION();
        String8 physicalPath;
        return ResolvePhysicalPath(arena, path, &physicalPath) ?
            FileSystem::ReadFile(arena, physicalPath) : nullptr;
    }
	
    String8 FileSystem::ReadTextFileVFS(Arena* arena, const String8& path)
    {
        LUMOS_PROFILE_FUNCTION();
        String8 physicalPath;
        if(ResolvePhysicalPath(arena, path, &physicalPath))
        {
			String8 text = FileSystem::ReadTextFile(arena, physicalPath);
            return text;
        }
        return Str8Lit("");
    }
	
    bool FileSystem::WriteFileVFS(const String8& path, uint8_t* buffer, uint32_t size)
    {
        LUMOS_PROFILE_FUNCTION();
        String8 physicalPath;
		ArenaTemp temp = ScratchBegin(nullptr, 0);
        bool success = Get().ResolvePhysicalPath(temp.arena, path, &physicalPath) ?
            FileSystem::WriteFile(physicalPath, buffer, size) : false;
		ScratchEnd(temp);

		return success;
    }
	
    bool FileSystem::WriteTextFileVFS(const String8& path, const String8& text)
    {
        LUMOS_PROFILE_FUNCTION();
        String8 physicalPath;
		ArenaTemp temp = ScratchBegin(nullptr, 0);

        bool success = Get().ResolvePhysicalPath(temp.arena, path,&physicalPath) ?
            FileSystem::WriteTextFile(physicalPath, text) : false;
		ScratchEnd(temp);

		return success;
    }
	
    bool FileSystem::AbsolutePathToFileSystem(Arena* arena, const String8& path, String8& outFileSystemPath, bool folder)
    {
        LUMOS_PROFILE_FUNCTION();
        for(uint64_t i = 0; i < path.size; i++)
            path.str[i] = CharToForwardSlash(path.str[i]);

		u64 foundPos = FindSubstr8(path, m_AssetsPath, 0);
        if(foundPos != path.size)
        {
            String8 newPartPath = Str8Lit("//Assets");
			outFileSystemPath = PushStr8F(arena, "%s%s", (const char*)newPartPath.str, (const char*)Substr8(path, { foundPos + m_AssetsPath.size, path.size }).str);

            return true;
        }
		
        outFileSystemPath = path;
        return false;
    }
	
    String8 FileSystem::AbsolutePathToFileSystem(Arena* arena, const String8& path, bool folder)
    {
        String8 outPath;
        AbsolutePathToFileSystem(arena, path, outPath, folder);
        return outPath;
    }
	
    void FileSystem::CreateFolderIfDoesntExist(const String8& path)
    {
        if(!FileSystem::FolderExists(path))
        {
#ifdef LUMOS_PLATFORM_IOS
            const char* home = std::getenv("HOME");
            if(!home)
                throw std::runtime_error("Can't obtain HOME");
			
            auto fullPath = std::filesystem::path(home) / "Documents/Lumos" / ToStdString(path);
            std::filesystem::create_directory(fullPath);
            LINFO("Creating folder %s", ToCChar(path));
#else
			std::filesystem::create_directory(ToStdString(path));
			LINFO("Creating folder %s", (const char*)path.str);
#endif
        }
    }
	
    void FileSystem::IterateFolder(const char* path, void (*f)(const char*))
    {
        auto folderPath = std::filesystem::path(std::string(path));

        if(std::filesystem::is_directory(folderPath))
        {
            for(auto& entry : std::filesystem::directory_iterator(folderPath))
                f(entry.path().string().c_str());
        }
    }
}
