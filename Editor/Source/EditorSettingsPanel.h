#pragma once

#include "EditorPanel.h"

namespace Lumos
{
    class EditorSettingsPanel : public EditorPanel
    {
    public:
        EditorSettingsPanel();
        ~EditorSettingsPanel() = default;

        void OnNewScene(Scene* scene) override { m_CurrentScene = scene; }
        void OnImGui() override;

    private:
        Scene* m_CurrentScene = nullptr;
    };
}