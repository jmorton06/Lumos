#include "LM.h"
#include "MeshComponent.h"
#include "Graphics/Mesh.h"
#include "Maths/BoundingSphere.h"
#include "Physics3DComponent.h"
#include "Physics/LumosPhysicsEngine/PhysicsObject3D.h"
#include "Entity/Entity.h"
#include "Graphics/Material.h"
#include "Graphics/API/Textures/Texture2D.h"
#include "Graphics/API/GraphicsContext.h"

#include <imgui/imgui.h>


namespace lumos
{
	MeshComponent::MeshComponent(std::shared_ptr<graphics::Mesh>& model)
		: m_Model(model)
	{
		m_BoundingShape = std::make_unique<maths::BoundingSphere>(maths::Vector3(0.0f),1.0f);
	}

	MeshComponent::MeshComponent(graphics::Mesh* mesh)
	{
		m_Model = std::shared_ptr<graphics::Mesh>(mesh);
		m_BoundingShape = std::make_unique<maths::BoundingSphere>(maths::Vector3(0.0f), 1.0f);
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
        if (m_Model->GetMaterial() && ImGui::CollapsingHeader("Material"))
		{
            bool flipImage = graphics::GraphicsContext::GetContext()->FlipImGUITexture();
            
			MaterialProperties* prop = m_Model->GetMaterial()->GetProperties();

            if (ImGui::CollapsingHeader("Albedo"))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2,2));
                ImGui::Columns(2);
                ImGui::Separator();
                
                ImGui::AlignTextToFramePadding();
                auto tex = m_Model->GetMaterial()->GetTextures().albedo;
                ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(64, 64), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
                
                if (ImGui::IsItemHovered() && tex)
                {
                    ImGui::BeginTooltip();
                    ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
                    ImGui::EndTooltip();
                }
                
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                ImGui::Text("%s", tex ? tex->GetFilepath().c_str() : "No Texture");
                
                ImGui::PopItemWidth();
                ImGui::NextColumn();
                
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Use Albedo Map");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                ImGui::SliderFloat("##UseAlbedoMap", &prop->usingAlbedoMap, 0.0f, 1.0f);
                
                ImGui::PopItemWidth();
                ImGui::NextColumn();
                
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Albedo");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                ImGui::ColorEdit4("##Albedo", &prop->albedoColour.x);
                
                ImGui::PopItemWidth();
                ImGui::NextColumn();
                
