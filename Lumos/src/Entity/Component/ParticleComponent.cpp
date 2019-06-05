#include "LM.h"
#include "ParticleComponent.h"
#include "Graphics/ParticleEmitter.h"
#include "Entity/Entity.h"
#include "App/Scene.h"
#include "Graphics/ParticleManager.h"
#include "Maths/BoundingSphere.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include <imgui/imgui.h>
#include "Physics/LumosPhysicsEngine/PhysicsObject3D.h"

namespace Lumos
{
    ParticleComponent::ParticleComponent(std::shared_ptr<ParticleEmitter>& emitter)
            : m_ParticleEmitter(emitter), m_PositionOffset(Maths::Vector3(0.0f,0.0f,0.0f))
    {
		m_Name = "Particle";
		m_BoundingShape = std::unique_ptr<Maths::BoundingBox>(emitter->CalculateBoundingBox());
    }

    void ParticleComponent::Init()
    {
    }

	void ParticleComponent::DebugDraw(uint64 debugFlags)
	{
		DebugRenderer::DebugDraw(static_cast<Maths::BoundingBox*>(m_BoundingShape.get()), Maths::Vector4(0.2f, 0.0f, 0.8f, 0.2f));
	}

	void ParticleComponent::OnUpdateComponent(float dt)
    {
        Physics3DComponent* physicsComponent = m_Entity->GetComponent<Physics3DComponent>();
        if (physicsComponent)
            m_ParticleEmitter->SetPosition(physicsComponent->m_PhysicsObject->GetPosition() + m_PositionOffset);

	
    }

	void ParticleComponent::OnIMGUI()
	{
	}
}
