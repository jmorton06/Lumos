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
        if(m_Entity->GetScene())
            m_Entity->GetScene()->GetLightSetup()->Remove(m_Light);
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
		m_Entity->GetScene()->GetLightSetup()->Add(m_Light);
	}

	void LightComponent::DebugDraw(uint64 debugFlags)
	{
		DebugRenderer::DebugDraw(static_cast<maths::BoundingSphere*>(m_BoundingShape.get()), maths::Vector4(m_Light->GetColour(),0.2f));
	}

	void LightComponent::OnIMGUI()
	{
		if (ImGui::TreeNode("Light"))
		{
			ImGui::TreePop();
		}
	}
}
