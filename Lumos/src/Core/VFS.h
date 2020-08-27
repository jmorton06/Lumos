#pragma once

#include "Core/Types.h"

namespace Lumos
{
	class LUMOS_EXPORT VFS
	{
	private:
		static VFS* s_Instance;

	private:
		std::unordered_map<std::string, std::vector<std::string>> m_MountPoints;

	public:
		void Mount(const std::string& virtualPath, const std::string& physicalPath);
		void Unmount(const std::string& path);
		bool ResolvePhysicalPath(const std::string& path, std::string& outPhysicalPath, bool folder = false);

		u8* ReadFile(const std::string& path);
		std::string ReadTextFile(const std::string& path);

		bool WriteFile(const std::string& path, u8* buffer);
		bool WriteTextFile(const std::string& path, const std::string& text);

	public:
		static void OnInit();
		static void OnShutdown();

		_FORCE_INLINE_ static VFS* Get()
		{
			return s_Instance;
		}
	};
}
