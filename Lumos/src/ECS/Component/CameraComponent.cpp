#include "lmpch.h"
#include "CameraComponent.h"
#include "Graphics/Camera/Camera.h"
#include "App/Application.h"

#include <imgui/imgui.h>

namespace Lumos
{
    CameraComponent::CameraComponent()
    {
        m_Camera = nullptr;
    }
    
	CameraComponent::CameraComponent(Camera* camera)
		: m_Camera(camera)
	{
	}

    void CameraComponent::OnImGui()
    {
		if (ImGui::Button("Set Active"))
			SetAsMainCamera();

		m_Camera->OnImGui();
    }

	void CameraComponent::SetAsMainCamera()
	{
		Application::Instance()->SetActiveCamera(m_Camera);
	}
}
