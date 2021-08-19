#include "Precompiled.h"
#include "DeferredRenderer.h"
#include "DeferredOffScreenRenderer.h"
#include "ShadowRenderer.h"

#include "Scene/Scene.h"
#include "Core/Application.h"
#include "Maths/Maths.h"
#include "Maths/Transform.h"

#include "RenderGraph.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Mesh.h"
#include "Graphics/MeshFactory.h"
#include "Graphics/Material.h"
#include "Graphics/GBuffer.h"
#include "Graphics/Light.h"

#include "Graphics/RHI/Shader.h"
#include "Graphics/RHI/Framebuffer.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/RHI/UniformBuffer.h"
#include "Graphics/RHI/Renderer.h"
#include "Graphics/RHI/CommandBuffer.h"
#include "Graphics/RHI/SwapChain.h"
#include "Graphics/RHI/RenderPass.h"
#include "Graphics/RHI/Pipeline.h"
#include "Graphics/RHI/GraphicsContext.h"
#include "Graphics/Environment.h"
#include "Embedded/BRDFTexture.inl"
#include "Utilities/AssetManager.h"

#include "CompiledSPV/Headers/DeferredLightvertspv.hpp"
#include "CompiledSPV/Headers/DeferredLightfragspv.hpp"

#include <imgui/imgui.h>

#define MAX_LIGHTS 32
#define MAX_SHADOWMAPS 4

namespace Lumos
{
    namespace Graphics
    {
        DeferredRenderer::DeferredRenderer(uint32_t width, uint32_t height)
        {
            DeferredRenderer::SetScreenBufferSize(width, height);
            DeferredRenderer::Init();
        }

        DeferredRenderer::~DeferredRenderer()
        {
            delete m_ScreenQuad;
            delete m_OffScreenRenderer;
            delete m_DeferredCommandBuffers;
        }

