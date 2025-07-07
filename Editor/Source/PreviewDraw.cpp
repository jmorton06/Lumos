#include "PreviewDraw.h"
#include <Lumos/Core/Asset/AssetManager.h>
#include <Lumos/Graphics/RHI/Renderer.h>
#include <Lumos/Graphics/Renderers/SceneRenderer.h>
#include <Lumos/Graphics/Model.h>
#include <Lumos/Graphics/Mesh.h>
#include <Lumos/Scene/Component/ModelComponent.h>
#include <Lumos/Scene/EntityManager.h>
#include <Lumos/Scene/Scene.h>
#include <Lumos/Graphics/Light.h>
#include <Lumos/Graphics/Environment.h>
#include <Lumos/Maths/BoundingBox.h>
#include <Lumos/Maths/MathsUtilities.h>

namespace Lumos
{
    PreviewDraw::PreviewDraw() = default;

    PreviewDraw::~PreviewDraw()
    {
        if(m_PreviewScene)
        {
            m_PreviewScene->OnCleanupScene();
            delete m_PreviewScene;
        }
    }

    void PreviewDraw::CreateDefaultScene()
    {
        if(m_PreviewScene)
            return;

        if(!m_PreviewTexture || m_Width != m_PreviewTexture->GetWidth() || m_Height != m_PreviewTexture->GetHeight())
        {
            Graphics::TextureDesc desc;
            desc.format = Graphics::RHIFormat::R8G8B8A8_Unorm;
            desc.flags  = Graphics::TextureFlags::Texture_RenderTarget;

            m_PreviewTexture = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::Create(desc, m_Width, m_Height));
        }

        if(!m_PreviewRenderer)
        {
            m_PreviewRenderer                       = CreateSharedPtr<Graphics::SceneRenderer>(m_Width, m_Height);
            m_PreviewRenderer->m_DebugRenderEnabled = false;
        }

        m_PreviewScene                            = new Scene("Preview");
        auto& sceneSettings                       = m_PreviewScene->GetSettings();
        sceneSettings.RenderSettings.MSAASamples  = 1;
        sceneSettings.RenderSettings.BloomEnabled = false;
        {
            auto light      = m_PreviewScene->GetEntityManager()->Create("Light");
            auto& lightComp = light.AddComponent<Graphics::Light>();
            Mat4 lightView  = Mat4::LookAt(Vec3(30.0f, 9.0f, 50.0f), Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f)).Inverse();
            light.GetTransform().SetLocalTransform(lightView);
            light.GetTransform().SetWorldMatrix(Mat4(1.0f));

            m_CameraEntity = m_PreviewScene->GetEntityManager()->Create("Camera");
            m_CameraEntity.AddComponent<Camera>();
            m_CameraEntity.GetComponent<Camera>().SetFar(10000);
            Mat4 viewMat = Mat4::LookAt(Vec3(-1.0f, 0.5f, 1.0f), Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f)).Inverse();
            m_CameraEntity.GetTransform().SetLocalTransform(viewMat);
            m_CameraEntity.GetTransform().SetWorldMatrix(Mat4(1.0f));

            auto environment = m_PreviewScene->GetEntityManager()->Create("Environment");
            environment.AddComponent<Graphics::Environment>();
            environment.GetComponent<Graphics::Environment>().Load();
        }

        m_PreviewRenderer->OnNewScene(m_PreviewScene);
        m_PreviewRenderer->SetRenderTarget(m_PreviewTexture.get(), true);
    }

    void PreviewDraw::Draw()
    {
        if(!m_PreviewRenderer)
            return;

        m_PreviewRenderer->SetOverrideCamera(nullptr, nullptr);
        m_PreviewRenderer->BeginScene(m_PreviewScene);
        m_PreviewRenderer->OnRender();
    }

    void PreviewDraw::SaveTexture(String8 savePath, bool Blur, float BlurRadius)
    {
        LINFO("Saving Screenshot, %s", (char*)savePath.str);
        Graphics::Renderer::GetRenderer()->SaveScreenshot(ToStdString(savePath), m_PreviewTexture.get(), Blur, BlurRadius);
    }

    void PreviewDraw::LoadMesh(String8 path)
    {
        if(m_PreviewObjectEntity)
            m_PreviewObjectEntity.Destroy();

        m_PreviewObjectEntity = m_PreviewScene->GetEntityManager()->Create("Model");

        if(Application::Get().GetAssetManager()->AssetExists(path))
        {
            m_PreviewObjectEntity.AddComponent<Graphics::ModelComponent>(Application::Get().GetAssetManager()->GetAssetData(path).As<Graphics::Model>());
        }
        else
        {
            m_PreviewObjectEntity.AddComponent<Graphics::ModelComponent>(ToStdString(path));
        }

        Mat4 viewMat = Mat4::LookAt(Vec3(-1.0f, 0.5f, 1.0f), Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f)).Inverse();
        m_CameraEntity.GetTransform().SetLocalTransform(viewMat);
        m_CameraEntity.GetTransform().SetWorldMatrix(Mat4(1.0f));

        auto bb = m_PreviewObjectEntity.GetComponent<Graphics::ModelComponent>().ModelRef->GetMeshes().Front()->GetBoundingBox();
        viewMat = Mat4::LookAt(-(m_CameraEntity.GetTransform().GetForwardDirection()) * Maths::Distance(bb.Max(), bb.Min()), bb.Center(), Vec3(0.0f, 1.0f, 0.0f)).Inverse();
        m_CameraEntity.GetTransform().SetLocalTransform(viewMat);
        m_CameraEntity.GetTransform().SetWorldMatrix(Mat4(1.0f));
    }

    void PreviewDraw::LoadMaterial(String8 path)
    {
        if(m_PreviewObjectEntity)
            m_PreviewObjectEntity.Destroy();

        m_PreviewObjectEntity = m_PreviewScene->GetEntityManager()->Create("Sphere");
        m_PreviewObjectEntity.AddComponent<Graphics::ModelComponent>(Graphics::PrimitiveType::Sphere);
        m_PreviewObjectEntity.GetComponent<Graphics::ModelComponent>().ModelRef->GetMeshes()[0]->SetAndLoadMaterial(ToStdString(path));

        Mat4 viewMat = Mat4::LookAt(Vec3(-1.0f, 0.5f, 1.0f), Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f)).Inverse();
        m_CameraEntity.GetTransform().SetLocalTransform(viewMat);
        m_CameraEntity.GetTransform().SetWorldMatrix(Mat4(1.0f));
    }

    void PreviewDraw::DeletePreviewModel()
    {
        if(m_PreviewObjectEntity)
            m_PreviewObjectEntity.Destroy();

        m_PreviewObjectEntity = {};
    }

    void PreviewDraw::SetDimensions(u32 width, u32 height)
    {
        m_Width  = width;
        m_Height = height;
    }

    void PreviewDraw::ReleaseResources()
    {
        if(m_PreviewScene)
        {
            m_PreviewScene->OnCleanupScene();
            delete m_PreviewScene;
        }
        m_PreviewScene = nullptr;

        m_PreviewTexture.reset();
        m_PreviewRenderer.reset();
    }
};
