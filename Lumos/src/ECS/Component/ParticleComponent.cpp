#include "lmpch.h"
#include "ParticleComponent.h"
#include "Graphics/ParticleManager.h"
#include <imgui/imgui.h>

namespace Lumos
{
    ParticleComponent::ParticleComponent()
    {
    }
    
    ParticleComponent::ParticleComponent(Ref<ParticleEmitter>& emitter)
            : m_ParticleEmitter(emitter), m_PositionOffset(Maths::Vector3(0.0f,0.0f,0.0f))
    {
    }

    void ParticleComponent::Init()
    {
    }

	void ParticleComponent::OnImGui()
	{
	}
}
