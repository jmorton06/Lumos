#include "LM.h"
#include "CameraComponent.h"
#include "Entity/Entity.h"
#include "Graphics/Camera/Camera.h"

#include <imgui/imgui.h>

namespace lumos
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
			m_Camera->OnImGUI();
			ImGui::TreePop();
        }
    }
}
