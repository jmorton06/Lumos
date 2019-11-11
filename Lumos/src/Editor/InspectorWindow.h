#pragma once
#include "lmpch.h"
#include "EditorWindow.h"
#include <entt/imgui_entt_entity_editor.hpp>

namespace Lumos
{
	class InspectorWindow : public EditorWindow
	{
	public:
		InspectorWindow();
		~InspectorWindow() = default;

		void OnNewScene(Scene* scene) override;
		void OnImGui() override;

	private:
		MM::ImGuiEntityEditor<entt::registry> m_EnttEditor;
	};
}