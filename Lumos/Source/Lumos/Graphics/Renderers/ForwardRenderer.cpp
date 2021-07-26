#include "Precompiled.h"
#include "ForwardRenderer.h"
#include "ShadowRenderer.h"
#include "Graphics/RHI/Shader.h"
#include "Graphics/RHI/Framebuffer.h"
#include "Graphics/Light.h"
#include "Graphics/Environment.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/RHI/UniformBuffer.h"
#include "Graphics/Mesh.h"
#include "Graphics/Model.h"
#include "Graphics/Material.h"
#include "Graphics/RHI/Renderer.h"
#include "Graphics/RHI/CommandBuffer.h"
#include "Graphics/RHI/Swapchain.h"
#include "Graphics/RHI/RenderPass.h"
#include "Graphics/RHI/Pipeline.h"
#include "Graphics/RHI/GraphicsContext.h"
#include "Graphics/GBuffer.h"
#include "Scene/Scene.h"
#include "Scene/Component/TextureMatrixComponent.h"
#include "Maths/Maths.h"
#include "Maths/Transform.h"
#include "Embedded/BRDFTexture.inl"
#include "Embedded/CheckerBoardTextureArray.inl"

#include "Core/Application.h"
#include "RenderGraph.h"
#include "Graphics/Camera/Camera.h"

#define MAX_LIGHTS 32
#define MAX_SHADOWMAPS 4

namespace Lumos
{
    namespace Graphics
    {
        enum PSSceneUniformIndices : int32_t
        {
            PSSceneUniformIndex_Lights = 0,
            PSSceneUniformIndex_CameraPosition,
            PSSceneUniformIndex_ViewMatrix,
            PSSceneUniformIndex_LightView,
            PSSceneUniformIndex_ShadowTransforms,
            PSSceneUniformIndex_ShadowSplitDepths,
            PSSceneUniformIndex_BiasMatrix,
            PSSceneUniformIndex_LightCount,
            PSSceneUniformIndex_ShadowCount,
            PSSceneUniformIndex_RenderMode,
            PSSceneUniformIndex_cubemapMipLevels,
            PSSceneUniformIndex_lightSize,
            PSSceneUniformIndex_maxShadowDistance,
            PSSceneUniformIndex_shadowFade,
            PSSceneUniformIndex_cascadeTransitionFade,
            PSSceneUniformIndex_Size
        };
        
        ForwardRenderer::ForwardRenderer(uint32_t width, uint32_t height, bool depthTest)
        {
            LUMOS_PROFILE_FUNCTION();

            m_LightUniformBuffer = nullptr;
            m_DepthTest = depthTest;
            SetScreenBufferSize(width, height);
            ForwardRenderer::Init();
        }

        ForwardRenderer::~ForwardRenderer()
        {
            LUMOS_PROFILE_FUNCTION();
            delete m_UniformBuffer;
            delete m_LightUniformBuffer;
            delete m_DefaultMaterial;

            delete[] m_VSSystemUniformBuffer;

            for(auto& commandBuffer : m_CommandBuffers)
            {
                delete commandBuffer;
            }
        }

        void ForwardRenderer::RenderScene()
        {
            LUMOS_PROFILE_FUNCTION();

            Begin();

            SetSystemUniforms(m_Shader.get());

            Present();

            EndScene();
            End();
        }

        enum VSSystemUniformIndices : int32_t
        {
            VSSystemUniformIndex_ProjectionMatrix = 0,
            VSSystemUniformIndex_ViewMatrix = 1,
            VSSystemUniformIndex_ModelMatrix = 2,
            VSSystemUniformIndex_TextureMatrix = 3,
            VSSystemUniformIndex_Size
        };

        void ForwardRenderer::Init()
        {
            LUMOS_PROFILE_FUNCTION();
            
            m_Shader = Application::Get().GetShaderLibrary()->GetResource("//CoreShaders/ForwardPBR.shader");

            m_CommandQueue.reserve(1000);

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
            
            //
            // Vertex shader System uniforms
            //
            
            m_LightUniformBuffer = nullptr;
            m_UniformBuffer = nullptr;
            
            m_VSSystemUniformBufferSize = sizeof(Maths::Matrix4);// + sizeof(Maths::Matrix4) + sizeof(Maths::Matrix4) + sizeof(Maths::Matrix4);
            m_VSSystemUniformBuffer = new uint8_t[m_VSSystemUniformBufferSize];
            memset(m_VSSystemUniformBuffer, 0, m_VSSystemUniformBufferSize);
            m_VSSystemUniformBufferOffsets.resize(VSSystemUniformIndex_Size);

            // Per Scene System Uniforms
            m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionMatrix] = 0;