                ImGui::Columns(1);
                ImGui::Separator();
                ImGui::PopStyleVar();
            }
            
            if (ImGui::CollapsingHeader("Normal"))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2,2));
                ImGui::Columns(2);
                ImGui::Separator();
                
                ImGui::AlignTextToFramePadding();
                auto tex = m_Model->GetMaterial()->GetTextures().normal;
                ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(64, 64), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
                
                if (ImGui::IsItemHovered() && tex)
                {
                    ImGui::BeginTooltip();
                    ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
                    ImGui::EndTooltip();
                }
                
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                ImGui::Text("%s", tex ? tex->GetFilepath().c_str() : "No Texture");
                
                ImGui::PopItemWidth();
                ImGui::NextColumn();
                
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Use Normal Map");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                ImGui::SliderFloat("##UseNormalMap", &prop->usingNormalMap, 0.0f, 1.0f);
                
                ImGui::PopItemWidth();
                ImGui::NextColumn();
                
                ImGui::Columns(1);
                ImGui::Separator();
                ImGui::PopStyleVar();
            }
            
            if (ImGui::CollapsingHeader("Specular"))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2,2));
                ImGui::Columns(2);
                ImGui::Separator();
                
                ImGui::AlignTextToFramePadding();
                auto tex = m_Model->GetMaterial()->GetTextures().specular;
                ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(64, 64), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
                
                if (ImGui::IsItemHovered() && tex)
                {
                    ImGui::BeginTooltip();
                    ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
                    ImGui::EndTooltip();
                }
                
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                ImGui::Text("%s", tex ? tex->GetFilepath().c_str() : "No Texture");
                
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
                ImGui::Text("Specular");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                ImGui::SliderFloat3("##Specular", &prop->specularColour.x, 0.0f, 1.0f);
                
                ImGui::PopItemWidth();
                ImGui::NextColumn();
                
                ImGui::Columns(1);
                ImGui::Separator();
                ImGui::PopStyleVar();
            }
            
            if (ImGui::CollapsingHeader("Roughness"))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2,2));
                ImGui::Columns(2);
                ImGui::Separator();
                
                ImGui::AlignTextToFramePadding();
                auto tex = m_Model->GetMaterial()->GetTextures().roughness;
                ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(64, 64), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
                
                if (ImGui::IsItemHovered() && tex)
                {
                    ImGui::BeginTooltip();
                    ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
                    ImGui::EndTooltip();
                }
                
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                ImGui::Text("%s", tex ? tex->GetFilepath().c_str() : "No Texture");
                
                ImGui::PopItemWidth();
                ImGui::NextColumn();
           
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Use Roughness Map");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                ImGui::SliderFloat("##UseRoughnessMap", &prop->usingRoughnessMap, 0.0f, 1.0f);
                
                ImGui::PopItemWidth();
                ImGui::NextColumn();
                
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Roughness");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                ImGui::SliderFloat3("##Roughness", &prop->roughnessColour.x, 0.0f, 1.0f);
                
                ImGui::PopItemWidth();
                ImGui::NextColumn();
                
                ImGui::Columns(1);
                ImGui::Separator();
                ImGui::PopStyleVar();
            }
            
            if (ImGui::CollapsingHeader("Ao"))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2,2));
                ImGui::Columns(2);
                ImGui::Separator();
                
                ImGui::AlignTextToFramePadding();
                auto tex = m_Model->GetMaterial()->GetTextures().ao;
                ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(64, 64), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
                
                if (ImGui::IsItemHovered() && tex)
                {
                    ImGui::BeginTooltip();
                    ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
                    ImGui::EndTooltip();
                }
                
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                ImGui::Text("%s", tex ? tex->GetFilepath().c_str() : "No Texture");
                
                ImGui::PopItemWidth();
                ImGui::NextColumn();
                
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Use AO Map");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                ImGui::SliderFloat("##UseAOMap", &prop->usingAOMap, 0.0f, 1.0f);
                
                ImGui::PopItemWidth();
                ImGui::NextColumn();
                
                ImGui::Columns(1);
                ImGui::Separator();
                ImGui::PopStyleVar();
            }

			if (ImGui::CollapsingHeader("Emissive"))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
				ImGui::Columns(2);
				ImGui::Separator();

				ImGui::AlignTextToFramePadding();
				auto tex = m_Model->GetMaterial()->GetTextures().emissive;
				ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(64, 64), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));

				if (ImGui::IsItemHovered() && tex)
				{
					ImGui::BeginTooltip();
					ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
					ImGui::EndTooltip();
				}

				ImGui::NextColumn();
				ImGui::PushItemWidth(-1);
				ImGui::Text("%s", tex ? tex->GetFilepath().c_str() : "No Texture");

				ImGui::PopItemWidth();
				ImGui::NextColumn();

				ImGui::AlignTextToFramePadding();
				ImGui::Text("Use Emissive Map");
				ImGui::NextColumn();
				ImGui::PushItemWidth(-1);
				ImGui::SliderFloat("##UseEmissiveMap", &prop->usingEmissiveMap, 0.0f, 1.0f);

				ImGui::PopItemWidth();
				ImGui::NextColumn();

				ImGui::Columns(1);
				ImGui::Separator();
				ImGui::PopStyleVar();
			}
            
            m_Model->GetMaterial()->SetMaterialProperites(*prop);
		}
	}

}
