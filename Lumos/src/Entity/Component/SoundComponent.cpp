#include "LM.h"
#include "SoundComponent.h"
#include "Audio/SoundNode.h"
#include "Renderer/Scene.h"
#include "Audio/SoundSystem.h"
#include "Maths/BoundingSphere.h"
#include "Graphics/Renderers/DebugRenderer.h"

namespace Lumos
{
	SoundComponent::SoundComponent(std::shared_ptr<SoundNode>& sound)
		: m_SoundNode(sound)
	{
		m_BoundingShape = std::make_unique<maths::BoundingSphere>(sound->GetPosition(),sound->GetRadius());
	}

	void SoundComponent::OnUpdateComponent(float dt)
	{
		Physics3DComponent* physicsComponent = m_Entity->GetComponent<Physics3DComponent>();
		if (physicsComponent)
		{
			m_SoundNode->SetPosition(physicsComponent->m_PhysicsObject->GetPosition());
			m_BoundingShape->SetPosition(m_SoundNode->GetPosition());
		}
	}

	void SoundComponent::DebugDraw(uint64 debugFlags)
	{
		DebugRenderer::DebugDraw(static_cast<maths::BoundingSphere*>(m_BoundingShape.get()), maths::Vector4(0.7f,0.2f,0.4f, 0.2f));
	}

	void SoundComponent::Init()
	{
		SoundSystem::Instance()->AddSoundNode(m_SoundNode);
	}
}