            // Pixel/fragment shader System uniforms
            m_PSSceneUniformBufferSize = sizeof(Light) * MAX_LIGHTS + sizeof(Maths::Matrix4) * MAX_SHADOWMAPS + sizeof(Maths::Matrix4) * 3 + sizeof(Maths::Vector4) + sizeof(Maths::Vector4) * MAX_SHADOWMAPS + sizeof(float) * 4 + sizeof(int) * 4 + sizeof(float);
            m_PSSceneUniformBuffer = new uint8_t[m_PSSceneUniformBufferSize];
            memset(m_PSSceneUniformBuffer, 0, m_PSSceneUniformBufferSize);
            m_PSSceneUniformBufferOffsets.resize(PSSceneUniformIndex_Size);

            // Per Scene System Uniforms
            m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_Lights] = 0;
            m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_ShadowTransforms] = m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_Lights] + sizeof(Light) * MAX_LIGHTS;
            m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_ViewMatrix] = m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_ShadowTransforms] + sizeof(Maths::Matrix4) * MAX_SHADOWMAPS;
            m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_LightView] = m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_ViewMatrix] + sizeof(Maths::Matrix4);
            m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_BiasMatrix] = m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_LightView] + sizeof(Maths::Matrix4);
            m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_CameraPosition] = m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_BiasMatrix] + sizeof(Maths::Matrix4);
            m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_ShadowSplitDepths] = m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_CameraPosition] + sizeof(Maths::Vector4);

            m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_lightSize] = m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_ShadowSplitDepths] + sizeof(Maths::Vector4) * MAX_SHADOWMAPS;
            m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_maxShadowDistance] = m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_lightSize] + sizeof(float);
            m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_shadowFade] = m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_maxShadowDistance] + sizeof(float);
            m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_cascadeTransitionFade] = m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_shadowFade] + sizeof(float);
            m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_LightCount] = m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_cascadeTransitionFade] + sizeof(float);
            m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_ShadowCount] = m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_LightCount] + sizeof(int);
            m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_RenderMode] = m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_ShadowCount] + sizeof(int);
            m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_cubemapMipLevels] = m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_RenderMode] + sizeof(int);


            m_UniformBuffer = Graphics::UniformBuffer::Create();

            Graphics::RenderPassDesc renderpassCI {};

            if(m_DepthTest)
            {
                AttachmentInfo textureTypes[2] = {
                    { TextureType::COLOUR, TextureFormat::RGBA8 },
                    { TextureType::DEPTH, TextureFormat::DEPTH }
                };

                renderpassCI.attachmentCount = 2;
                renderpassCI.textureType = textureTypes;
				m_RenderPass = Graphics::RenderPass::Get(renderpassCI);
            }
            else
            {
                AttachmentInfo textureTypes[1] = {
                    { TextureType::COLOUR, TextureFormat::RGBA8 },
                };

                renderpassCI.attachmentCount = 1;
                renderpassCI.textureType = textureTypes;
				m_RenderPass = Graphics::RenderPass::Get(renderpassCI);
            }

            CreateFramebuffers();
            CreateGraphicsPipeline(); 

            m_ClearColour = Maths::Vector4(0.4f, 0.4f, 0.4f, 1.0f);
            
            auto descriptorSetScene = m_Shader->GetDescriptorInfo(2);

            Graphics::DescriptorDesc info {};
            info.layoutIndex = 0;
            info.shader = m_Shader.get();
            m_DescriptorSet.resize(3);
            m_DescriptorSet[0] = SharedRef<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));
            info.layoutIndex = 2;
            m_DescriptorSet[2] = SharedRef<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));
            
            m_DefaultMaterial = new Material(m_Shader);

            Graphics::MaterialProperties properties;
            properties.albedoColour = Maths::Vector4(1.0f);
            properties.roughnessColour = Maths::Vector4(0.5f);
            properties.metallicColour = Maths::Vector4(0.5f);
            properties.usingAlbedoMap = 0.0f;
            properties.usingRoughnessMap = 0.0f;
            properties.usingNormalMap = 0.0f;
            properties.usingMetallicMap = 0.0f;

            m_DefaultMaterial->SetMaterialProperites(properties);
            m_DefaultMaterial->CreateDescriptorSet(1);

            m_CurrentDescriptorSets.resize(3);
            
            uint32_t bufferSize = static_cast<uint32_t>(sizeof(UniformBufferObject));
            m_UniformBuffer->Init(bufferSize, nullptr);

            std::vector<Graphics::Descriptor> bufferInfos;

            Graphics::Descriptor bufferInfo = {};
            bufferInfo.buffer = m_UniformBuffer;
            bufferInfo.offset = 0;
            bufferInfo.size = sizeof(UniformBufferObject);
            bufferInfo.type = Graphics::DescriptorType::UNIFORM_BUFFER;
            bufferInfo.binding = 0;
            bufferInfo.shaderType = ShaderType::VERTEX;
            bufferInfo.name = "UniformBufferObject";
            bufferInfos.push_back(bufferInfo);
            m_DescriptorSet[0]->Update(bufferInfos);
            
            UpdateScreenDescriptorSet();
        }

        void ForwardRenderer::Begin()
        {
            LUMOS_PROFILE_FUNCTION();
            m_CurrentBufferID = 0;
            if(!m_RenderTexture)
                m_CurrentBufferID = Renderer::GetSwapchain()->GetCurrentBufferIndex();
            
            m_RenderPass->BeginRenderpass(Renderer::GetSwapchain()->GetCurrentCommandBuffer(), m_ClearColour, m_Framebuffers[m_CurrentBufferID].get(), Graphics::INLINE, m_ScreenBufferWidth, m_ScreenBufferHeight);
        }

        void ForwardRenderer::BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform)
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

            auto view = m_CameraTransform->GetWorldMatrix().Inverse();
            auto projView = m_Camera->GetProjectionMatrix() * view;
            
            memcpy(m_VSSystemUniformBuffer + m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionMatrix], &projView, sizeof(Maths::Matrix4));
            
            m_CommandQueue.clear();

            auto group = registry.group<Model>(entt::get<Maths::Transform>);

            for(auto entity : group)
            {
                const auto& [model, trans] = group.get<Model, Maths::Transform>(entity);

                const auto& meshes = model.GetMeshes();

                for(auto mesh : meshes)
                {
                    if(mesh->GetActive())
                    {
                        auto& worldTransform = trans.GetWorldMatrix();

                        auto bb = mesh->GetBoundingBox();
                        auto bbCopy = bb->Transformed(worldTransform);
                        auto inside = m_Frustum.IsInsideFast(bbCopy);

                        if(inside == Maths::Intersection::OUTSIDE)
                            continue;

                        auto meshPtr = mesh;

                        auto textureMatrixTransform = registry.try_get<TextureMatrixComponent>(entity);
                        Maths::Matrix4 textureMatrix;
                        if(textureMatrixTransform)
                            textureMatrix = textureMatrixTransform->GetMatrix();
                        else
                            textureMatrix = Maths::Matrix4();

                        SubmitMesh(meshPtr.get(), meshPtr->GetMaterial() ? meshPtr->GetMaterial().get() : m_DefaultMaterial, worldTransform, textureMatrix);
                    }
                }
            }
                    
            auto envView = registry.view<Graphics::Environment>();

            if(envView.size() == 0)
            {
                if(m_EnvironmentMap)
                {
                    m_EnvironmentMap = nullptr;
                    m_IrradianceMap = nullptr;

                    Graphics::DescriptorDesc info {};
                    info.shader = m_Shader.get();
                    info.layoutIndex = 2;
                    m_DescriptorSet[2] = SharedRef<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));
                    UpdateScreenDescriptorSet();
                }
            }
            else
            {
                //Just use first
                const auto& env = envView.get<Graphics::Environment>(envView.front());

                if(m_EnvironmentMap != env.GetEnvironmentMap())
                {
                    Graphics::DescriptorDesc info {};
                    info.shader = m_Shader.get();
                    info.layoutIndex = 2;
                    m_DescriptorSet[2] = SharedRef<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));
                    m_EnvironmentMap = env.GetEnvironmentMap();
                    m_IrradianceMap = env.GetIrradianceMap();
                    UpdateScreenDescriptorSet();

                }
            }
            SubmitLightSetup(scene);

        }

        void ForwardRenderer::Submit(const RenderCommand& command)
        {
            LUMOS_PROFILE_FUNCTION();
            m_CommandQueue.push_back(command);
        }

        void ForwardRenderer::SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix)
        {
            LUMOS_PROFILE_FUNCTION();
            RenderCommand command;
            command.mesh = mesh;
            command.transform = transform;
            command.textureMatrix = textureMatrix;
            command.material = material;
            Submit(command);
        }

        void ForwardRenderer::EndScene()
        {
        }

        void ForwardRenderer::End()
        {
            LUMOS_PROFILE_FUNCTION();
            m_RenderPass->EndRenderpass(Renderer::GetSwapchain()->GetCurrentCommandBuffer());
        }

        void ForwardRenderer::SetSystemUniforms(Shader* shader) const
        {
            LUMOS_PROFILE_FUNCTION();
            m_UniformBuffer->SetData(sizeof(UniformBufferObject), *&m_VSSystemUniformBuffer);
            m_LightUniformBuffer->SetData(m_PSSceneUniformBufferSize, *&m_PSSceneUniformBuffer);

        }

        void ForwardRenderer::Present()
        {
            LUMOS_PROFILE_FUNCTION();

            int index = 0;

            for(auto& command : m_CommandQueue)
            {
                Mesh* mesh = command.mesh;

                Graphics::CommandBuffer* currentCMDBuffer = Renderer::GetSwapchain()->GetCurrentCommandBuffer();

                Graphics::PipelineDesc pipelineCreateInfo {};
                pipelineCreateInfo.shader = command.material->GetShader();
                pipelineCreateInfo.renderpass = m_RenderPass;
                pipelineCreateInfo.polygonMode = Graphics::PolygonMode::FILL;
                pipelineCreateInfo.cullMode = command.material->GetFlag(Material::RenderFlags::TWOSIDED) ? Graphics::CullMode::NONE : Graphics::CullMode::BACK;
                pipelineCreateInfo.transparencyEnabled = command.material->GetFlag(Material::RenderFlags::ALPHABLEND);

                auto pipeline = Graphics::Pipeline::Get(pipelineCreateInfo);
                
                pipeline->Bind(currentCMDBuffer);

                command.material->Bind();

                m_CurrentDescriptorSets[0] = m_DescriptorSet[0].get();
                m_CurrentDescriptorSets[1] = command.material->GetDescriptorSet();
                m_CurrentDescriptorSets[2] = m_DescriptorSet[2].get();

                mesh->GetVertexBuffer()->Bind(currentCMDBuffer, pipeline.get());
                mesh->GetIndexBuffer()->Bind(currentCMDBuffer);
				
				auto trans = command.transform;
                auto& pushConstants = m_Shader->GetPushConstants()[0];
                pushConstants.SetValue("transform", (void*)&trans);
				
                m_Shader->BindPushConstants(currentCMDBuffer, pipeline.get());
                Renderer::BindDescriptorSets(pipeline.get(), currentCMDBuffer, 0, m_CurrentDescriptorSets);
                Renderer::DrawIndexed(currentCMDBuffer, DrawType::TRIANGLE, mesh->GetIndexBuffer()->GetCount());

                mesh->GetVertexBuffer()->Unbind();
                mesh->GetIndexBuffer()->Unbind();

                index++;
            }
        }

        void ForwardRenderer::OnResize(uint32_t width, uint32_t height)
        {
            LUMOS_PROFILE_FUNCTION();
            SetScreenBufferSize(width, height);
            m_Framebuffers.clear();

            CreateFramebuffers();
            
            m_EnvironmentMap = nullptr;
            m_IrradianceMap = nullptr;

            UpdateScreenDescriptorSet();
        }

        void ForwardRenderer::CreateGraphicsPipeline()
        {
            LUMOS_PROFILE_FUNCTION();

            Graphics::PipelineDesc pipelineCreateInfo {};
            pipelineCreateInfo.shader = m_Shader;
            pipelineCreateInfo.renderpass = m_RenderPass;
            pipelineCreateInfo.polygonMode = Graphics::PolygonMode::FILL;
            pipelineCreateInfo.cullMode = Graphics::CullMode::BACK;
            pipelineCreateInfo.transparencyEnabled = false;

            m_Pipeline = Graphics::Pipeline::Get(pipelineCreateInfo);
        }

        void ForwardRenderer::SetRenderTarget(Texture* texture, bool rebuildFramebuffer)
        {
            LUMOS_PROFILE_FUNCTION();

            m_RenderTexture = texture;

            if(!rebuildFramebuffer)
                return;

            m_Framebuffers.clear();

            CreateFramebuffers();
        }

        void ForwardRenderer::CreateFramebuffers()
        {
            LUMOS_PROFILE_FUNCTION();

            TextureType attachmentTypes[2];
            Texture* attachments[2];

            attachmentTypes[0] = TextureType::COLOUR;
            if(m_DepthTest)
            {
                attachmentTypes[1] = TextureType::DEPTH;
                attachments[1] = reinterpret_cast<Texture*>(Application::Get().GetRenderGraph()->GetGBuffer()->GetDepthTexture());
            }

            FramebufferDesc bufferInfo {};
            bufferInfo.width = m_ScreenBufferWidth;
            bufferInfo.height = m_ScreenBufferHeight;
            bufferInfo.attachmentCount = m_DepthTest ? 2 : 1;
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
                    attachments[0] = Renderer::GetSwapchain()->GetImage(i);
                    bufferInfo.attachments = attachments;

                    m_Framebuffers.emplace_back(Framebuffer::Get(bufferInfo));
                }
            }
        }
    
    void ForwardRenderer::SubmitLightSetup(Scene* scene)
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

            memcpy(m_PSSceneUniformBuffer + m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_Lights] + sizeof(Graphics::Light) * numLights, &light, sizeof(Graphics::Light));
            numLights++;
        }

        Maths::Vector4 cameraPos = Maths::Vector4(m_CameraTransform->GetWorldPosition());
        memcpy(m_PSSceneUniformBuffer + m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_CameraPosition], &cameraPos, sizeof(Maths::Vector4));

        auto shadowRenderer = Application::Get().GetRenderGraph()->GetShadowRenderer();
        if(shadowRenderer)
        {
            Maths::Matrix4* shadowTransforms = shadowRenderer->GetShadowProjView();
            Lumos::Maths::Vector4* uSplitDepth = shadowRenderer->GetSplitDepths();
            const Maths::Matrix4& lightView = shadowRenderer->GetLightView();

            memcpy(m_PSSceneUniformBuffer + m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_ViewMatrix], &viewMatrix, sizeof(Maths::Matrix4));
            memcpy(m_PSSceneUniformBuffer + m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_LightView], &lightView, sizeof(Maths::Matrix4));

            memcpy(m_PSSceneUniformBuffer + m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_ShadowTransforms], shadowTransforms, sizeof(Maths::Matrix4) * MAX_SHADOWMAPS);
            memcpy(m_PSSceneUniformBuffer + m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_ShadowSplitDepths], uSplitDepth, sizeof(Maths::Vector4) * MAX_SHADOWMAPS);
            memcpy(m_PSSceneUniformBuffer + m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_BiasMatrix], &m_BiasMatrix, sizeof(Maths::Matrix4));

            float bias = shadowRenderer->GetInitialBias();

            float maxShadowDistance = shadowRenderer->GetMaxShadowDistance();
            float LightSize = shadowRenderer->GetLightSize();
            float transitionFade = shadowRenderer->GetCascadeTransitionFade();
            float shadowFade = shadowRenderer->GetShadowFade();

            memcpy(m_PSSceneUniformBuffer + m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_lightSize], &LightSize, sizeof(float));
            memcpy(m_PSSceneUniformBuffer + m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_shadowFade], &shadowFade, sizeof(float));
            memcpy(m_PSSceneUniformBuffer + m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_cascadeTransitionFade], &transitionFade, sizeof(float));
            memcpy(m_PSSceneUniformBuffer + m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_maxShadowDistance], &maxShadowDistance, sizeof(float));

            memcpy(m_PSSceneUniformBuffer + m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_cubemapMipLevels] + sizeof(int), &bias, sizeof(float));
        }

        int numShadows = shadowRenderer ? int(shadowRenderer->GetShadowMapNum()) : 0;
        auto cubemapMipLevels = m_EnvironmentMap ? m_EnvironmentMap->GetMipMapLevels() : 0;
        memcpy(m_PSSceneUniformBuffer + m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_LightCount], &numLights, sizeof(int));
        memcpy(m_PSSceneUniformBuffer + m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_ShadowCount], &numShadows, sizeof(int));
        memcpy(m_PSSceneUniformBuffer + m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_RenderMode], &m_RenderMode, sizeof(int));
        memcpy(m_PSSceneUniformBuffer + m_PSSceneUniformBufferOffsets[PSSceneUniformIndex_cubemapMipLevels], &cubemapMipLevels, sizeof(int));
    }
    
    void ForwardRenderer::UpdateScreenDescriptorSet()
    {
        std::vector<Graphics::Descriptor> bufferInfos;

        Graphics::Descriptor imageInfo5 = {};
        imageInfo5.texture = { m_PreintegratedFG.get() };
        imageInfo5.binding = 0;
        imageInfo5.type = DescriptorType::IMAGE_SAMPLER;
        imageInfo5.name = "uPreintegratedFG";

        Graphics::Descriptor imageInfo6 = {};
        imageInfo6.texture = { m_EnvironmentMap };
        imageInfo6.binding = 1;
        imageInfo6.type = DescriptorType::IMAGE_SAMPLER;
        imageInfo6.textureType = TextureType::CUBE;
        imageInfo6.name = "uEnvironmentMap";

        Graphics::Descriptor imageInfo7 = {};
        imageInfo7.texture = { m_IrradianceMap };
        imageInfo7.binding = 2;
        imageInfo7.type = DescriptorType::IMAGE_SAMPLER;
        imageInfo7.textureType = TextureType::CUBE;
        imageInfo7.name = "uIrradianceMap";

        Graphics::Descriptor imageInfo8 = {};
        auto shadowRenderer = Application::Get().GetRenderGraph()->GetShadowRenderer();
        if(shadowRenderer)
        {
            imageInfo8.texture = { reinterpret_cast<Texture*>(shadowRenderer->GetTexture()) };
            imageInfo8.binding = 3;
            imageInfo8.type = DescriptorType::IMAGE_SAMPLER;
            imageInfo8.textureType = TextureType::DEPTHARRAY;
            imageInfo8.name = "uShadowMap";
        }

        Graphics::Descriptor imageInfo9 = {};
        imageInfo9.texture = { Application::Get().GetRenderGraph()->GetGBuffer()->GetDepthTexture() };
        imageInfo9.binding = 4;
        imageInfo9.type = DescriptorType::IMAGE_SAMPLER;
        imageInfo9.textureType = TextureType::DEPTH;
        imageInfo9.name = "uDepthSampler";

        bufferInfos.push_back(imageInfo5);
        if(m_EnvironmentMap)
            bufferInfos.push_back(imageInfo6);
        if(m_IrradianceMap)
            bufferInfos.push_back(imageInfo7);
        if(shadowRenderer)
            bufferInfos.push_back(imageInfo8);
        
        if(m_LightUniformBuffer == nullptr)
        {
            m_LightUniformBuffer = Graphics::UniformBuffer::Create();
            m_LightUniformBuffer->Init(m_PSSceneUniformBufferSize, nullptr);
        }


        Graphics::Descriptor bufferInfo = {};
        bufferInfo.name = "UniformBufferLight";
        bufferInfo.buffer = m_LightUniformBuffer;
        bufferInfo.offset = 0;
        bufferInfo.size = m_PSSceneUniformBufferSize;
        bufferInfo.type = Graphics::DescriptorType::UNIFORM_BUFFER;
        bufferInfo.binding = 5;
        bufferInfo.shaderType = ShaderType::FRAGMENT;

        bufferInfos.push_back(bufferInfo);

        m_DescriptorSet[2]->Update(bufferInfos);
    }

    }
}
