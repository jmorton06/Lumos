#pragma once
#include "EditorPanel.h"
#include <imgui/plugins/ImTextEditor.h>

namespace Lumos
{
    class TextEditPanel : public EditorPanel
    {
    public:
        TextEditPanel(const std::string& filePath);
        ~TextEditPanel() = default;

        void OnImGui() override;
        void OnClose();

    private:
        std::string m_FilePath;
        TextEditor editor;
    };
}
