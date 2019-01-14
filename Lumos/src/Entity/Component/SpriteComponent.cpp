#include "JM.h"
#include "SpriteComponent.h"
#include "Graphics/Sprite.h"
#include "Physics2DComponent.h"
#include "Entity/Entity.h"
#include "Physics/B2PhysicsEngine/PhysicsObject2D.h"
#include "Maths/MathsUtilities.h"

namespace jm
{
	SpriteComponent::SpriteComponent(std::shared_ptr<Sprite>& sprite)
		: m_Sprite(sprite)
	{

	}

	void SpriteComponent::OnUpdateComponent(float dt)
	{
		Physics2DComponent* physicsComponent = m_Entity->GetComponent<Physics2DComponent>();
		if (physicsComponent)
		{
            m_Sprite->m_Position = physicsComponent->m_PhysicsObject->GetPosition();
			m_Sprite->m_RotationMatrix = maths::Matrix4::Rotation(static_cast<float>(maths::RadToDeg(physicsComponent->m_PhysicsObject->GetAngle())), maths::Vector3::ZAxis());
		}
			
	}

}
