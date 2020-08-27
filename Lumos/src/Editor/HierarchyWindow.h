#pragma once

#include "EditorWindow.h"

#include <entt/entity/fwd.hpp>
#include <imgui/imgui.h>

namespace Lumos
{
	class HierarchyWindow : public EditorWindow
	{
	public:
		HierarchyWindow();
		~HierarchyWindow() = default;

        void DrawNode(entt::entity node, entt::registry& registry);
		void OnImGui() override;

		void DestroyEntity(entt::entity entity, entt::registry& registry);
		bool IsParentOfEntity(entt::entity entity, entt::entity child, entt::registry& registry);
		
	private:
		ImGuiTextFilter m_HierarchyFilter;
		entt::entity m_DoubleClicked;
		entt::entity m_HadRecentDroppedEntity;
	};
}
