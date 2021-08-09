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

#include "CompiledSPV/Headers/ForwardPBRvertspv.hpp"
#include "CompiledSPV/Headers/ForwardPBRfragspv.hpp"
#include <imgui/imgui.h>

#define MAX_LIGHTS 32
#define MAX_SHADOWMAPS 4

namespace Lumos
{
    namespace Graphics
    {
        ForwardRenderer::ForwardRenderer(uint32_t width, uint32_t height, bool depthTest)
        {
            LUMOS_PROFILE_FUNCTION();

            m_DepthTest = depthTest;
            SetScreenBufferSize(width, height);
            
            ForwardRenderer::Init();
        }

        ForwardRenderer::~ForwardRenderer()
        {
            LUMOS_PROFILE_FUNCTION();
            delete m_DefaultMaterial;

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
    
        void ForwardRenderer::Init()
        {
            LUMOS_PROFILE_FUNCTION();

           // m_Shader = Application::Get().GetShaderLibrary()->GetResource("//CoreShaders/ForwardPBR.shader");
            m_Shader = Graphics::Shader::CreateFromEmbeddedArray(spirv_ForwardPBRvertspv.data(), spirv_ForwardPBRvertspv_size, spirv_ForwardPBRfragspv.data(), spirv_ForwardPBRfragspv_size);
            Application::Get().GetShaderLibrary()->AddResource("ForwardPBR", m_Shader);

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

            m_ClearColour = Maths::Vector4(0.2f, 0.2f, 0.2f, 1.0f);

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
            m_DescriptorSet[0]->SetUniform("UniformBufferObject", "projView", &projView);
            m_DescriptorSet[0]->Update();
			
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
                        Maths::Matrix4 textureMatrix;
                        auto textureMatrixTransform = registry.try_get<TextureMatrixComponent>(entity);
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

                    //TODO: remove need for this
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
                pipelineCreateInfo.shader = m_Shader;//command.material->GetShader();
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

        void ForwardRenderer::UpdateScreenDescriptorSet()
        {
			m_DescriptorSet[2]->SetTexture("uPreintegratedFG", m_PreintegratedFG.get());
			m_DescriptorSet[2]->SetTexture("uEnvironmentMap", m_EnvironmentMap, TextureType::CUBE);
			m_DescriptorSet[2]->SetTexture("uIrradianceMap", m_IrradianceMap, TextureType::CUBE);
            auto shadowRenderer = Application::Get().GetRenderGraph()->GetShadowRenderer();
            if(shadowRenderer)
                m_DescriptorSet[2]->SetTexture("uShadowMap", reinterpret_cast<Texture*>(shadowRenderer->GetTexture()) , TextureType::DEPTHARRAY);
            m_DescriptorSet[2]->SetTexture("uDepthSampler", Application::Get().GetRenderGraph()->GetGBuffer()->GetDepthTexture(), TextureType::DEPTH);
            m_DescriptorSet[2]->Update();
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
		
        void ForwardRenderer::OnImGui()
        {
            LUMOS_PROFILE_FUNCTION();			
            ImGui::TextUnformatted("Forward PBR Renderer");
			
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

    }
}
