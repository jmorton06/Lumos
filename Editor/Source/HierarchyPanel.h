#pragma once

#include "EditorPanel.h"
#include "Core/OS/Memory.h"
#include <entt/entity/fwd.hpp>
#include <imgui/imgui.h>

namespace Lumos
{
    class HierarchyPanel : public EditorPanel
    {
    public:
        HierarchyPanel();
        ~HierarchyPanel();

        void DrawNode(entt::entity node, entt::registry& registry);
        void OnImGui() override;
        bool IsParentOfEntity(entt::entity entity, entt::entity child, entt::registry& registry);

    private:
        ImGuiTextFilter m_HierarchyFilter;
        entt::entity m_DoubleClicked;
        entt::entity m_HadRecentDroppedEntity;
        entt::entity m_CurrentPrevious;
        bool m_SelectUp;
        bool m_SelectDown;

        Arena* m_StringArena;
    };
}
