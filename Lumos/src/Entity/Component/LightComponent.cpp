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

namespace Lumos
{
	LightComponent::LightComponent(std::shared_ptr<Light>& light)
		: m_Light(light), m_PostionOffset(maths::Vector3(0.0f))
	{
		m_BoundingShape = std::make_unique<maths::BoundingSphere>(light->GetPosition() + m_PostionOffset, light->GetRadius() * light->GetRadius());
	}


	void LightComponent::SetRadius(float radius)
	{
		m_Light->SetRadius(radius);
		m_BoundingShape->SetRadius(radius);
	}

	void LightComponent::OnUpdateComponent(float dt)
	{
		Physics3DComponent* physicsComponent = m_Entity->GetComponent<Physics3DComponent>();
		if (physicsComponent)
		{
			m_Light->SetPosition(physicsComponent->m_PhysicsObject->GetPosition() + m_PostionOffset);
			m_BoundingShape->SetPosition(m_Light->GetPosition() + m_PostionOffset);
		}
	}

	void LightComponent::Init()
	{
		m_Entity->GetScene()->GetLightSetup()->Add(m_Light);
	}

	void LightComponent::DebugDraw(uint64 debugFlags)
	{
		DebugRenderer::DebugDraw(static_cast<maths::BoundingSphere*>(m_BoundingShape.get()), maths::Vector4(m_Light->GetColour(),0.2f));
	}
}