        void DeferredRenderer::Init()
        {
            LUMOS_PROFILE_FUNCTION();
            m_OffScreenRenderer = new DeferredOffScreenRenderer(m_ScreenBufferWidth, m_ScreenBufferHeight);

            m_Shader = Graphics::Shader::CreateFromEmbeddedArray(spirv_DeferredLightvertspv.data(), spirv_DeferredLightvertspv_size, spirv_DeferredLightfragspv.data(), spirv_DeferredLightfragspv_size);

            switch(Graphics::GraphicsContext::GetRenderAPI())
            {
#ifdef LUMOS_RENDER_API_OPENGL
            case Graphics::RenderAPI::OPENGL:
                m_BiasMatrix = Maths::Matrix4(0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f);
                break;
#endif

#ifdef LUMOS_RENDER_API_VULKAN
            case Graphics::RenderAPI::VULKAN:
                m_BiasMatrix = Maths::Matrix4(0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
                break;
#endif

#ifdef LUMOS_RENDER_API_DIRECT3D
            case Graphics::RenderAPI::DIRECT3D:
                m_BiasMatrix = Maths::Matrix4(0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
                break;
#endif
            default:
                break;
            }

            TextureParameters param;
            param.minFilter = TextureFilter::LINEAR;
            param.magFilter = TextureFilter::LINEAR;
            param.format = TextureFormat::RGBA8;
            param.srgb = false;
            param.wrap = TextureWrap::CLAMP_TO_EDGE;
            m_PreintegratedFG = UniquePtr<Texture2D>(Texture2D::CreateFromSource(BRDFTextureWidth, BRDFTextureHeight, (void*)BRDFTexture, param));

            m_ScreenQuad = Graphics::CreateScreenQuad();

            AttachmentInfo textureTypes[2] = {
                { TextureType::COLOUR, TextureFormat::RGBA8 }
            };
            Graphics::RenderPassDesc renderPassDesc {};
            renderPassDesc.attachmentCount = 1;
            renderPassDesc.textureType = textureTypes;
            renderPassDesc.clear = true;

            m_RenderPass = Graphics::RenderPass::Get(renderPassDesc);

            m_DeferredCommandBuffers = Graphics::CommandBuffer::Create();
            m_DeferredCommandBuffers->Init(true);

            Graphics::DescriptorDesc descriptorDesc {};
            descriptorDesc.layoutIndex = 0;
            descriptorDesc.shader = m_Shader.get();
            m_DescriptorSet.resize(2);
            m_DescriptorSet[0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
            descriptorDesc.layoutIndex = 1;
            m_DescriptorSet[1] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

            CreateDeferredPipeline();
            CreateFramebuffers();

            m_ClearColour = Maths::Vector4(0.2f, 0.2f, 0.2f, 1.0f);

            UpdateScreenDescriptorSet();

            m_CurrentDescriptorSets.resize(2);
        }

        void DeferredRenderer::RenderScene()
        {
            LUMOS_PROFILE_FUNCTION();

            int commandBufferIndex = 0;
            if(!m_RenderTexture)
                commandBufferIndex = Renderer::GetSwapChain()->GetCurrentBufferIndex();

            m_OffScreenRenderer->RenderScene();
            Begin(commandBufferIndex);
            Present();
            End();
        }

        void DeferredRenderer::PresentToScreen()
        {
            LUMOS_PROFILE_FUNCTION();
            //Renderer::Present(Renderer::GetSwapChain()->GetCurrentCommandBuffer());
        }

        void DeferredRenderer::Begin(int commandBufferID)
        {
            LUMOS_PROFILE_FUNCTION();
            m_CommandQueue.clear();

            m_CommandBufferIndex = commandBufferID;
            m_RenderPass->BeginRenderpass(Renderer::GetSwapChain()->GetCurrentCommandBuffer(), m_ClearColour, m_Framebuffers[m_CommandBufferIndex].get(), Graphics::INLINE, m_ScreenBufferWidth, m_ScreenBufferHeight);
        }

        void DeferredRenderer::BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform)
        {
            LUMOS_PROFILE_FUNCTION();
            auto& registry = scene->GetRegistry();

            if(overrideCamera)
            {
                m_Camera = overrideCamera;
                m_CameraTransform = overrideCameraTransform;
            }
            else
            {
                auto cameraView = registry.view<Camera>();
                if(!cameraView.empty())
                {
                    m_Camera = &cameraView.get<Camera>(cameraView.front());
                    m_CameraTransform = registry.try_get<Maths::Transform>(cameraView.front());
                }
            }

            if(!m_Camera || !m_CameraTransform)
                return;

            auto view = registry.view<Graphics::Environment>();

            if(view.size() == 0)
            {
                if(m_EnvironmentMap)
                {
                    m_EnvironmentMap = nullptr;
                    m_IrradianceMap = nullptr;

                    Graphics::DescriptorDesc info {};
                    info.layoutIndex = 0;
                    info.shader = m_Shader.get();
                    m_DescriptorSet.clear();
                    m_DescriptorSet.resize(2);
                    m_DescriptorSet[0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));
                    info.layoutIndex = 1;
                    m_DescriptorSet[1] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));

                    UpdateScreenDescriptorSet();
                }
            }
            else
            {
                //Just use first
                const auto& env = view.get<Graphics::Environment>(view.front());

                if(m_EnvironmentMap != env.GetEnvironmentMap())
                {
                    Graphics::DescriptorDesc info {};
                    info.layoutIndex = 0;
                    info.shader = m_Shader.get();
                    m_DescriptorSet.clear();
                    m_DescriptorSet.resize(2);
                    m_DescriptorSet[0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));
                    info.layoutIndex = 1;
                    m_DescriptorSet[1] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));

