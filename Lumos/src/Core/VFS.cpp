#include "lmpch.h"
#include "VFS.h"

#include "OS/FileSystem.h"


namespace Lumos 
{

	VFS* VFS::s_Instance = nullptr;

	void VFS::OnInit()
	{
		s_Instance = lmnew VFS();
	}

	void VFS::OnShutdown()
	{
		delete s_Instance;
	}

	void VFS::Mount(const String& virtualPath, const String& physicalPath)
	{
		LUMOS_ASSERT(s_Instance,"");
		m_MountPoints[virtualPath].push_back(physicalPath);
	}

	void VFS::Unmount(const String& path)
	{
		LUMOS_ASSERT(s_Instance,"");
		m_MountPoints[path].clear();
	}

	bool VFS::ResolvePhysicalPath(const String& path, String& outPhysicalPath, bool folder)
	{
		if (path[0] != '/')
		{
			outPhysicalPath = path;
			return folder ? FileSystem ::FolderExists(path) : FileSystem::FileExists(path);
		}

		std::vector<String> dirs = SplitString(path, '/');
		const String& virtualDir = dirs.front();

		if (m_MountPoints.find(virtualDir) == m_MountPoints.end() || m_MountPoints[virtualDir].empty())
        {
            outPhysicalPath = path;
            return folder ? FileSystem::FolderExists(path) : FileSystem::FileExists(path);
        }

		const String remainder = path.substr(virtualDir.size() + 1, path.size() - virtualDir.size());
		for (const String& physicalPath : m_MountPoints[virtualDir])
		{
			const String newPath = physicalPath + remainder;
			if (folder ? FileSystem::FolderExists(newPath) : FileSystem::FileExists(newPath))
			{
				outPhysicalPath = newPath;
				return true;
			}
		}
		return false;
	}

	u8* VFS::ReadFile(const String& path)
	{
		LUMOS_ASSERT(s_Instance,"");
		String physicalPath;
		return ResolvePhysicalPath(path, physicalPath) ? FileSystem::ReadFile(physicalPath) : nullptr;
	}

	String VFS::ReadTextFile(const String& path)
	{
		LUMOS_ASSERT(s_Instance,"");
		String physicalPath;
		return ResolvePhysicalPath(path, physicalPath) ? FileSystem::ReadTextFile(physicalPath) : nullptr;
	}

	bool VFS::WriteFile(const String& path, u8* buffer)
	{
		LUMOS_ASSERT(s_Instance,"");
		String physicalPath;
		return ResolvePhysicalPath(path, physicalPath) ? FileSystem::WriteFile(physicalPath, buffer) : false;

	}

	bool VFS::WriteTextFile(const String& path, const String& text)
	{
		LUMOS_ASSERT(s_Instance,"");
		String physicalPath;
		return ResolvePhysicalPath(path, physicalPath) ? FileSystem::WriteTextFile(physicalPath, text) : false;
	}

}
