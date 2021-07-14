#pragma once

#include "EditorPanel.h"
#include <Lumos/ImGui/ImGuiEnttEditor.hpp>

namespace Lumos
{
    class InspectorPanel : public EditorPanel
    {
    public:
        InspectorPanel();
        ~InspectorPanel() = default;

        void OnNewScene(Scene* scene) override;
        void OnImGui() override;
        void SetDebugMode(bool mode);
        bool GetIsDebugMode() const { return m_DebugMode; };

    private:
        MM::ImGuiEntityEditor<entt::entity> m_EnttEditor;
        bool m_DebugMode = false;
    };
}
