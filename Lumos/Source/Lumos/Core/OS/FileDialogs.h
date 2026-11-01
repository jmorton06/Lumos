#pragma once
#include <string>
#include "Core/DataStructures/TDArray.h"

namespace Lumos
{
    class FileDialogs
    {
    public:
        static std::string OpenFile(const std::string& filter = "", const std::string& defaultPath = "");

        // Returns selected save path, or empty string if cancelled
        static std::string SaveFile(const std::string& filter = "", const std::string& defaultPath = "", const std::string& defaultName = "");

        // Returns selected folder path, or empty string if cancelled
        static std::string PickFolder(const std::string& defaultPath = "");

#ifdef LUMOS_PLATFORM_IOS
        // Files app picker, several at once; each is copied into destDir (renamed if taken). Returns the new paths.
        static TDArray<std::string> ImportFiles(const std::string& filter, const std::string& destDir);
#endif
    };
}
