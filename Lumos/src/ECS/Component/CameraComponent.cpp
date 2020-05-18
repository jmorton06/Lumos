#include "lmpch.h"
#include "CameraComponent.h"
#include "Graphics/Camera/Camera.h"
#include "App/Application.h"

#include <imgui/imgui.h>

namespace Lumos
{
    CameraComponent::CameraComponent()
    {
        m_Camera = new Camera(60.0f, 0.1f, 100.0f, 1.0f);
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
