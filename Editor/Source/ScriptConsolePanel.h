#pragma once
#include "EditorPanel.h"
#include <imgui/Plugins/ImTextEditor.h>
#include <string>
#include <vector>

namespace Lumos
{
    class ScriptConsolePanel : public EditorPanel
    {
    public:
        ScriptConsolePanel();
        ~ScriptConsolePanel() = default;

        void OnImGui() override;
        void OnNewScene(Scene* scene) override;

    private:
        void ExecuteScript();
        void ClearOutput();

        TextEditor m_Editor;
        std::vector<std::string> m_OutputHistory;
        bool m_AutoScroll = true;
        Scene* m_CurrentScene = nullptr;
    };
}