                    m_EnvironmentMap = env.GetEnvironmentMap();
                    m_IrradianceMap = env.GetIrradianceMap();
                    UpdateScreenDescriptorSet();
                }
            }

            SubmitLightSetup(scene);

            m_OffScreenRenderer->BeginScene(scene, m_Camera, m_CameraTransform);
        }

        void DeferredRenderer::Submit(const RenderCommand& command)
        {
            LUMOS_PROFILE_FUNCTION();
            m_CommandQueue.push_back(command);
        }

        void DeferredRenderer::SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix)
        {
            LUMOS_PROFILE_FUNCTION();
            RenderCommand command;
            command.mesh = mesh;
            command.transform = transform;
            command.textureMatrix = textureMatrix;
            command.material = material;
            Submit(command);
        }

        void DeferredRenderer::SubmitLightSetup(Scene* scene)
        {
            LUMOS_PROFILE_FUNCTION();

            auto& registry = scene->GetRegistry();

            auto group = registry.group<Graphics::Light>(entt::get<Maths::Transform>);

            uint32_t numLights = 0;

            auto viewMatrix = m_CameraTransform->GetWorldMatrix().Inverse();

            auto& frustum = m_Camera->GetFrustum(viewMatrix);
            Light lights[256];
            for(auto entity : group)
            {
                const auto& [light, trans] = group.get<Graphics::Light, Maths::Transform>(entity);
                light.Position = trans.GetWorldPosition();

                if(light.Type != float(LightType::DirectionalLight))
                {
                    auto inside = frustum.IsInsideFast(Maths::Sphere(light.Position.ToVector3(), light.Radius));

                    if(inside == Maths::Intersection::OUTSIDE)
                        continue;
                }

                Maths::Vector3 forward = Maths::Vector3::FORWARD;
                forward = trans.GetWorldOrientation() * forward;

                light.Direction = forward.Normalised();
                lights[numLights] = light;
                numLights++;
            }

            m_DescriptorSet[2]->SetUniform("UniformBufferLight", "lights", lights, sizeof(Graphics::Light) * numLights);
            Maths::Vector4 cameraPos = Maths::Vector4(m_CameraTransform->GetWorldPosition());
            m_DescriptorSet[2]->SetUniform("UniformBufferLight", "cameraPosition", &cameraPos);

            auto shadowRenderer = Application::Get().GetRenderGraph()->GetShadowRenderer();
            if(shadowRenderer)
            {
                Maths::Matrix4* shadowTransforms = shadowRenderer->GetShadowProjView();
                Lumos::Maths::Vector4* uSplitDepth = shadowRenderer->GetSplitDepths();
                Maths::Matrix4 lightView = shadowRenderer->GetLightView();
                float bias = shadowRenderer->GetInitialBias();

                float maxShadowDistance = shadowRenderer->GetMaxShadowDistance();
                float LightSize = shadowRenderer->GetLightSize();
                float transitionFade = shadowRenderer->GetCascadeTransitionFade();
                float shadowFade = shadowRenderer->GetShadowFade();

                m_DescriptorSet[2]->SetUniform("UniformBufferLight", "viewMatrix", &viewMatrix);
                m_DescriptorSet[2]->SetUniform("UniformBufferLight", "lightView", &lightView);
                m_DescriptorSet[2]->SetUniform("UniformBufferLight", "uShadowTransform", shadowTransforms);
                m_DescriptorSet[2]->SetUniform("UniformBufferLight", "uSplitDepths", uSplitDepth);
                m_DescriptorSet[2]->SetUniform("UniformBufferLight", "biasMat", &m_BiasMatrix);
                m_DescriptorSet[2]->SetUniform("UniformBufferLight", "lightSize", &LightSize);
                m_DescriptorSet[2]->SetUniform("UniformBufferLight", "shadowFade", &shadowFade);
                m_DescriptorSet[2]->SetUniform("UniformBufferLight", "cascadeTransitionFade", &transitionFade);
                m_DescriptorSet[2]->SetUniform("UniformBufferLight", "maxShadowDistance", &maxShadowDistance);
                m_DescriptorSet[2]->SetUniform("UniformBufferLight", "initialBias", &bias);
            }

            int numShadows = shadowRenderer ? int(shadowRenderer->GetShadowMapNum()) : 0;
            auto cubemapMipLevels = m_EnvironmentMap ? m_EnvironmentMap->GetMipMapLevels() : 0;
            m_DescriptorSet[2]->SetUniform("UniformBufferLight", "lightCount", &numLights);
            m_DescriptorSet[2]->SetUniform("UniformBufferLight", "shadowCount", &numShadows);
            m_DescriptorSet[2]->SetUniform("UniformBufferLight", "mode", &m_RenderMode);
            m_DescriptorSet[2]->SetUniform("UniformBufferLight", "cubemapMipLevels", &cubemapMipLevels);
            m_DescriptorSet[2]->Update();
        }

        void DeferredRenderer::EndScene()
        {
        }

        void DeferredRenderer::End()
        {
            LUMOS_PROFILE_FUNCTION();
            m_RenderPass->EndRenderpass(Renderer::GetSwapChain()->GetCurrentCommandBuffer());
        }

        void DeferredRenderer::Present()
        {
            LUMOS_PROFILE_FUNCTION();
            Graphics::CommandBuffer* currentCMDBuffer = Renderer::GetSwapChain()->GetCurrentCommandBuffer();

            m_Pipeline->Bind(currentCMDBuffer);

            m_CurrentDescriptorSets[0] = m_DescriptorSet[0].get();
            m_CurrentDescriptorSets[1] = m_DescriptorSet[1].get();

            m_ScreenQuad->GetVertexBuffer()->Bind(currentCMDBuffer, m_Pipeline.get());
            m_ScreenQuad->GetIndexBuffer()->Bind(currentCMDBuffer);

            Renderer::BindDescriptorSets(m_Pipeline.get(), currentCMDBuffer, 0, m_CurrentDescriptorSets);
            Renderer::DrawIndexed(currentCMDBuffer, DrawType::TRIANGLE, m_ScreenQuad->GetIndexBuffer()->GetCount());

            m_ScreenQuad->GetVertexBuffer()->Unbind();
            m_ScreenQuad->GetIndexBuffer()->Unbind();
        }

        void DeferredRenderer::CreateDeferredPipeline()
        {
            LUMOS_PROFILE_FUNCTION();

            Graphics::PipelineDesc pipelineDesc {};
            pipelineDesc.shader = m_Shader;

            pipelineDesc.polygonMode = Graphics::PolygonMode::FILL;
            pipelineDesc.cullMode = Graphics::CullMode::BACK;
            pipelineDesc.transparencyEnabled = false;
            pipelineDesc.depthBiasEnabled = false;

            m_Pipeline = Graphics::Pipeline::Get(pipelineDesc);
        }

        void DeferredRenderer::SetRenderTarget(Texture* texture, bool rebuildFramebuffer)
        {
            LUMOS_PROFILE_FUNCTION();
            m_RenderTexture = texture;

            if(rebuildFramebuffer)
            {
                m_Framebuffers.clear();
                CreateFramebuffers();
            }
        }

        std::string RenderModeToString(int mode)
        {
            switch(mode)
            {
            case 0:
                return "Lighting";
            case 1:
                return "Colour";
            case 2:
                return "Metallic";
            case 3:
                return "Roughness";
            case 4:
                return "AO";
            case 5:
                return "Emissive";
            case 6:
                return "Normal";
            case 7:
                return "Shadow Cascades";
            default:
                return "Lighting";
            }
        }

        void DeferredRenderer::OnImGui()
        {
            LUMOS_PROFILE_FUNCTION();
            m_OffScreenRenderer->OnImGui();

            ImGui::TextUnformatted("Deferred Renderer");

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Number Of Renderables");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::Text("%5.2lu", m_CommandQueue.size());
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Render Mode");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if(ImGui::BeginMenu(RenderModeToString(m_RenderMode).c_str()))
            {
                const int numRenderModes = 8;

                for(int i = 0; i < numRenderModes; i++)
                {
                    if(ImGui::MenuItem(RenderModeToString(i).c_str(), "", m_RenderMode == i, true))
                    {
                        m_RenderMode = i;
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopStyleVar();
        }

        void DeferredRenderer::CreateFramebuffers()
        {
            TextureType attachmentTypes[2];
            attachmentTypes[0] = TextureType::COLOUR;

            Texture* attachments[1];
            FramebufferDesc frameBufferDesc {};
            frameBufferDesc.width = m_ScreenBufferWidth;
            frameBufferDesc.height = m_ScreenBufferHeight;
            frameBufferDesc.attachmentCount = 1;
            frameBufferDesc.renderPass = m_RenderPass.get();
            frameBufferDesc.attachmentTypes = attachmentTypes;

            if(m_RenderTexture)
            {
                attachments[0] = m_RenderTexture;
                frameBufferDesc.attachments = attachments;
                frameBufferDesc.screenFBO = false;
                m_Framebuffers.emplace_back(Framebuffer::Get(frameBufferDesc));
            }
            else
            {
                for(uint32_t i = 0; i < Renderer::GetSwapChain()->GetSwapChainBufferCount(); i++)
                {
                    frameBufferDesc.screenFBO = true;
                    //TODO: frameBufferDesc.screenFBO = true; should be enough. No need for GetImage(i);
                    //Maybe have frameBufferDesc.swapchainIndex = i;
                    attachments[0] = Renderer::GetSwapChain()->GetImage(i);
                    frameBufferDesc.attachments = attachments;

                    m_Framebuffers.emplace_back(Framebuffer::Get(frameBufferDesc));
                }
            }
        }

        void DeferredRenderer::CreateLightBuffer()
        {
        }

        void DeferredRenderer::OnResize(uint32_t width, uint32_t height)
        {
            LUMOS_PROFILE_FUNCTION();
            m_Framebuffers.clear();

            DeferredRenderer::SetScreenBufferSize(width, height);

            CreateFramebuffers();

            m_OffScreenRenderer->OnResize(width, height);

            m_EnvironmentMap = nullptr;
            m_IrradianceMap = nullptr;

            //Update DescriptorSet with updated gbuffer textures
            UpdateScreenDescriptorSet();
        }

        void DeferredRenderer::UpdateScreenDescriptorSet()
        {
            m_DescriptorSet[1]->SetTexture("uColourSampler", Application::Get().GetRenderGraph()->GetGBuffer()->GetTexture(SCREENTEX_COLOUR));
            m_DescriptorSet[1]->SetTexture("uPositionSampler", Application::Get().GetRenderGraph()->GetGBuffer()->GetTexture(SCREENTEX_POSITION));
            m_DescriptorSet[1]->SetTexture("uNormalSampler", Application::Get().GetRenderGraph()->GetGBuffer()->GetTexture(SCREENTEX_NORMALS));
            m_DescriptorSet[1]->SetTexture("uPBRSampler", Application::Get().GetRenderGraph()->GetGBuffer()->GetTexture(SCREENTEX_PBR));
            m_DescriptorSet[1]->SetTexture("uPreintegratedFG", m_PreintegratedFG.get());
            m_DescriptorSet[1]->SetTexture("uEnvironmentMap", m_EnvironmentMap, TextureType::CUBE);
            m_DescriptorSet[1]->SetTexture("uIrradianceMap", m_IrradianceMap, TextureType::CUBE);
            auto shadowRenderer = Application::Get().GetRenderGraph()->GetShadowRenderer();
            if(shadowRenderer)
                m_DescriptorSet[1]->SetTexture("uShadowMap", reinterpret_cast<Texture*>(shadowRenderer->GetTexture()), TextureType::DEPTHARRAY);
            m_DescriptorSet[1]->SetTexture("uDepthSampler", Application::Get().GetRenderGraph()->GetGBuffer()->GetDepthTexture(), TextureType::DEPTH);
            m_DescriptorSet[1]->Update();

            CreateLightBuffer();
        }
    }
}
