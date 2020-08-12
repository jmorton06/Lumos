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
		void SetDebugMode(bool mode);
		bool GetIsDebugMode() const { return m_DebugMode; };

	private:
		MM::ImGuiEntityEditor<entt::entity> m_EnttEditor;
		bool m_DebugMode = false;
	};
}
