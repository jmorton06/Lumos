#include "Editor.h"
#include "ImportPanel.h"
#include <Lumos/Core/Asset/AssetImporter.h>
#include <Lumos/Core/Application.h>
#include <Lumos/Core/BusyOverlay.h>
#include <Lumos/Utilities/StringUtilities.h>
#include <Lumos/Scene/Component/ModelComponent.h>
#include <Lumos/Scene/EntityManager.h>
#include <Lumos/Graphics/Model.h>
#include <Lumos/Graphics/Mesh.h>
#include <Lumos/Scene/Entity.h>
#include <Lumos/Graphics/UI.h>
#include <Lumos/Graphics/UIDock.h>
#include <Lumos/Graphics/UITheme.h>

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

    void ImportPanel::Import()
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
    }

    void ImportPanel::OnEditorUI(const UIDockPanelContext& context)
    {
        if(!m_Active)
        {
            UIPushStyle(StyleVar_TextColor, UIGetToolTheme().TextDim);
            UILabelCStr("##importnone", "Open a model from Assets to import it.");
            UIPopStyle(StyleVar_TextColor);
            return;
        }

        std::string filename = StringUtilities::GetFileName(m_SourcePath);
        UILabelCStr("##importfile", filename.c_str());
        UISpacer(4.0f);

        if(UISectionHeader("Model###importmodel", true))
        {
            UIBeginPropertyGrid("##importmodelgrid", 0.55f);
            UIPropertyBool("Import Materials", &m_Settings.ImportMaterials);
            UIPropertyBool("Import Animations", &m_Settings.ImportAnimations);
            UIPropertyBool("Copy Source to Project", &m_Settings.CopySourceToProject);
            UIPropertyBool("Split Meshes", &m_Settings.SplitMeshes);
            UIPropertyBool("Generate Collision Mesh", &m_Settings.GenerateCollision);
            if(m_Settings.GenerateCollision)
                UIPropertyFloat("Simplify Ratio", &m_Settings.CollisionSimplifyRatio, 0.005f, 0.01f, 1.0f, "%.2f");
            UIEndPropertyGrid();
        }

        UISpacer(4.0f);
        if(UISectionHeader("Optimisation###importopt", true))
        {
            UIBeginPropertyGrid("##importoptgrid", 0.55f);
            UIPropertyBool("Vertex Cache", &m_Settings.OptVertexCache);
            UIPropertyBool("Overdraw", &m_Settings.OptOverdraw);
            UIPropertyBool("Vertex Fetch", &m_Settings.OptVertexFetch);
            // 0 = keep the source size
            int maxW = (int)m_Settings.MaxTextureWidth;
            int maxH = (int)m_Settings.MaxTextureHeight;
            if(UIPropertyInt("Max Texture Width", &maxW, 8.0f, 0, 16384))
                m_Settings.MaxTextureWidth = (u32)maxW;
            if(UIPropertyInt("Max Texture Height", &maxH, 8.0f, 0, 16384))
                m_Settings.MaxTextureHeight = (u32)maxH;
            UIEndPropertyGrid();
        }

        UISpacer(8.0f);
        UIBeginRowFullWidth();
        const bool import = UIButton("Import###importgo").clicked;
        UISpacer(6.0f);
        const bool cancel = UIButton("Cancel###importcancel").clicked;
        UIEndRow();

        if(import)
        {
            // outside the frame, so the loading overlay can paint while it works
            Busy::Run("Importing", [this]() { Import(); });
            m_Active = false;
        }
        else if(cancel)
            m_Active = false;
    }
}
