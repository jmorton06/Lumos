#include "lmpch.h"
#include "SceneWindow.h"
#include "Editor.h"
#include "Graphics/Camera/Camera.h"
#include "App/Application.h"
#include "App/SceneManager.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/RenderManager.h"
#include "Graphics/GBuffer.h"
#include "Graphics/Light.h"

#include <imgui/plugins/ImGuizmo.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>

namespace Lumos
{
	SceneWindow::SceneWindow()
	{
		m_Name = ICON_FA_GAMEPAD" Scene###scene";
		m_SimpleName = "Scene";

		m_ComponentIconMap[typeid(Graphics::Light).hash_code()] = ICON_FA_LIGHTBULB;
		m_ComponentIconMap[typeid(CameraComponent).hash_code()] = ICON_FA_VIDEO;
		m_ComponentIconMap[typeid(SoundComponent).hash_code()] = ICON_FA_VOLUME_UP;

		m_ShowComponentGizmoMap[typeid(Graphics::Light).hash_code()] = true;
		m_ShowComponentGizmoMap[typeid(CameraComponent).hash_code()] = true;
		m_ShowComponentGizmoMap[typeid(SoundComponent).hash_code()] = true;
	}

	void SceneWindow::OnImGui()
	{
		auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
		ImGui::SetNextWindowBgAlpha(0.0f);
		ImGui::Begin(m_Name.c_str(), &m_Active, flags);

		ImGuizmo::SetDrawlist();
		auto sceneViewSize = Maths::Vector2(ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);
		auto sceneViewPosition = Maths::Vector2(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y);

		Camera* camera = Application::Instance()->GetSceneManager()->GetCurrentScene()->GetCamera();
		camera->SetAspectRatio(static_cast<float>(ImGui::GetWindowSize().x) / static_cast<float>(ImGui::GetWindowSize().y));

		auto width = static_cast<unsigned int>(ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x + 2);
		auto height = static_cast<unsigned int>(ImGui::GetWindowContentRegionMax().y - ImGui::GetWindowContentRegionMin().y + 22);

		// Make pixel perfect
		width -= (width % 2 != 0) ? 1 : 0;
		height -= (height % 2 != 0) ? 1 : 0;

		bool flipImage = Graphics::GraphicsContext::GetContext()->FlipImGUITexture();

		ImGui::SetCursorPos({ 0,0 });

		ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, static_cast<float>(width), static_cast<float>(height));
		ImGui::Image(Application::Instance()->GetRenderManager()->GetGBuffer()->GetTexture(Graphics::SCREENTEX_OFFSCREEN0)->GetHandle(), ImVec2(static_cast<float>(width), static_cast<float>(height)), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));

		if (m_Editor->GetShowGrid())
		{
			if (camera->Is2D())
			{
				m_Editor->Draw2DGrid(ImGui::GetWindowDrawList(), { camera->GetPosition().GetX(), camera->GetPosition().GetY() }, ImGui::GetWindowPos(), { sceneViewSize.GetX(), sceneViewSize.GetY() }, camera->GetScale(), 1.5f);
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
		DrawGizmos(static_cast<float>(width), static_cast<float>(height), ImGui::GetWindowPos().x, ImGui::GetWindowPos().y);
		Application::Instance()->SetSceneActive(ImGui::IsWindowFocused() && !ImGuizmo::IsUsing());
		ImGui::End();
	}

	void SceneWindow::DrawGizmos(float width, float height, float xpos, float ypos)
	{	
		Camera* camera = Application::Instance()->GetSceneManager()->GetCurrentScene()->GetCamera();

		Maths::Matrix4 view = camera->GetViewMatrix();
		Maths::Matrix4 proj = camera->GetProjectionMatrix();

		if (Graphics::GraphicsContext::GetRenderAPI() == Graphics::RenderAPI::VULKAN)
			proj[5] *= -1.0f;

		Maths::Matrix4 viewProj = proj * view;
		Maths::Frustum f;
		f.FromMatrix(viewProj);

		ShowComponentGizmo<Graphics::Light>(width, height, xpos, ypos, viewProj, f);
		ShowComponentGizmo<CameraComponent>(width, height, xpos, ypos, viewProj, f);
		ShowComponentGizmo<SoundComponent>(width, height, xpos, ypos, viewProj, f);
	}
}
