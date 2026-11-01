#include "GameViewPanel.h"
#include "Editor.h"
#include <Lumos/Graphics/Camera/Camera.h>
#include <Lumos/Core/Application.h>
#include <Lumos/Scene/Scene.h>
#include <Lumos/Core/Engine.h>
#include <Lumos/Core/Profiler.h>
#include <Lumos/Graphics/RHI/Texture.h>
#include <Lumos/Graphics/Renderers/SceneRenderer.h>
#include <Lumos/Graphics/RHI/GraphicsContext.h>
#include <Lumos/Physics/B2PhysicsEngine/B2PhysicsEngine.h>
#include <Lumos/Core/OS/Input.h>
#include <Lumos/Maths/MathsUtilities.h>
#include <Lumos/ImGui/IconsMaterialDesignIcons.h>

#include <box2d/box2d.h>
namespace Lumos
{
    GameViewPanel::GameViewPanel()
    {
        m_Name         = "Game###game";
        m_SimpleName   = "Game";
        m_CurrentScene = nullptr;

        m_Width  = 800;
        m_Height = 600;

        m_SceneRenderer                       = CreateUniquePtr<Graphics::SceneRenderer>(m_Width, m_Height);
        m_SceneRenderer->m_DebugRenderEnabled = false;
        m_SceneRenderer->m_EnableUIPass       = true;
        Graphics::SceneRenderer::SetGameRenderer(m_SceneRenderer.get());
    }

    static std::string AspectToString(float aspect)
    {
        if(Maths::Equals(aspect, 16.0f / 10.0f))
        {
            return "16:10";
        }
        else if(Maths::Equals(aspect, 16.0f / 9.0f))
        {
            return "16:9";
        }
        else if(Maths::Equals(aspect, 4.0f / 3.0f))
        {
            return "4:3";
        }
        else if(Maths::Equals(aspect, 3.0f * 0.5f))
        {
            return "3:2";
        }
        else if(Maths::Equals(aspect, 9.0f / 16.0f))
        {
            return "9:16";
        }
        else
        {
            return "Unsupported";
        }
    }

    static float StringToAspect(const std::string& aspect)
    {
        if(aspect == "16:10")
        {
            return 16.0f / 10.0f;
        }
        else if(aspect == "16:9")
        {
            return 16.0f / 9.0f;
        }
        else if(aspect == "4:3")
        {
            return 4.0f / 3.0f;
        }
        else if(aspect == "3:2")
        {
            return 3.0f * 0.5f;
        }
        else if(aspect == "9:16")
        {
            return 9.0f / 16.0f;
        }
        else
        {
            return 1.0f;
        }
    }

    void GameViewPanel::OnNewScene(Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
        m_CurrentScene = scene;

        // m_SceneRenderer
        m_SceneRenderer->OnNewScene(scene);
        m_SceneRenderer->SetRenderTarget(m_GameViewTexture.get(), true);
        m_SceneRenderer->SetOverrideCamera(nullptr, nullptr);
    }

    Graphics::Texture2D* GameViewPanel::PrepareExternal(uint32_t width, uint32_t height, bool fitEditorCamera)
    {
        if(!m_CurrentScene)
            return nullptr;

        m_SceneRenderer->SetOverrideCamera(nullptr, nullptr);
        auto& registry  = m_CurrentScene->GetRegistry();
        auto cameraView = registry.view<Camera>();
        m_NoCamera      = cameraView.empty();
        if(!cameraView.empty())
        {
            Camera& camera     = registry.get<Camera>(cameraView.front());
            const float aspect = (float)width / (float)Maths::Max(height, 1u);
            if(!Maths::Equals(aspect, camera.GetAspectRatio()))
                camera.SetAspectRatio(aspect);
        }
        else if(Camera* editorCamera = m_Editor->GetCamera())
        {
            // nothing of the scene's to look through yet: the editor's view, so the scene is there to see
            const float aspect = (float)width / (float)Maths::Max(height, 1u);
            if(fitEditorCamera && !Maths::Equals(aspect, editorCamera->GetAspectRatio()))
                editorCamera->SetAspectRatio(aspect);
            m_SceneRenderer->SetOverrideCamera(editorCamera, &m_Editor->GetEditorCameraTransform());
        }

        Resize(width, height);
        return m_GameViewTexture.get();
    }

    void GameViewPanel::Resize(uint32_t width, uint32_t height)
    {
        LUMOS_PROFILE_FUNCTION();
        bool resize = false;

        ASSERT(width > 0 && height > 0, "Game View Dimensions 0");
        Application::Get().SetSceneViewDimensions(width, height);

        if(m_Width != width || m_Height != height || m_AppliedScale != m_ViewScale)
        {
            resize         = true;
            m_Width        = width;
            m_Height       = height;
            m_AppliedScale = m_ViewScale;
        }

        // Render target follows the render scale so it matches the SceneRenderer's
        // internal attachments; displayed image is upscaled to the panel rect.
        // Must match SceneRenderer::OnResize's even-round-then-scale exactly.
        const QualitySettings& qs = Application::Get().GetQualitySettings();
        uint32_t renderWidth      = uint32_t(qs.RendererScale * m_ViewScale * float(m_Width - (m_Width % 2)));
        uint32_t renderHeight     = uint32_t(qs.RendererScale * m_ViewScale * float(m_Height - (m_Height % 2)));

        if(!m_GameViewTexture)
        {
            Graphics::TextureDesc mainRenderTargetDesc;
            mainRenderTargetDesc.format = Graphics::RHIFormat::R8G8B8A8_Unorm;
            mainRenderTargetDesc.flags  = Graphics::TextureFlags::Texture_RenderTarget;

            m_GameViewTexture = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::Create(mainRenderTargetDesc, renderWidth, renderHeight));
        }

        if(resize)
        {
            m_GameViewTexture->Resize(renderWidth, renderHeight);
            m_SceneRenderer->SetViewScale(m_ViewScale);
            m_SceneRenderer->SetRenderTarget(m_GameViewTexture.get(), true, false);
            m_SceneRenderer->OnResize(width, height);
        }
    }

    void GameViewPanel::OnRender()
    {
        if(((m_GameViewVisible && m_Active) || m_ExternalVisible) && m_CurrentScene)
        {
            m_SceneRenderer->BeginScene(m_CurrentScene);
            m_SceneRenderer->OnRender();
        }
    }

}
