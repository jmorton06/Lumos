#include "LM.h"
#include "ModelComponent.h"
#include "Graphics/Model/Model.h"
#include "Maths/BoundingSphere.h"
#include "Physics3DComponent.h"
#include "Physics/LumosPhysicsEngine/PhysicsObject3D.h"
#include "Entity/Entity.h"

#include <imgui/imgui.h>


namespace Lumos
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

	void ModelComponent::OnIMGUI()
	{
		if (ImGui::TreeNode("Model"))
		{
			auto meshes = m_Model->GetMeshs();
			ImGui::Text("Number Of Meshes", "%s", static_cast<int>(meshes.size()));
			
			if (ImGui::TreeNode("Meshes"))
			{
				for (auto& mesh : meshes)
				{
					if (ImGui::TreeNode("Mesh"))
					{
                        if (ImGui::TreeNode("Material : ", "%s", mesh->GetMaterial()->GetName().c_str()))
						{
							MaterialProperties* prop = mesh->GetMaterial()->GetProperties();

							ImGui::SliderFloat("Use Albedo Map", &prop->usingAlbedoMap, 0.0f, 1.0f);
							ImGui::SliderFloat("Use Specular Map", &prop->usingSpecularMap, 0.0f, 1.0f);
							ImGui::SliderFloat("Use Gloss Map", &prop->usingGlossMap, 0.0f, 1.0f);
							ImGui::SliderFloat("Use Normal Map", &prop->usingNormalMap, 0.0f, 1.0f);

							ImGui::SliderFloat3("Albedo", &prop->albedoColour.x, 0.0f, 1.0f);
							ImGui::SliderFloat3("Gloss", &prop->glossColour.x, 0.0f, 1.0f);
							ImGui::SliderFloat3("Specular", &prop->specularColour.x, 0.0f, 1.0f);

							mesh->GetMaterial()->SetMaterialProperites(*prop);
							ImGui::TreePop();
						}
						ImGui::TreePop();
					}
				}
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}
	}

}
