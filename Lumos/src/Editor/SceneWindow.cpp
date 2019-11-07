#include "lmpch.h"
#include "SceneWindow.h"
#include "Editor.h"
#include "Graphics/Camera/Camera.h"
#include "App/Application.h"
#include "App/SceneManager.h"
#include "App/Engine.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/RenderManager.h"
#include "Graphics/GBuffer.h"
#include "Graphics/Light.h"
#include "ECS/Component/CameraComponent.h"
#include "ECS/Component/SoundComponent.h"

#include <imgui/imgui_internal.h>
#include <imgui/plugins/ImGuizmo.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>

namespace Lumos
{
	SceneWindow::SceneWindow()
	{
		m_Name = ICON_FA_GAMEPAD" Scene###scene";
		m_SimpleName = "Scene";

		m_ShowComponentGizmoMap[typeid(Graphics::Light).hash_code()] = true;
		m_ShowComponentGizmoMap[typeid(CameraComponent).hash_code()] = true;
		m_ShowComponentGizmoMap[typeid(SoundComponent).hash_code()] = true;
	}

	void SceneWindow::OnImGui()
	{
		auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
		ImGui::SetNextWindowBgAlpha(0.0f);
		ImGui::Begin(m_Name.c_str(), &m_Active, flags);

        ToolBar();
        
		ImGuizmo::SetDrawlist();
		auto sceneViewSize = ImGui::GetContentRegionAvail();
        auto sceneViewPosition = ImGui::GetWindowPos();
        
        sceneViewPosition.x = ImGui::GetCursorPos().x + ImGui::GetWindowPos().x;
        sceneViewPosition.y = ImGui::GetCursorPos().y + ImGui::GetWindowPos().y;

        sceneViewSize.x = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
        sceneViewSize.y = ImGui::GetWindowContentRegionMax().y - ImGui::GetWindowContentRegionMin().y;

        sceneViewSize.x -= static_cast<int>(sceneViewSize.x) % 2 != 0 ? 1.0f : 0.0f;
        sceneViewSize.y -= static_cast<int>(sceneViewSize.y) % 2 != 0 ? 1.0f : 0.0f;

        Camera* camera = Application::Instance()->GetSceneManager()->GetCurrentScene()->GetCamera();
		camera->SetAspectRatio(static_cast<float>(sceneViewSize.x) / static_cast<float>(sceneViewSize.y));

		ImGuizmo::SetRect(sceneViewPosition.x, sceneViewPosition.y, sceneViewSize.x, sceneViewSize.y);
        
        ImGuiHelpers::Image(Application::Instance()->GetRenderManager()->GetGBuffer()->GetTexture(Graphics::SCREENTEX_OFFSCREEN0), {sceneViewSize.x, sceneViewSize.y});

		if (m_Editor->ShowGrid())
		{
			if (camera->Is2D())
			{
				m_Editor->Draw2DGrid(ImGui::GetWindowDrawList(), { camera->GetPosition().GetX(), camera->GetPosition().GetY() }, sceneViewPosition, { sceneViewSize.x, sceneViewSize.y }, camera->GetScale(), 1.5f);
			}
			else
			{
#if 0 
				Maths::Matrix4 view = camera->GetViewMatrix();
				Maths::Matrix4 proj = camera->GetProjectionMatrix();
				Maths::Matrix4 identityMatrix;

#ifdef LUMOS_RENDER_API_VULKAN
				if (Graphics::GraphicsContext::GetRenderAPI() == Graphics::RenderAPI::VULKAN)
					proj[5] *= -1.0f;
#endif

				ImGuizmo::DrawGrid(view.values, proj.values, identityMatrix.values, m_Editor->GetGridSize(), 1.0f);
#endif
			}
		}


		m_Editor->OnImGuizmo();
        
        DrawGizmos(sceneViewSize.x, sceneViewSize.y, 0.0f, 40.0f, Application::Instance()->GetSceneManager()->GetCurrentScene()); // Not sure why 40
		Application::Instance()->SetSceneActive(ImGui::IsWindowFocused() && !ImGuizmo::IsUsing());
        
		if (m_ShowStats)
		{
			static bool p_open = true;
			const float DISTANCE = 5.0f;
			static int corner = 0;

			if (corner != -1)
			{
				ImVec2 window_pos = ImVec2((corner & 1) ? (sceneViewPosition.x + sceneViewSize.x - DISTANCE) : (sceneViewPosition.x + DISTANCE), (corner & 2) ? (sceneViewPosition.y + sceneViewSize.y - DISTANCE) : (sceneViewPosition.y + DISTANCE));
				ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
				ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
			}

			ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
			if (ImGui::Begin("Example: Simple overlay", &p_open, (corner != -1 ? ImGuiWindowFlags_NoMove : 0) | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
			{
				ImGui::Text("%.2f ms (%i FPS)", 1000.0f / (float)Engine::Instance()->GetFPS(), Engine::Instance()->GetFPS());
				ImGui::Separator();
				ImGuiIO& io = ImGui::GetIO();
				if (ImGui::IsMousePosValid())
					ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
				else
					ImGui::Text("Mouse Position: <invalid>");
				if (ImGui::BeginPopupContextWindow())
				{
					if (ImGui::MenuItem("Custom", NULL, corner == -1)) corner = -1;
					if (ImGui::MenuItem("Top-left", NULL, corner == 0)) corner = 0;
					if (ImGui::MenuItem("Top-right", NULL, corner == 1)) corner = 1;
					if (ImGui::MenuItem("Bottom-left", NULL, corner == 2)) corner = 2;
					if (ImGui::MenuItem("Bottom-right", NULL, corner == 3)) corner = 3;
					if (p_open && ImGui::MenuItem("Close")) p_open = false;
					ImGui::EndPopup();
				}
			}
			ImGui::End();
		}
        
		ImGui::End();
	}

	void SceneWindow::DrawGizmos(float width, float height, float xpos, float ypos, Scene* scene)
	{	
		Camera* camera = Application::Instance()->GetSceneManager()->GetCurrentScene()->GetCamera();

		Maths::Matrix4 view = camera->GetViewMatrix();
		Maths::Matrix4 proj = camera->GetProjectionMatrix();

#ifdef LUMOS_RENDER_API_VULKAN
        if (Graphics::GraphicsContext::GetRenderAPI() == Graphics::RenderAPI::VULKAN)
            proj[5] *= -1.0f;
#endif

		Maths::Matrix4 viewProj = proj * view;
		Maths::Frustum f;
		f.FromMatrix(viewProj);

		auto& registry = scene->GetRegistry();
		ShowComponentGizmo<Graphics::Light>(width, height, xpos, ypos, viewProj, f, registry);
		ShowComponentGizmo<CameraComponent>(width, height, xpos, ypos, viewProj, f, registry);
		ShowComponentGizmo<SoundComponent>(width, height, xpos, ypos, viewProj, f, registry);
	}

    void SceneWindow::ToolBar()
    {
		ImGui::Indent();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));
        bool selected = false;

		{
			selected = m_Editor->GetImGuizmoOperation() == 4;
			if (selected)
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.9f, 1.0f));
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_MOUSE_POINTER, ImVec2(19.0f, 19.0f)))
				m_Editor->SetImGuizmoOperation(4);

			if (selected)
				ImGui::PopStyleColor();
			ImGuiHelpers::Tooltip("Translate");
		}

		ImGui::SameLine(); ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);  ImGui::SameLine();

        {
            selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::TRANSLATE;
            if (selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.9f, 1.0f));
			ImGui::SameLine();
            if (ImGui::Button(ICON_FA_ARROWS_ALT, ImVec2(19.0f, 19.0f)))
                    m_Editor->SetImGuizmoOperation(ImGuizmo::TRANSLATE);

			if (selected)
				ImGui::PopStyleColor();
            ImGuiHelpers::Tooltip("Translate");
        }

        {
            selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::ROTATE;
			if (selected)
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.9f, 1.0f));

            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_SYNC, ImVec2(19.0f, 19.0f)))
                    m_Editor->SetImGuizmoOperation(ImGuizmo::ROTATE);

			if (selected)
				ImGui::PopStyleColor();
            ImGuiHelpers::Tooltip("Rotate");
        }

        {
            selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::SCALE;
			if (selected)
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.9f, 1.0f));

            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_EXPAND_ARROWS_ALT, ImVec2(19.0f, 19.0f)))
                m_Editor->SetImGuizmoOperation(ImGuizmo::SCALE);

			if (selected)
				ImGui::PopStyleColor();
            ImGuiHelpers::Tooltip("Scale");
        }

		ImGui::SameLine(); ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);  ImGui::SameLine();

		{
			selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::BOUNDS;
			if (selected)
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.9f, 1.0f));

			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_SQUARE, ImVec2(19.0f, 19.0f)))
				m_Editor->SetImGuizmoOperation(ImGuizmo::BOUNDS);

			if (selected)
				ImGui::PopStyleColor();
			ImGuiHelpers::Tooltip("Bounds");
		}

		ImGui::SameLine(); ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);  ImGui::SameLine();
        
		ImGui::SameLine();
                
		if (ImGui::Button("Gizmos"))
			ImGui::OpenPopup("GizmosPopup");
		if (ImGui::BeginPopup("GizmosPopup"))
		{
			{
				ImGui::Checkbox("Grid", &m_Editor->ShowGrid());

				ImGui::Separator();
				ImGui::Checkbox("Camera", &m_ShowComponentGizmoMap[typeid(CameraComponent).hash_code()]);
				ImGui::Checkbox("Light", &m_ShowComponentGizmoMap[typeid(Graphics::Light).hash_code()]);
				ImGui::Checkbox("Audio", &m_ShowComponentGizmoMap[typeid(SoundComponent).hash_code()]);
				ImGui::EndPopup();
			}
		}

		ImGui::SameLine();
		{
			selected = m_ShowStats;
			if (selected)
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.9f, 1.0f));

			ImGui::SameLine();
			if (ImGui::Button("Stats"))
			{
				m_ShowStats = !m_ShowStats;

			}

			if (selected)
				ImGui::PopStyleColor();
			ImGuiHelpers::Tooltip("Show Statistics");
		}

		ImGui::PopStyleColor();
		ImGui::Unindent();
    }
}
