#pragma once

#include "EditorPanel.h"

namespace Lumos
{
    class ProjectSettingsPanel : public EditorPanel
    {
    public:
        ProjectSettingsPanel();
        ~ProjectSettingsPanel() = default;

        void OnNewScene(Scene* scene) override { m_CurrentScene = scene; }
        void OnImGui() override;

    private:
        Scene* m_CurrentScene = nullptr;
    };
}
