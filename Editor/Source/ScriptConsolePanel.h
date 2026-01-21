#pragma once
#include "EditorPanel.h"
#include <imgui/Plugins/ImTextEditor.h>
#include <string>
#include <vector>

namespace Lumos
{
    struct ScriptExample
    {
        const char* Name;
        const char* Description;
        const char* Code;
    };

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
        void DrawExamplesDropdown();

        TextEditor m_Editor;
        std::vector<std::string> m_OutputHistory;
        bool m_AutoScroll = true;
        Scene* m_CurrentScene = nullptr;
        int m_SelectedExample = 0;

        static const std::vector<ScriptExample> s_Examples;
    };
}
