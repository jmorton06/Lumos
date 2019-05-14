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

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2,2));
            ImGui::Columns(2);
            ImGui::Separator();
            
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Use Albedo Map");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::SliderFloat("##UseAlbedoMap", &prop->usingAlbedoMap, 0.0f, 1.0f);
            
            ImGui::PopItemWidth();
            ImGui::NextColumn();
            
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Use Specular Map");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::SliderFloat("##UseSpecularMap", &prop->usingSpecularMap, 0.0f, 1.0f);
            
            ImGui::PopItemWidth();
            ImGui::NextColumn();
            
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Use Gloss Map");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::SliderFloat("##UseGlossMap", &prop->usingGlossMap, 0.0f, 1.0f);
            
            ImGui::PopItemWidth();
            ImGui::NextColumn();
            
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Use Normal Map");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::SliderFloat("##UseNormalMap", &prop->usingNormalMap, 0.0f, 1.0f);
            
            ImGui::PopItemWidth();
            ImGui::NextColumn();
            
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Albedo");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::ColorEdit4("##Albedo", &prop->albedoColour.x);
            
            ImGui::PopItemWidth();
            ImGui::NextColumn();
            
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Gloss");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::SliderFloat3("##Gloss", &prop->glossColour.x, 0.0f, 1.0f);
            
            ImGui::PopItemWidth();
            ImGui::NextColumn();
            
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Specular");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::SliderFloat3("##Specular", &prop->specularColour.x, 0.0f, 1.0f);
            
            ImGui::PopItemWidth();
            ImGui::NextColumn();

			m_Model->GetMaterial()->SetMaterialProperites(*prop);
            
            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopStyleVar();
            
			ImGui::TreePop();
		}
	}

}
