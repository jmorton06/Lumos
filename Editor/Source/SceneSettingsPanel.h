#pragma once

#include "EditorPanel.h"

namespace Lumos
{
    class SceneSettingsPanel : public EditorPanel
    {
    public:
        SceneSettingsPanel();
        ~SceneSettingsPanel() = default;

        void OnNewScene(Scene* scene) override { m_CurrentScene = scene; }
        void OnImGui() override;

    private:
        Scene* m_CurrentScene = nullptr;
        bool m_NameUpdated    = false;
        std::string m_SceneName;
    };
}
