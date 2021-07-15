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
#include "Graphics/RHI/Swapchain.h"
#include "Graphics/RHI/RenderPass.h"
#include "Graphics/RHI/Pipeline.h"
#include "Graphics/RHI/GraphicsContext.h"
#include "Graphics/Environment.h"
#include "Embedded/BRDFTexture.inl"
#include "Utilities/AssetManager.h"

#include <imgui/imgui.h>

#define MAX_LIGHTS 32
#define MAX_SHADOWMAPS 4

namespace Lumos
{
    namespace Graphics
    {
        enum PSSystemUniformIndices : int32_t
        {
            PSSystemUniformIndex_Lights = 0,
            PSSystemUniformIndex_CameraPosition,
            PSSystemUniformIndex_ViewMatrix,
            PSSystemUniformIndex_LightView,
            PSSystemUniformIndex_ShadowTransforms,
            PSSystemUniformIndex_ShadowSplitDepths,
            PSSystemUniformIndex_BiasMatrix,
            PSSystemUniformIndex_LightCount,
            PSSystemUniformIndex_ShadowCount,
            PSSystemUniformIndex_RenderMode,
            PSSystemUniformIndex_cubemapMipLevels,
            PSSystemUniformIndex_lightSize,
            PSSystemUniformIndex_maxShadowDistance,
            PSSystemUniformIndex_shadowFade,
            PSSystemUniformIndex_cascadeTransitionFade,
            PSSystemUniformIndex_Size
        };

        DeferredRenderer::DeferredRenderer(uint32_t width, uint32_t height)
        {
            DeferredRenderer::SetScreenBufferSize(width, height);
            DeferredRenderer::Init();
        }

        DeferredRenderer::~DeferredRenderer()
        {
            delete m_UniformBuffer;
            delete m_LightUniformBuffer;
            delete m_ScreenQuad;
            delete m_OffScreenRenderer;

            delete[] m_PSSystemUniformBuffer;
            delete m_DeferredCommandBuffers;
        }

