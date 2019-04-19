#include "LM.h"
#include "MeshComponent.h"
#include "Graphics/Mesh.h"
#include "Maths/BoundingSphere.h"
#include "Physics3DComponent.h"
#include "Physics/LumosPhysicsEngine/PhysicsObject3D.h"
#include "Entity/Entity.h"
#include "Graphics/Material.h"

#include <imgui/imgui.h>


namespace Lumos
{
	MeshComponent::MeshComponent(std::shared_ptr<Mesh>& model)
		: m_Model(model)
	{
		m_BoundingShape = std::make_unique<maths::BoundingSphere>(maths::Vector3(0.0f),1.0f);
	}

	void MeshComponent::OnUpdateComponent(float dt)
	{
		Physics3DComponent* physicsComponent = m_Entity->GetComponent<Physics3DComponent>();
		if (physicsComponent)
		{
			m_BoundingShape->SetPosition(physicsComponent->m_PhysicsObject->GetPosition());
		}
	}

	void MeshComponent::OnIMGUI()
	{
        if (ImGui::TreeNode("Material"))
		{
			MaterialProperties* prop = m_Model->GetMaterial()->GetProperties();

			ImGui::SliderFloat("Use Albedo Map", &prop->usingAlbedoMap, 0.0f, 1.0f);
			ImGui::SliderFloat("Use Specular Map", &prop->usingSpecularMap, 0.0f, 1.0f);
			ImGui::SliderFloat("Use Gloss Map", &prop->usingGlossMap, 0.0f, 1.0f);
			ImGui::SliderFloat("Use Normal Map", &prop->usingNormalMap, 0.0f, 1.0f);

			ImGui::SliderFloat3("Albedo", &prop->albedoColour.x, 0.0f, 1.0f);
			ImGui::SliderFloat3("Gloss", &prop->glossColour.x, 0.0f, 1.0f);
			ImGui::SliderFloat3("Specular", &prop->specularColour.x, 0.0f, 1.0f);

			m_Model->GetMaterial()->SetMaterialProperites(*prop);
			ImGui::TreePop();
		}
	}

}
