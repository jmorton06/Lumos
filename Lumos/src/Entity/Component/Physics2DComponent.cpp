#include "LM.h"
#include "Physics2DComponent.h"
#include "Physics/B2PhysicsEngine/PhysicsObject2D.h"

namespace Lumos
{
	Physics2DComponent::Physics2DComponent(std::shared_ptr<PhysicsObject2D>& physics)
		: m_PhysicsObject(physics)
	{

	}

}
