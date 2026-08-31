#pragma once
#include <string>

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
    };
}
