#pragma once
#include "EditorPanel.h"
#include <functional>

#if __has_include(<filesystem>)
#include <filesystem>
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
#endif

namespace ImGui
{
    class FileBrowser;
}

namespace Lumos
{
    class FileBrowserPanel : public EditorPanel
    {
    public:
        FileBrowserPanel();
        ~FileBrowserPanel();

        void Open();
        void OnImGui() override;
        void SetCurrentPath(const std::string& path);
        void SetOpenDirectory(bool value);
        void SetCallback(const std::function<void(const std::string&)>& callback)
        {
            m_Callback = callback;
        }

        bool IsOpen();
        void SetFileTypeFilters(const std::vector<const char*>& fileFilters);
        void ClearFileTypeFilters();
        std::filesystem::path& GetPath();

    private:
        std::function<void(const std::string&)> m_Callback;
        ImGui::FileBrowser* m_FileBrowser;

#if defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_IOS)
        bool m_SelectDirectory = false;
        std::string m_CurrentPath;
        std::vector<std::string> m_FileFilters;
        std::filesystem::path m_Path;
#endif
    };
}
