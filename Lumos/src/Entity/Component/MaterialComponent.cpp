#include "LM.h"
#include "MaterialComponent.h"
#include "Graphics/Material.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/API/Textures/Texture2D.h"

#include <imgui/imgui.h>
#include <imgui/plugins/imfilebrowser.h>

namespace Lumos
{
    MaterialComponent::MaterialComponent()
    {
        m_Name = "Material";
        m_Material = std::make_shared<Material>();
    }
    
    MaterialComponent::MaterialComponent(std::shared_ptr<Material>& material)
    : m_Material(material)
    {
        ImGuiFileBrowserFlags flags = ImGuiFileBrowserFlags_CreateNewDir | ImGuiFileBrowserFlags_SelectDirectory;
        m_FileDialog = new ImGui::FileBrowser(flags);
    }
    
    MaterialComponent::~MaterialComponent()
    {
        delete m_FileDialog;
    }
    
    void MaterialComponent::OnUpdateComponent(float dt)
    {
    }
    
    void MaterialComponent::OnIMGUI()
    {
		ImGui::Separator();

		if (m_Material)
		{
			bool flipImage = Graphics::GraphicsContext::GetContext()->FlipImGUITexture();

			MaterialProperties* prop = m_Material->GetProperties();

			if (ImGui::TreeNode("Albedo"))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
				ImGui::Columns(2);
				ImGui::Separator();

				ImGui::AlignTextToFramePadding();
				auto tex = m_Material->GetTextures().albedo;

				if (tex)
				{
					ImGui::Image(tex->GetHandle(), ImVec2(64, 64), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));

					if (ImGui::IsItemHovered() && tex)
					{
						ImGui::BeginTooltip();
						ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
						ImGui::EndTooltip();
					}
				}
				else
				{
					if(ImGui::Button("Empty", ImVec2(64, 64)))
                    {
                        m_FileDialog->Open();
                    }
                    
                    m_FileDialog->Display();
                    
                    if(m_FileDialog->HasSelected())
                    {
                        LUMOS_CORE_INFO("Selected filename {0}", m_FileDialog->GetSelected().string().c_str());
                        m_FileDialog->ClearSelected();
                    }
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

				ImGui::TreePop();
			}

			ImGui::Separator();

			if (ImGui::TreeNode("Normal"))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
				ImGui::Columns(2);
				ImGui::Separator();

				ImGui::AlignTextToFramePadding();
				auto tex = m_Material->GetTextures().normal;

				if (tex)
				{
					ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(64, 64), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));

					if (ImGui::IsItemHovered() && tex)
					{
						ImGui::BeginTooltip();
						ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
						ImGui::EndTooltip();
					}
				}
				else
				{
					ImGui::Button("Empty", ImVec2(64, 64));
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
				ImGui::TreePop();
			}

			ImGui::Separator();

			if (ImGui::TreeNode("Specular"))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
				ImGui::Columns(2);
				ImGui::Separator();

				ImGui::AlignTextToFramePadding();
				auto tex = m_Material->GetTextures().specular;

				if (tex)
				{
					ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(64, 64), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));

					if (ImGui::IsItemHovered() && tex)
					{
						ImGui::BeginTooltip();
						ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
						ImGui::EndTooltip();
					}
				}
				else
				{
					ImGui::Button("Empty", ImVec2(64, 64));
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
				ImGui::TreePop();
			}

			ImGui::Separator();

			if (ImGui::TreeNode("Roughness"))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
				ImGui::Columns(2);
				ImGui::Separator();

				ImGui::AlignTextToFramePadding();
				auto tex = m_Material->GetTextures().roughness;
				if (tex)
				{
					ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(64, 64), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));

					if (ImGui::IsItemHovered() && tex)
					{
						ImGui::BeginTooltip();
						ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
						ImGui::EndTooltip();
					}
				}
				else
				{
					ImGui::Button("Empty", ImVec2(64, 64));
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
				ImGui::TreePop();
			}

			ImGui::Separator();

			if (ImGui::TreeNode("Ao"))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
				ImGui::Columns(2);
				ImGui::Separator();

				ImGui::AlignTextToFramePadding();
				auto tex = m_Material->GetTextures().ao;
				if (tex)
				{
					ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(64, 64), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));

					if (ImGui::IsItemHovered() && tex)
					{
						ImGui::BeginTooltip();
						ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
						ImGui::EndTooltip();
					}
				}
				else
				{
					ImGui::Button("Empty", ImVec2(64, 64));
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
				ImGui::TreePop();
			}

			ImGui::Separator();

			if (ImGui::TreeNode("Emissive"))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
				ImGui::Columns(2);
				ImGui::Separator();

				ImGui::AlignTextToFramePadding();
				auto tex = m_Material->GetTextures().emissive;
				if (tex)
				{
					ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(64, 64), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));

					if (ImGui::IsItemHovered() && tex)
					{
						ImGui::BeginTooltip();
						ImGui::Image(tex ? tex->GetHandle() : nullptr, ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
						ImGui::EndTooltip();
					}
				}
				else
				{
					ImGui::Button("Empty", ImVec2(64, 64));
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
				ImGui::TreePop();
			}

			ImGui::Separator();

			m_Material->SetMaterialProperites(*prop);
		}
    }
}
