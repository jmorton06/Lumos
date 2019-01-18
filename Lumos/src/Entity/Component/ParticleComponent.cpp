#include "LM.h"
#include "ParticleComponent.h"
#include "Graphics/ParticleEmitter.h"
#include "Entity/Entity.h"
#include "App/Scene.h"
#include "Graphics/ParticleManager.h"
#include "Maths/BoundingSphere.h"
#include "Graphics/Renderers/DebugRenderer.h"

namespace Lumos
{
    ParticleComponent::ParticleComponent(std::shared_ptr<ParticleEmitter>& emitter)
            : m_ParticleEmitter(emitter), m_PositionOffset(maths::Vector3(0.0f,0.0f,0.0f))
    {
		m_BoundingShape = std::unique_ptr<maths::BoundingBox>(emitter->CalculateBoundingBox());
    }

    void ParticleComponent::Init()
    {
        m_Entity->GetScene()->GetParticleSystem()->Add(m_ParticleEmitter);
    }

	void ParticleComponent::DebugDraw(uint64 debugFlags)
	{
		DebugRenderer::DebugDraw(static_cast<maths::BoundingBox*>(m_BoundingShape.get()), maths::Vector4(0.2f, 0.0f, 0.8f, 0.2f));
	}

	void ParticleComponent::OnUpdateComponent(float dt)
    {
        Physics3DComponent* physicsComponent = m_Entity->GetComponent<Physics3DComponent>();
        if (physicsComponent)
            m_ParticleEmitter->SetPosition(physicsComponent->m_PhysicsObject->GetPosition() + m_PositionOffset);

	
    }
}
