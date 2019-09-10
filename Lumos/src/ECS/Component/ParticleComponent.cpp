#include "LM.h"
#include "ParticleComponent.h"
#include "ECS/EntityManager.h"
#include "App/Scene.h"
#include "Graphics/ParticleManager.h"
#include "Physics/LumosPhysicsEngine/PhysicsObject3D.h"
#include <imgui/imgui.h>

namespace Lumos
{
    ParticleComponent::ParticleComponent()
    {
        m_Name = "Particle";
        m_BoundingShape = Scope<Maths::BoundingBox>();
    }
    
    ParticleComponent::ParticleComponent(Ref<ParticleEmitter>& emitter)
            : m_ParticleEmitter(emitter), m_PositionOffset(Maths::Vector3(0.0f,0.0f,0.0f))
    {
		m_Name = "Particle";
		m_BoundingShape = Scope<Maths::BoundingBox>(emitter->CalculateBoundingBox());
    }

    void ParticleComponent::Init()
    {
    }

	void ParticleComponent::OnImGui()
	{
	}
}
