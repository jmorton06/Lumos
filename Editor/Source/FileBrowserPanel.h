#pragma once
#include "EditorPanel.h"
#include <functional>

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

    private:
        std::function<void(const std::string&)> m_Callback;
        ImGui::FileBrowser* m_FileBrowser;
    };
}
