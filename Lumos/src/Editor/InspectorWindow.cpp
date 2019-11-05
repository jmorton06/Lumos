#include "lmpch.h"
#include "InspectorWindow.h"
#include "Editor.h"
#include "ECS/EntityManager.h"
#include "Graphics/API/GraphicsContext.h"
#include "App/Application.h"
#include "App/SceneManager.h"

#include <imgui/imgui.h>
#include <entt/imgui_entt_entity_editor.hpp>
#include <IconFontCppHeaders/IconsFontAwesome5.h>

namespace Lumos
{
	InspectorWindow::InspectorWindow()
	{
		m_Name = ICON_FA_INFO_CIRCLE" Inspector###inspector";
		m_SimpleName = "Inspector";
	}

	void InspectorWindow::OnImGui()
	{
        auto& registry = Application::Instance()->GetSceneManager()->GetCurrentScene()->GetRegistry();
        //MM::ImGuiEntityEditor<decltype(registry)> editor;
        
		auto flags = ImGuiWindowFlags_NoCollapse;
		ImGui::Begin(m_Name.c_str(), &m_Active, flags);

//		if (m_Editor->GetSelected())
//		{
//			m_Editor->GetSelected()->OnImGui();
//		}

		ImGui::End();
	}
}