        void DeferredRenderer::Init()
        {
            LUMOS_PROFILE_FUNCTION();
            m_OffScreenRenderer = new DeferredOffScreenRenderer(m_ScreenBufferWidth, m_ScreenBufferHeight);

            m_Shader = Application::Get().GetShaderLibrary()->GetResource("//CoreShaders/DeferredLight.shader");

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
            m_PreintegratedFG = UniqueRef<Texture2D>(Texture2D::CreateFromSource(BRDFTextureWidth, BRDFTextureHeight, (void*)BRDFTexture, param));

            m_LightUniformBuffer = nullptr;
            m_UniformBuffer = nullptr;

            m_ScreenQuad = Graphics::CreateScreenQuad();

            // Pixel/fragment shader System uniforms
            m_PSSystemUniformBufferSize = sizeof(Light) * MAX_LIGHTS + sizeof(Maths::Matrix4) * MAX_SHADOWMAPS + sizeof(Maths::Matrix4) * 3 + sizeof(Maths::Vector4) + sizeof(Maths::Vector4) * MAX_SHADOWMAPS + sizeof(float) * 4 + sizeof(int) * 4 + sizeof(float);
            m_PSSystemUniformBuffer = new uint8_t[m_PSSystemUniformBufferSize];
            memset(m_PSSystemUniformBuffer, 0, m_PSSystemUniformBufferSize);
            m_PSSystemUniformBufferOffsets.resize(PSSystemUniformIndex_Size);

            // Per Scene System Uniforms
            m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_Lights] = 0;
            m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowTransforms] = m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_Lights] + sizeof(Light) * MAX_LIGHTS;
            m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ViewMatrix] = m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowTransforms] + sizeof(Maths::Matrix4) * MAX_SHADOWMAPS;
            m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_LightView] = m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ViewMatrix] + sizeof(Maths::Matrix4);
            m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_BiasMatrix] = m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_LightView] + sizeof(Maths::Matrix4);
            m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_CameraPosition] = m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_BiasMatrix] + sizeof(Maths::Matrix4);
            m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowSplitDepths] = m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_CameraPosition] + sizeof(Maths::Vector4);

            m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_lightSize] = m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowSplitDepths] + sizeof(Maths::Vector4) * MAX_SHADOWMAPS;
            m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_maxShadowDistance] = m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_lightSize] + sizeof(float);
            m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_shadowFade] = m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_maxShadowDistance] + sizeof(float);
            m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_cascadeTransitionFade] = m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_shadowFade] + sizeof(float);
            m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_LightCount] = m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_cascadeTransitionFade] + sizeof(float);
            m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowCount] = m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_LightCount] + sizeof(int);
            m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_RenderMode] = m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowCount] + sizeof(int);
            m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_cubemapMipLevels] = m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_RenderMode] + sizeof(int);

            AttachmentInfo textureTypes[2] = {
                { TextureType::COLOUR, TextureFormat::RGBA8 }
            };
            Graphics::RenderPassDesc renderpassCI {};
            renderpassCI.attachmentCount = 1;
            renderpassCI.textureType = textureTypes;
            renderpassCI.clear = true;

            m_RenderPass = Graphics::RenderPass::Get(renderpassCI);

            m_DeferredCommandBuffers = Graphics::CommandBuffer::Create();
            m_DeferredCommandBuffers->Init(true);

            Graphics::DescriptorDesc info {};
            info.layoutIndex = 0;
            info.shader = m_Shader.get();
            m_DescriptorSet.resize(2);
            m_DescriptorSet[0] = SharedRef<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));
            info.layoutIndex = 1;
            m_DescriptorSet[1] = SharedRef<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));

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
                commandBufferIndex = Renderer::GetSwapchain()->GetCurrentBufferIndex();

            //Renderer::GetRenderer()->ClearRenderTarget(m_RenderTexture ? m_RenderTexture : Renderer::GetSwapchain()->GetImage(commandBufferIndex), Renderer::GetSwapchain()->GetCurrentCommandBuffer());

            m_OffScreenRenderer->RenderScene();

            //if(!m_OffScreenRenderer->HadRendered())
            //   return;

            SetSystemUniforms(m_Shader.get());

            Begin(commandBufferIndex);
            Present();
            End();
        }

        void DeferredRenderer::PresentToScreen()
        {
            LUMOS_PROFILE_FUNCTION();
            //Renderer::Present(Renderer::GetSwapchain()->GetCurrentCommandBuffer());
        }

        void DeferredRenderer::Begin(int commandBufferID)
        {
            LUMOS_PROFILE_FUNCTION();
            m_CommandQueue.clear();

            m_CommandBufferIndex = commandBufferID;
            m_RenderPass->BeginRenderpass(Renderer::GetSwapchain()->GetCurrentCommandBuffer(), m_ClearColour, m_Framebuffers[m_CommandBufferIndex].get(), Graphics::INLINE, m_ScreenBufferWidth, m_ScreenBufferHeight);
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
                    m_DescriptorSet[0] = SharedRef<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));
                    info.layoutIndex = 1;
                    m_DescriptorSet[1] = SharedRef<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));

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
                    m_DescriptorSet[0] = SharedRef<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));
                    info.layoutIndex = 1;
                    m_DescriptorSet[1] = SharedRef<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));

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

                memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_Lights] + sizeof(Graphics::Light) * numLights, &light, sizeof(Graphics::Light));
                numLights++;
            }

            Maths::Vector4 cameraPos = Maths::Vector4(m_CameraTransform->GetWorldPosition());
            memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_CameraPosition], &cameraPos, sizeof(Maths::Vector4));

            auto shadowRenderer = Application::Get().GetRenderGraph()->GetShadowRenderer();
            if(shadowRenderer)
            {
                Maths::Matrix4* shadowTransforms = shadowRenderer->GetShadowProjView();
                Lumos::Maths::Vector4* uSplitDepth = shadowRenderer->GetSplitDepths();
                const Maths::Matrix4& lightView = shadowRenderer->GetLightView();

                memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ViewMatrix], &viewMatrix, sizeof(Maths::Matrix4));
                memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_LightView], &lightView, sizeof(Maths::Matrix4));

                memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowTransforms], shadowTransforms, sizeof(Maths::Matrix4) * MAX_SHADOWMAPS);
                memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowSplitDepths], uSplitDepth, sizeof(Maths::Vector4) * MAX_SHADOWMAPS);
                memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_BiasMatrix], &m_BiasMatrix, sizeof(Maths::Matrix4));

                float bias = shadowRenderer->GetInitialBias();

                float maxShadowDistance = shadowRenderer->GetMaxShadowDistance();
                float LightSize = shadowRenderer->GetLightSize();
                float transitionFade = shadowRenderer->GetCascadeTransitionFade();
                float shadowFade = shadowRenderer->GetShadowFade();

                memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_lightSize], &LightSize, sizeof(float));
                memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_shadowFade], &shadowFade, sizeof(float));
                memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_cascadeTransitionFade], &transitionFade, sizeof(float));
                memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_maxShadowDistance], &maxShadowDistance, sizeof(float));

                memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_cubemapMipLevels] + sizeof(int), &bias, sizeof(float));
            }

            int numShadows = shadowRenderer ? int(shadowRenderer->GetShadowMapNum()) : 0;

            auto cubemapMipLevels = m_EnvironmentMap ? m_EnvironmentMap->GetMipMapLevels() : 0;
            memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_LightCount], &numLights, sizeof(int));
            memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowCount], &numShadows, sizeof(int));
            memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_RenderMode], &m_RenderMode, sizeof(int));
            memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_cubemapMipLevels], &cubemapMipLevels, sizeof(int));
        }

        void DeferredRenderer::EndScene()
        {
        }

        void DeferredRenderer::End()
        {
            LUMOS_PROFILE_FUNCTION();
            m_RenderPass->EndRenderpass(Renderer::GetSwapchain()->GetCurrentCommandBuffer());
        }

        void DeferredRenderer::SetSystemUniforms(Shader* shader) const
        {
            LUMOS_PROFILE_FUNCTION();
            m_LightUniformBuffer->SetData(m_PSSystemUniformBufferSize, *&m_PSSystemUniformBuffer);
        }

        void DeferredRenderer::Present()
        {
            LUMOS_PROFILE_FUNCTION();
            Graphics::CommandBuffer* currentCMDBuffer = Renderer::GetSwapchain()->GetCurrentCommandBuffer();

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

            Graphics::PipelineDesc pipelineCreateInfo {};
            pipelineCreateInfo.shader = m_Shader;
            pipelineCreateInfo.renderpass = m_RenderPass;
            pipelineCreateInfo.polygonMode = Graphics::PolygonMode::FILL;
            pipelineCreateInfo.cullMode = Graphics::CullMode::BACK;
            pipelineCreateInfo.transparencyEnabled = false;
            pipelineCreateInfo.depthBiasEnabled = false;

            m_Pipeline = Graphics::Pipeline::Get(pipelineCreateInfo);
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
            FramebufferDesc bufferInfo {};
            bufferInfo.width = m_ScreenBufferWidth;
            bufferInfo.height = m_ScreenBufferHeight;
            bufferInfo.attachmentCount = 1;
            bufferInfo.renderPass = m_RenderPass.get();
            bufferInfo.attachmentTypes = attachmentTypes;

            if(m_RenderTexture)
            {
                attachments[0] = m_RenderTexture;
                bufferInfo.attachments = attachments;
                bufferInfo.screenFBO = false;
                m_Framebuffers.emplace_back(Framebuffer::Get(bufferInfo));
            }
            else
            {
                for(uint32_t i = 0; i < Renderer::GetSwapchain()->GetSwapchainBufferCount(); i++)
                {
                    bufferInfo.screenFBO = true;
                    //TODO: bufferInfo.screenFBO = true; should be enough. No need for GetImage(i);
                    //Maybe have bufferInfo.swapchainIndex = i;
                    attachments[0] = Renderer::GetSwapchain()->GetImage(i);
                    bufferInfo.attachments = attachments;

                    m_Framebuffers.emplace_back(Framebuffer::Get(bufferInfo));
                }
            }
        }

        void DeferredRenderer::CreateLightBuffer()
        {
            if(m_LightUniformBuffer == nullptr)
            {
                m_LightUniformBuffer = Graphics::UniformBuffer::Create();

                uint32_t bufferSize = m_PSSystemUniformBufferSize;
                m_LightUniformBuffer->Init(bufferSize, nullptr);
            }

            std::vector<Graphics::Descriptor> bufferInfos;

            Graphics::Descriptor bufferInfo = {};
            bufferInfo.name = "UniformBufferLight";
            bufferInfo.buffer = m_LightUniformBuffer;
            bufferInfo.offset = 0;
            bufferInfo.size = m_PSSystemUniformBufferSize;
            bufferInfo.type = Graphics::DescriptorType::UNIFORM_BUFFER;
            bufferInfo.binding = 0;
            bufferInfo.shaderType = ShaderType::FRAGMENT;

            bufferInfos.push_back(bufferInfo);

            m_DescriptorSet[0]->Update(bufferInfos);
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
            std::vector<Graphics::Descriptor> bufferInfos;

            Graphics::Descriptor imageInfo = {};
            imageInfo.texture = { Application::Get().GetRenderGraph()->GetGBuffer()->GetTexture(SCREENTEX_COLOUR) };
            imageInfo.binding = 0;
            imageInfo.type = DescriptorType::IMAGE_SAMPLER;
            imageInfo.name = "uColourSampler";

            Graphics::Descriptor imageInfo2 = {};
            imageInfo2.texture = { Application::Get().GetRenderGraph()->GetGBuffer()->GetTexture(SCREENTEX_POSITION) };
            imageInfo2.binding = 1;
            imageInfo2.type = DescriptorType::IMAGE_SAMPLER;
            imageInfo2.name = "uPositionSampler";

            Graphics::Descriptor imageInfo3 = {};
            imageInfo3.texture = { Application::Get().GetRenderGraph()->GetGBuffer()->GetTexture(SCREENTEX_NORMALS) };
            imageInfo3.binding = 2;
            imageInfo3.type = DescriptorType::IMAGE_SAMPLER;
            imageInfo3.name = "uNormalSampler";

            Graphics::Descriptor imageInfo4 = {};
            imageInfo4.texture = { Application::Get().GetRenderGraph()->GetGBuffer()->GetTexture(SCREENTEX_PBR) };
            imageInfo4.binding = 3;
            imageInfo4.type = DescriptorType::IMAGE_SAMPLER;
            imageInfo4.name = "uPBRSampler";

            Graphics::Descriptor imageInfo5 = {};
            imageInfo5.texture = { m_PreintegratedFG.get() };
            imageInfo5.binding = 4;
            imageInfo5.type = DescriptorType::IMAGE_SAMPLER;
            imageInfo5.name = "uPreintegratedFG";

            Graphics::Descriptor imageInfo6 = {};
            imageInfo6.texture = { m_EnvironmentMap };
            imageInfo6.binding = 5;
            imageInfo5.type = DescriptorType::IMAGE_SAMPLER;
            imageInfo6.textureType = TextureType::CUBE;
            imageInfo6.name = "uEnvironmentMap";

            Graphics::Descriptor imageInfo7 = {};
            imageInfo7.texture = { m_IrradianceMap };
            imageInfo7.binding = 6;
            imageInfo5.type = DescriptorType::IMAGE_SAMPLER;
            imageInfo7.textureType = TextureType::CUBE;
            imageInfo7.name = "uIrradianceMap";

            Graphics::Descriptor imageInfo8 = {};
            auto shadowRenderer = Application::Get().GetRenderGraph()->GetShadowRenderer();
            if(shadowRenderer)
            {
                imageInfo8.texture = { reinterpret_cast<Texture*>(shadowRenderer->GetTexture()) };
                imageInfo8.binding = 7;
                imageInfo5.type = DescriptorType::IMAGE_SAMPLER;
                imageInfo8.textureType = TextureType::DEPTHARRAY;
                imageInfo8.name = "uShadowMap";
            }

            Graphics::Descriptor imageInfo9 = {};
            imageInfo9.texture = { Application::Get().GetRenderGraph()->GetGBuffer()->GetDepthTexture() };
            imageInfo9.binding = 8;
            imageInfo5.type = DescriptorType::IMAGE_SAMPLER;
            imageInfo9.textureType = TextureType::DEPTH;
            imageInfo9.name = "uDepthSampler";

            bufferInfos.push_back(imageInfo);
            bufferInfos.push_back(imageInfo2);
            bufferInfos.push_back(imageInfo3);
            bufferInfos.push_back(imageInfo4);
            bufferInfos.push_back(imageInfo5);
            if(m_EnvironmentMap)
                bufferInfos.push_back(imageInfo6);
            if(m_IrradianceMap)
                bufferInfos.push_back(imageInfo7);
            if(shadowRenderer)
                bufferInfos.push_back(imageInfo8);

            m_DescriptorSet[1]->Update(bufferInfos);

            CreateLightBuffer();
        }
    }
}
