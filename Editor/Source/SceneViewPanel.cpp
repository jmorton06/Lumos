#include "SceneViewPanel.h"
#include "Editor.h"
#include <Lumos/Graphics/Camera/Camera.h>
#include <Lumos/Core/Application.h>
#include <Lumos/Scene/SceneManager.h>
#include <Lumos/Scene/EntityFactory.h>
#include <Lumos/Core/Engine.h>
#include <Lumos/Core/Profiler.h>
#include <Lumos/Graphics/RHI/GraphicsContext.h>
#include <Lumos/Graphics/RHI/Texture.h>
#include <Lumos/Graphics/RHI/SwapChain.h>
#include <Lumos/Graphics/Renderers/SceneRenderer.h>
#include <Lumos/Graphics/Light.h>
#include <Lumos/Graphics/Model.h>
#include <Lumos/Scene/Component/SoundComponent.h>
#include <Lumos/Scene/Component/ModelComponent.h>
#include <Lumos/Scene/Component/VoxelWorldComponent.h>
#include <Lumos/Scripting/Lua/LuaScriptComponent.h>
#include <Lumos/Graphics/Renderers/GridRenderer.h>
#include <Lumos/Physics/LumosPhysicsEngine/LumosPhysicsEngine.h>
#include <Lumos/Physics/B2PhysicsEngine/B2PhysicsEngine.h>
#include <Lumos/Core/OS/Input.h>
#include <Lumos/Scene/Scene.h>
#include <Lumos/Graphics/Renderers/DebugRenderer.h>
#include <Lumos/ImGui/IconsMaterialDesignIcons.h>
#include <Lumos/Graphics/Camera/EditorCamera.h>
#include <Lumos/Events/ApplicationEvent.h>
#include <Lumos/Graphics/Sprite.h>
#include <Lumos/Maths/Ray.h>

#include "TerrainSculpt.h"

#include <box2d/box2d.h>
#include <sol/sol.hpp>

namespace Lumos
{
    SceneViewPanel::SceneViewPanel()
    {
        m_Name         = "Scene###scene";
        m_SimpleName   = "Scene";
        m_CurrentScene = nullptr;

        m_Width  = 800;
        m_Height = 600;

        Application::Get().GetSceneRenderer()->SetDisablePostProcess(false);
    }

    void SceneViewPanel::OnNewScene(Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
        m_Editor->GetSettings().m_AspectRatio = 1.0f;
        m_CurrentScene                        = scene;

        auto SceneRenderer = Application::Get().GetSceneRenderer();
        SceneRenderer->SetRenderTarget(m_GameViewTexture.get(), true);
        SceneRenderer->SetOverrideCamera(m_Editor->GetCamera(), &m_Editor->GetEditorCameraTransform());
        m_Editor->GetGridRenderer()->SetRenderTarget(m_GameViewTexture.get(), true);
        m_Editor->GetGridRenderer()->SetDepthTarget(SceneRenderer->GetForwardData().m_DepthTexture);
    }

    uint32_t SceneViewPanel::GetRenderWidth() const { return m_GameViewTexture ? m_GameViewTexture->GetWidth() : 0; }
    uint32_t SceneViewPanel::GetRenderHeight() const { return m_GameViewTexture ? m_GameViewTexture->GetHeight() : 0; }

    void SceneViewPanel::Resize(uint32_t width, uint32_t height)
    {
        LUMOS_PROFILE_FUNCTION();
        bool resize = false;

        ASSERT(width > 0 && height > 0, "Scene View Dimensions 0");

        if(m_Width != width || m_Height != height || m_AppliedScale != m_ViewScale)
        {
            resize         = true;
            m_Width        = width;
            m_Height       = height;
            m_AppliedScale = m_ViewScale;
        }

        // Render target follows the render scale so it matches SceneRenderer's
        // internal attachments (depth etc.); ImGui upscales it to the panel rect.
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

            auto SceneRenderer = Application::Get().GetSceneRenderer();
            SceneRenderer->SetRenderTarget(m_GameViewTexture.get(), true, false);
            SceneRenderer->SetViewScale(m_ViewScale);

            if(!m_Editor->GetGridRenderer())
                m_Editor->CreateGridRenderer();
            m_Editor->GetGridRenderer()->SetViewScale(m_ViewScale);
            m_Editor->GetGridRenderer()->SetRenderTarget(m_GameViewTexture.get(), false);
            m_Editor->GetGridRenderer()->SetDepthTarget(SceneRenderer->GetForwardData().m_DepthTexture);

            WindowResizeEvent e(width, height);
            auto& app = Application::Get();
            app.GetSceneRenderer()->OnResize(width, height);

            SceneRenderer->OnEvent(e);

            m_Editor->GetGridRenderer()->OnResize(m_Width, m_Height);

            // Should be just build texture and scene renderer set render target
            // Renderer::GetGraphicsContext()->WaitIdle();
        }
    }

}
