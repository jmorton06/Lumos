#include "LM.h"
#include "CameraComponent.h"
#include "Entity/Entity.h"
#include "Graphics/Camera/Camera.h"
#include "App/Application.h"

#include <imgui/imgui.h>

namespace Lumos
{
	CameraComponent::CameraComponent(Camera* camera)
		: m_Camera(camera)
	{

	}

	void CameraComponent::OnUpdateComponent(float dt)
	{
	}

    void CameraComponent::OnIMGUI()
    {
        if (ImGui::TreeNode("CameraComponent"))
        {
			if (ImGui::Button("Set Active"))
				SetAsMainCamera();

			m_Camera->OnImGUI();
			ImGui::TreePop();
        }
    }

	void CameraComponent::SetAsMainCamera()
	{
		Application::Instance()->SetActiveCamera(m_Camera);
	}
}
