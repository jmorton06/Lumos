#pragma once

#include "LM.h"

namespace lumos
{
	class FileSystem
	{
	public:
		static bool FileExists(const String& path);
		static int64 GetFileSize(const String& path);

		static byte* ReadFile(const String& path);
		static bool ReadFile(const String& path, void* buffer, int64 size = -1);
		static String ReadTextFile(const String& path);

		static bool WriteFile(const String& path, byte* buffer);
		static bool WriteTextFile(const String& path, const String& text);
	};

}
