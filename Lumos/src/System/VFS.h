#pragma once

#include "LM.h"

namespace Lumos
{
	class LUMOS_EXPORT VFS
	{
	private:
		static VFS* s_Instance;
	private:
		std::unordered_map<String, std::vector<String>> m_MountPoints;
	public:
		void Mount(const String& virtualPath, const String& physicalPath);
		void Unmount(const String& path);
		bool ResolvePhysicalPath(const String& path, String& outPhysicalPath);

		byte* ReadFile(const String& path);
		String ReadTextFile(const String& path);

		bool WriteFile(const String& path, byte* buffer);
		bool WriteTextFile(const String& path, const String& text);
	public:
		static void Init();
		static void Shutdown();

		inline static VFS* Get() { return s_Instance; }
	};
}
