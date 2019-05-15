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

namespace Lumos
{
	LightComponent::LightComponent(std::shared_ptr<Light>& light)
		: m_Light(light)
	{
		m_BoundingShape = std::make_unique<maths::BoundingSphere>(light->GetPosition(), light->GetRadius() * light->GetRadius());
	}
    
    LightComponent::~LightComponent()
    {
    }

	void LightComponent::SetRadius(float radius)
	{
		m_Light->SetRadius(radius);
		m_BoundingShape->SetRadius(radius);
	}

	void LightComponent::OnUpdateComponent(float dt)
	{
        m_Light->SetDirection(m_Entity->GetTransform()->m_Transform.GetWorldMatrix().GetPositionVector());
        m_Light->SetPosition(m_Entity->GetTransform()->m_Transform.GetWorldMatrix().GetPositionVector());
        m_BoundingShape->SetPosition(m_Light->GetPosition());
	}

	void LightComponent::Init()
	{
	}

	void LightComponent::DebugDraw(uint64 debugFlags)
	{
		DebugRenderer::DebugDraw(static_cast<maths::BoundingSphere*>(m_BoundingShape.get()), maths::Vector4(m_Light->GetColour(),0.2f));
	}

	String LightTypeToString(LightType type)
	{
		switch (type)
		{
		case LightType::DirectionalLight : return "Directional Light";
		case LightType::SpotLight: return "Spot Light";
		case LightType::PointLight: return "Point Light";
		default: return "ERROR";
		}
	}

	void LightComponent::OnIMGUI()
	{
		if (ImGui::TreeNode("Light"))
		{
			auto pos = m_Light->GetPosition();
			auto radius = m_Light->GetRadius();
			auto isOn = m_Light->GetIsOn();
			auto colour = m_Light->GetColour();
			auto brightness = m_Light->GetBrightness();
			auto lightType = m_Light->GetLightType();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Position");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			if (ImGui::InputFloat3("##Position", &pos.x))
				m_Light->SetPosition(pos);

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Radius");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			if (ImGui::InputFloat("##Radius", &radius))
				m_Light->SetRadius(radius);

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("On");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			if (ImGui::Checkbox("##On", &isOn))
				m_Light->SetIsOn(isOn);

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Colour");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			if (ImGui::DragFloat4("##Colour", &colour.x))
				m_Light->SetColour(colour);

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Brightness");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			if (ImGui::DragFloat("##Brightness", &brightness))
				m_Light->SetBrightness(brightness);

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Light Type");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			if (ImGui::BeginMenu(LightTypeToString(m_Light->GetLightType()).c_str()))
			{
				if (ImGui::MenuItem("Directional Light", "", static_cast<int>(m_Light->GetLightType()) == 0, true)) { m_Light->SetLightType(LightType::DirectionalLight); }
				if (ImGui::MenuItem("Spot Light", "", static_cast<int>(m_Light->GetLightType()) == 1, true)) { m_Light->SetLightType(LightType::SpotLight); }
				if (ImGui::MenuItem("Point Light", "", static_cast<int>(m_Light->GetLightType()) == 2, true)) { m_Light->SetLightType(LightType::PointLight); }
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
