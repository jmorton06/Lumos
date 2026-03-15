#include "Editor.h"
#include "ImportPanel.h"
#include <Lumos/Core/Asset/AssetImporter.h>
#include <Lumos/Core/Application.h>
#include <Lumos/Utilities/StringUtilities.h>
#include <Lumos/Scene/Component/ModelComponent.h>
#include <Lumos/Scene/EntityManager.h>
#include <Lumos/Graphics/Model.h>
#include <Lumos/Graphics/Mesh.h>
#include <Lumos/Scene/Entity.h>
#include <imgui/imgui.h>

namespace Lumos
{
    void ImportPanel::Show(const std::string& sourcePath)
    {
        m_SourcePath = sourcePath;
        m_Settings   = ImportSettings();
        m_Active     = true;

        // Load existing .meta if available
        ImportMeta meta;
        if(AssetImporter::LoadMeta(sourcePath, meta))
            m_Settings = meta.Settings;
    }

    void ImportPanel::OnImGui()
    {
        if(!m_Active)
            return;

        ImGui::OpenPopup("Import Asset");

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(400, 440), ImGuiCond_Appearing);

        if(ImGui::BeginPopupModal("Import Asset", &m_Active, ImGuiWindowFlags_NoResize))
        {
            std::string filename = StringUtilities::GetFileName(m_SourcePath);
            ImGui::Text("Import: %s", filename.c_str());
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Checkbox("Import Materials", &m_Settings.ImportMaterials);
            ImGui::Checkbox("Import Animations", &m_Settings.ImportAnimations);
            ImGui::Checkbox("Copy Source to Project", &m_Settings.CopySourceToProject);
            ImGui::Checkbox("Split Meshes", &m_Settings.SplitMeshes);

            ImGui::Spacing();
            ImGui::Checkbox("Generate Collision Mesh", &m_Settings.GenerateCollision);

            if(m_Settings.GenerateCollision)
            {
                ImGui::Indent();
                ImGui::SliderFloat("Simplify Ratio", &m_Settings.CollisionSimplifyRatio, 0.01f, 1.0f, "%.2f");
                ImGui::Unindent();
            }

            ImGui::Spacing();
            ImGui::Text("Mesh Optimization:");
            ImGui::Checkbox("Vertex Cache", &m_Settings.OptVertexCache);
            ImGui::SameLine();
            ImGui::Checkbox("Overdraw", &m_Settings.OptOverdraw);
            ImGui::Checkbox("Vertex Fetch", &m_Settings.OptVertexFetch);

            ImGui::Spacing();
            int maxW = (int)m_Settings.MaxTextureWidth;
            int maxH = (int)m_Settings.MaxTextureHeight;
            ImGui::Text("Max Texture Size:");
            ImGui::SetNextItemWidth(120);
            if(ImGui::InputInt("##MaxW", &maxW, 0, 0))
                m_Settings.MaxTextureWidth = (u32)(maxW > 0 ? maxW : 0);
            ImGui::SameLine();
            ImGui::Text("x");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120);
            if(ImGui::InputInt("##MaxH", &maxH, 0, 0))
                m_Settings.MaxTextureHeight = (u32)(maxH > 0 ? maxH : 0);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            float buttonWidth = 100.0f;
            float spacing     = ImGui::GetStyle().ItemSpacing.x;
            float totalWidth  = buttonWidth * 2 + spacing;
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - totalWidth) * 0.5f);

            if(ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
            {
                m_Active = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if(ImGui::Button("Import", ImVec2(buttonWidth, 0)))
            {
                std::string importedPath = AssetImporter::Import(m_SourcePath, m_Settings);

                if(!importedPath.empty())
                {
                    if(!Graphics::ModelComponent::ReloadSceneModels(m_SourcePath))
                    {
                        auto* scene = Application::Get().GetSceneManager()->GetCurrentScene();
                        std::string entityName = StringUtilities::GetFileName(m_SourcePath);

                        if(m_Settings.SplitMeshes)
                        {
                            // Load the model to get mesh count
                            Graphics::Model tempModel(importedPath);
                            auto& meshes = tempModel.GetMeshes();

                            if(meshes.Size() > 1)
                            {
                                // Create parent entity
                                Entity parentEntity = scene->GetEntityManager()->Create(entityName);

                                for(int i = 0; i < meshes.Size(); i++)
                                {
                                    auto& mesh = meshes[i];
                                    std::string meshName = mesh->GetName();
                                    if(meshName.empty())
                                    {
                                        char buf[64];
                                        snprintf(buf, sizeof(buf), "%s_%d", entityName.c_str(), i);
                                        meshName = buf;
                                    }

                                    Entity childEntity = scene->GetEntityManager()->Create(meshName);

                                    // Create a single-mesh model wrapping this mesh
                                    auto singleModel = CreateSharedPtr<Graphics::Model>();
                                    singleModel->AddMesh(mesh);

                                    childEntity.AddComponent<Graphics::ModelComponent>(singleModel);
                                    childEntity.SetParent(parentEntity);
                                }
                            }
                            else
                            {
                                Entity modelEntity = scene->GetEntityManager()->Create(entityName);
                                modelEntity.AddComponent<Graphics::ModelComponent>(m_SourcePath);
                            }
                        }
                        else
                        {
                            Entity modelEntity = scene->GetEntityManager()->Create(entityName);
                            modelEntity.AddComponent<Graphics::ModelComponent>(m_SourcePath);
                        }
                    }
                }

                m_Active = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
}
