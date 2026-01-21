#pragma once
#include "EditorPanel.h"
#include <imgui/Plugins/ImTextEditor.h>
#include <functional>

namespace Lumos
{
    class TextEditPanel : public EditorPanel
    {
    public:
        TextEditPanel(const std::string& filePath);
        ~TextEditPanel() = default;

        void OnImGui() override;
        void OnClose();

        void SetOnSaveCallback(const std::function<void()>& callback) { m_OnSaveCallback = callback; }
        void SetErrors(const std::unordered_map<int, std::string>& errors);

    private:
        std::string m_FilePath;
        TextEditor editor;
        std::function<void()> m_OnSaveCallback;

        bool m_TextUnsaved = false;
        bool m_FocusMode = false;
        bool m_PreviousFocusMode = false;
        uint32_t m_SavedDockID = 0;

        float m_SavedTimer = -1.0f;
    };
}
