#include "JM.h"
#include "ModelComponent.h"
#include "Graphics/Model/Model.h"
#include "Maths/BoundingSphere.h"
#include "Physics3DComponent.h"
#include "Physics/JMPhysicsEngine/PhysicsObject3D.h"
#include "Entity/Entity.h"


namespace jm
{
	ModelComponent::ModelComponent(std::shared_ptr<Model>& model)
		: m_Model(model)
	{
		m_BoundingShape = std::make_unique<maths::BoundingSphere>(maths::Vector3(0.0f),1.0f);
	}

	void ModelComponent::OnUpdateComponent(float dt)
	{
		Physics3DComponent* physicsComponent = m_Entity->GetComponent<Physics3DComponent>();
		if (physicsComponent)
		{
			m_BoundingShape->SetPosition(physicsComponent->m_PhysicsObject->GetPosition());
		}
	}

}
