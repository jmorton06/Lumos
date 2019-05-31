#include "LM.h"
#include "LightComponent.h"
#include "Graphics/Light.h"
#include "Physics3DComponent.h"
#include "Physics/LumosPhysicsEngine/PhysicsObject3D.h"
#include "Entity/Entity.h"
#include "App/Scene.h"
#include "Graphics/LightSetUp.h"
#include "Maths/Vector3.h"
#include "Maths/BoundingSphere.h"
#include "Graphics/Renderers/DebugRenderer.h"

#include <imgui/imgui.h>

namespace lumos
{
	LightComponent::LightComponent(std::shared_ptr<graphics::Light>& light)
		: m_Light(light)
	{
		m_BoundingShape = std::make_unique<maths::BoundingSphere>(light->m_Position.ToVector3(), light->m_Radius * light->m_Radius);
	}
    
    LightComponent::~LightComponent()
    {
    }

	void LightComponent::SetRadius(float radius)
	{
		m_Light->m_Radius = radius;
		m_BoundingShape->SetRadius(radius);
	}

	void LightComponent::OnUpdateComponent(float dt)
	{
		   //m_Light->m_Direction = maths::Vector4(maths::Matrix4::GetEulerAngles(m_Entity->GetTransformComponent()->GetTransform().GetWorldMatrix()), 1.0f);
           m_Light->m_Position = maths::Vector4(m_Entity->GetTransformComponent()->GetTransform().GetWorldMatrix().GetPositionVector(), 1.0f);
           m_BoundingShape->SetPosition(m_Light->m_Position.ToVector3());
	}

	void LightComponent::Init()
	{
	}

	void LightComponent::DebugDraw(uint64 debugFlags)
	{
	}

	String LightTypeToString(graphics::LightType type)
	{
		switch (type)
		{
		case graphics::LightType::DirectionalLight : return "Directional Light";
		case graphics::LightType::SpotLight: return "Spot Light";
		case graphics::LightType::PointLight: return "Point Light";
		default: return "ERROR";
		}
	}

	void LightComponent::OnIMGUI()
	{
		if (ImGui::TreeNode("Light"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Position");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::InputFloat3("##Position", &m_Light->m_Position.x);

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Radius");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::InputFloat("##Radius", &m_Light->m_Radius);

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Colour");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::ColorEdit4("##Colour", &m_Light->m_Colour.x);

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Intensity");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::DragFloat("##Intensity", &m_Light->m_Intensity);

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Light Type");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			if (ImGui::BeginMenu(LightTypeToString(graphics::LightType(int(m_Light->m_Type))).c_str()))
			{
				if (ImGui::MenuItem("Directional Light", "", static_cast<int>(m_Light->m_Type) == 0, true)) { m_Light->m_Type = float(int(graphics::LightType::DirectionalLight)); }
				if (ImGui::MenuItem("Spot Light", "", static_cast<int>(m_Light->m_Type) == 1, true)) { m_Light->m_Type = float(int(graphics::LightType::SpotLight)); }
				if (ImGui::MenuItem("Point Light", "", static_cast<int>(m_Light->m_Type) == 2, true)) { m_Light->m_Type = float(int(graphics::LightType::PointLight)); }
				ImGui::EndMenu();
			}

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::PopStyleVar();

			ImGui::TreePop();
		}
	}
}
