#include "lmpch.h"
#include "LightComponent.h"
#include "Physics3DComponent.h"
#include "Physics/LumosPhysicsEngine/PhysicsObject3D.h"
#include "ECS/EntityManager.h"
#include "App/Scene.h"
#include "Maths/Vector3.h"
#include "Maths/BoundingSphere.h"
#include "TransformComponent.h"

#include <imgui/imgui.h>

namespace Lumos
{
    LightComponent::LightComponent()
    {
        m_Light = CreateRef<Graphics::Light>();
    }
    
	LightComponent::LightComponent(const Ref<Graphics::Light>& light)
		: m_Light(light)
	{
	}
    
    LightComponent::~LightComponent()
    {
    }

	void LightComponent::SetRadius(float radius)
	{
		m_Light->m_Radius = radius;
	}

	void LightComponent::Update()
	{
		//auto euler = Maths::Matrix4::GetEulerAngles(m_Entity->GetTransformComponent()->GetTransform()->GetWorldMatrix());
		//float x = cos(euler.y)*cos(euler.x);
		//float y = sin(euler.y)*cos(euler.x);
		//float z = sin(euler.x);
		//m_Light->m_Direction = Maths::Vector4(x,y,z, 1.0f);
		auto entity = ComponentManager::Instance()->GetComponentArray<LightComponent>()->GetEntity(this);
		m_Light->m_Position  = Maths::Vector4(entity->GetTransformComponent()->GetTransform()->GetWorldMatrix().GetPositionVector(), 1.0f);
	}

	void LightComponent::Init()
	{
	}

	String LightTypeToString(Graphics::LightType type)
	{
		switch (type)
		{
		case Graphics::LightType::DirectionalLight : return "Directional Light";
		case Graphics::LightType::SpotLight: return "Spot Light";
		case Graphics::LightType::PointLight: return "Point Light";
		default: return "ERROR";
		}
	}

	void LightComponent::OnImGui()
	{
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Position");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		ImGui::InputFloat3("##Position", &m_Light->m_Position.x);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Direction");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		ImGui::InputFloat3("##Direction", &m_Light->m_Direction.x);

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
		if (ImGui::BeginMenu(LightTypeToString(Graphics::LightType(int(m_Light->m_Type))).c_str()))
		{
			if (ImGui::MenuItem("Directional Light", "", static_cast<int>(m_Light->m_Type) == 0, true)) { m_Light->m_Type = float(int(Graphics::LightType::DirectionalLight)); }
			if (ImGui::MenuItem("Spot Light", "", static_cast<int>(m_Light->m_Type) == 1, true)) { m_Light->m_Type = float(int(Graphics::LightType::SpotLight)); }
			if (ImGui::MenuItem("Point Light", "", static_cast<int>(m_Light->m_Type) == 2, true)) { m_Light->m_Type = float(int(Graphics::LightType::PointLight)); }
			ImGui::EndMenu();
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::Columns(1);
		ImGui::Separator();
		ImGui::PopStyleVar();
	}

	nlohmann::json LightComponent::Serialise() 
	{
		nlohmann::json output;
		output["typeID"] = LUMOS_TYPENAME(LightComponent);
		output["position"]	= m_Light->m_Position.Serialise();
		output["direction"] = m_Light->m_Direction.Serialise();
		output["colour"]	= m_Light->m_Colour.Serialise();
		output["intensity"] = m_Light->m_Intensity;
		output["radius"]	= m_Light->m_Radius;
		output["type"]		= m_Light->m_Type;

		return output;
	}

	void LightComponent::Deserialise(nlohmann::json & data)
	{
		m_Light->m_Position.Deserialise(data["position"]);
		m_Light->m_Direction.Deserialise(data["direction"]);
		m_Light->m_Colour.Deserialise(data["colour"]);
		m_Light->m_Intensity	= data["intensity"];
		m_Light->m_Radius		= data["radius"];
		m_Light->m_Type			= data["type"];
	}
}
