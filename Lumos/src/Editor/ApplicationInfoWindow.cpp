#include "lmpch.h"
#include "ApplicationInfoWindow.h"
#include "Graphics/API/GraphicsContext.h"
#include "HierarchyWindow.h"
#include "Editor.h"
#include "Core/Application.h"
#include "Scene/SceneManager.h"
#include "Core/Engine.h"
#include "Graphics/Layers/LayerStack.h"
#include "Graphics/RenderManager.h"
#include "Graphics/GBuffer.h"
#include "ImGui/ImGuiHelpers.h"
#include <imgui/imgui.h>

namespace Lumos
{
	ApplicationInfoWindow::ApplicationInfoWindow()
	{
		m_Name = "ApplicationInfo";
		m_SimpleName = "ApplicationInfo";
	}

	void ApplicationInfoWindow::OnImGui()
	{
		auto flags = ImGuiWindowFlags_NoCollapse;
		ImGui::Begin(m_Name.c_str(), &m_Active, flags);
		{
			if(ImGui::TreeNode("Application"))
			{
				auto systems = Application::Get().GetSystemManager();

				if(ImGui::TreeNode("Systems"))
				{
					systems->OnImGui();
					ImGui::TreePop();
				}

				auto layerStack = Application::Get().GetSceneLayers();
				if(ImGui::TreeNode("Layers"))
				{
					layerStack->OnImGui();
					ImGui::TreePop();
				}

				ImGui::NewLine();
				ImGui::Text("FPS : %5.2i", Engine::Get().GetFPS());
				ImGui::Text("UPS : %5.2i", Engine::Get().GetUPS());
				ImGui::Text("Frame Time : %5.2f ms", Engine::Get().GetFrametime());
				ImGui::NewLine();
				ImGui::Text("Scene : %s", Application::Get().GetSceneManager()->GetCurrentScene()->GetSceneName().c_str());

				if(ImGui::TreeNode("GBuffer"))
				{
					if(ImGui::TreeNode("Colour Texture"))
					{
						ImGuiHelpers::Image(Application::Get().GetRenderManager()->GetGBuffer()->GetTexture(Graphics::SCREENTEX_COLOUR), Maths::Vector2(128.0f, 128.0f));
						ImGuiHelpers::Tooltip(Application::Get().GetRenderManager()->GetGBuffer()->GetTexture(Graphics::SCREENTEX_COLOUR), Maths::Vector2(256.0f, 256.0f));

						ImGui::TreePop();
					}
					if(ImGui::TreeNode("Normal Texture"))
					{
						ImGuiHelpers::Image(Application::Get().GetRenderManager()->GetGBuffer()->GetTexture(Graphics::SCREENTEX_NORMALS), Maths::Vector2(128.0f, 128.0f));
						ImGuiHelpers::Tooltip(Application::Get().GetRenderManager()->GetGBuffer()->GetTexture(Graphics::SCREENTEX_NORMALS), Maths::Vector2(256.0f, 256.0f));

						ImGui::TreePop();
					}
					if(ImGui::TreeNode("PBR Texture"))
					{
						ImGuiHelpers::Image(Application::Get().GetRenderManager()->GetGBuffer()->GetTexture(Graphics::SCREENTEX_PBR), Maths::Vector2(128.0f, 128.0f));
						ImGuiHelpers::Tooltip(Application::Get().GetRenderManager()->GetGBuffer()->GetTexture(Graphics::SCREENTEX_PBR), Maths::Vector2(256.0f, 256.0f));

						ImGui::TreePop();
					}
					if(ImGui::TreeNode("Position Texture"))
					{
						ImGuiHelpers::Image(Application::Get().GetRenderManager()->GetGBuffer()->GetTexture(Graphics::SCREENTEX_POSITION), Maths::Vector2(128.0f, 128.0f));
						ImGuiHelpers::Tooltip(Application::Get().GetRenderManager()->GetGBuffer()->GetTexture(Graphics::SCREENTEX_POSITION), Maths::Vector2(256.0f, 256.0f));

						ImGui::TreePop();
					}
					ImGui::TreePop();
				}
				ImGui::TreePop();
			};
		}
		ImGui::End();
	}
}
