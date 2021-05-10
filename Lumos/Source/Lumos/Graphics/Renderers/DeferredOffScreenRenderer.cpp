#include "Precompiled.h"
#include "DeferredOffScreenRenderer.h"
#include "Scene/Scene.h"
#include "Core/Application.h"
#include "Core/Engine.h"
#include "Scene/Component/TextureMatrixComponent.h"
#include "Maths/Maths.h"
#include "Maths/Transform.h"

#include "RenderGraph.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Mesh.h"
#include "Graphics/Model.h"
#include "Graphics/Material.h"
#include "Graphics/GBuffer.h"

#include "Graphics/API/Shader.h"
#include "Graphics/API/Framebuffer.h"
#include "Graphics/API/Texture.h"
#include "Graphics/API/UniformBuffer.h"
#include "Graphics/API/Renderer.h"
#include "Graphics/API/CommandBuffer.h"
#include "Graphics/API/Swapchain.h"
#include "Graphics/API/RenderPass.h"
#include "Graphics/API/Pipeline.h"
#include "Graphics/API/GraphicsContext.h"

#include <imgui/imgui.h>

#define MAX_LIGHTS 32
#define MAX_SHADOWMAPS 16
#define MAX_BONES 100

namespace Lumos
{
    namespace Graphics
    {
        enum VSSystemUniformIndices : int32_t
        {
            VSSystemUniformIndex_ProjectionViewMatrix = 0,
            VSSystemUniformIndex_Size
        };

        enum PSSystemUniformIndices : int32_t
        {
            PSSystemUniformIndex_Lights = 0,
            PSSystemUniformIndex_Size
        };

        DeferredOffScreenRenderer::DeferredOffScreenRenderer(uint32_t width, uint32_t height)
        {
            m_ScreenRenderer = false;

            DeferredOffScreenRenderer::SetScreenBufferSize(width, height);
            DeferredOffScreenRenderer::Init();
        }

        DeferredOffScreenRenderer::~DeferredOffScreenRenderer()
        {
            delete m_UniformBuffer;
            delete m_AnimUniformBuffer;
            delete m_DefaultMaterial;

            delete[] m_VSSystemUniformBuffer;

            m_Framebuffers.clear();
        }

        void DeferredOffScreenRenderer::Init()
        {
            LUMOS_PROFILE_FUNCTION();
            m_Shader = Application::Get().GetShaderLibrary()->GetResource("/CoreShaders/DeferredColour.shader");
            m_AnimatedShader = Application::Get().GetShaderLibrary()->GetResource("/CoreShaders/DeferredColourAnim.shader");

            m_DefaultMaterial = new Material();

            Graphics::MaterialProperties properties;
            properties.albedoColour = Maths::Vector4(1.0f);
            properties.roughnessColour = Maths::Vector4(0.5f);
            properties.metallicColour = Maths::Vector4(0.5f);
            properties.usingAlbedoMap = 0.0f;
            properties.usingRoughnessMap = 0.0f;
            properties.usingNormalMap = 0.0f;
            properties.usingMetallicMap = 0.0f;
            m_DefaultMaterial->SetMaterialProperites(properties);

            const size_t minUboAlignment = size_t(Graphics::Renderer::GetCapabilities().UniformBufferOffsetAlignment);

            m_UniformBuffer = nullptr;
            m_AnimUniformBuffer = nullptr;

            m_CommandQueue.reserve(1000);

            //
            // Vertex shader System uniforms
            //
            m_VSSystemUniformBufferSize = sizeof(Maths::Matrix4);
            m_VSSystemUniformBuffer = new uint8_t[m_VSSystemUniformBufferSize];
            memset(m_VSSystemUniformBuffer, 0, m_VSSystemUniformBufferSize);
            m_VSSystemUniformBufferOffsets.resize(VSSystemUniformIndex_Size);

            //Animated Vertex shader uniform
            m_VSSystemUniformBufferAnimSize = sizeof(Maths::Matrix4) * MAX_BONES;
            m_VSSystemUniformBufferAnim = new uint8_t[m_VSSystemUniformBufferAnimSize];
            memset(m_VSSystemUniformBufferAnim, 0, m_VSSystemUniformBufferAnimSize);

            // Per Scene System Uniforms
            m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionViewMatrix] = 0;

            AttachmentInfo textureTypesOffScreen[5] = {
                { TextureType::COLOUR, Application::Get().GetRenderGraph()->GetGBuffer()->GetTextureFormat(SCREENTEX_COLOUR) },
                { TextureType::COLOUR, Application::Get().GetRenderGraph()->GetGBuffer()->GetTextureFormat(SCREENTEX_POSITION) },
                { TextureType::COLOUR, Application::Get().GetRenderGraph()->GetGBuffer()->GetTextureFormat(SCREENTEX_NORMALS) },
                { TextureType::COLOUR, Application::Get().GetRenderGraph()->GetGBuffer()->GetTextureFormat(SCREENTEX_PBR) },
                { TextureType::DEPTH, TextureFormat::DEPTH }
            };

            Graphics::RenderPassInfo renderpassCIOffScreen {};
            renderpassCIOffScreen.attachmentCount = 5;
            renderpassCIOffScreen.textureType = textureTypesOffScreen;

            m_RenderPass = Graphics::RenderPass::Get(renderpassCIOffScreen);

            CreatePipeline();
            CreateBuffer();
            CreateFramebuffer();

            m_DefaultMaterial->CreateDescriptorSet(m_Pipeline.get(), 1);

            m_ClearColour = Maths::Vector4(0.1f, 0.1f, 0.1f, 1.0f);
            m_CurrentDescriptorSets.resize(2);
        }

        void DeferredOffScreenRenderer::RenderScene()
        {
            LUMOS_PROFILE_FUNCTION();

            if(m_CommandQueue.empty())
            {
                m_HasRendered = false;
                // return;
            }

            m_HasRendered = true;

            Begin();
            SetSystemUniforms(m_Shader.get());
            Present();
            End();
        }

        void DeferredOffScreenRenderer::Begin()
        {
            LUMOS_PROFILE_FUNCTION();
            m_RenderPass->BeginRenderpass(Renderer::GetSwapchain()->GetCurrentCommandBuffer(), Maths::Vector4(0.0f), m_Framebuffers.front().get(), Graphics::INLINE, m_ScreenBufferWidth, m_ScreenBufferHeight);
        }

        void DeferredOffScreenRenderer::BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform)
        {
            LUMOS_PROFILE_FUNCTION();
            m_CommandQueue.clear();
            {
                LUMOS_PROFILE_SCOPE("Get Camera");

                m_Camera = overrideCamera;
                m_CameraTransform = overrideCameraTransform;

                auto view = m_CameraTransform->GetWorldMatrix().Inverse();

                if(!m_Camera)
                {
                    return;
                }

                LUMOS_ASSERT(m_Camera, "No Camera Set for Renderer");
                auto projView = m_Camera->GetProjectionMatrix() * view;
                memcpy(m_VSSystemUniformBuffer + m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionViewMatrix], &projView, sizeof(Maths::Matrix4));

                m_Frustum = m_Camera->GetFrustum(view);
            }

            {
                auto& registry = scene->GetRegistry();
                auto group = registry.group<Model>(entt::get<Maths::Transform>);

                for(auto entity : group)
                {
                    const auto& [model, trans] = group.get<Model, Maths::Transform>(entity);
                    const auto& meshes = model.GetMeshes();

                    for(auto& mesh : meshes)
                    {

                        if(mesh->GetActive())
                        {

                            auto& worldTransform = trans.GetWorldMatrix();
                            Maths::Intersection inside;
                            {
                                LUMOS_PROFILE_SCOPE("Frustum Check");

                                inside = m_Frustum.IsInsideFast(mesh->GetBoundingBox()->Transformed(worldTransform));
                            }

                            if(inside == Maths::Intersection::OUTSIDE)
                                continue;

                            auto textureMatrixTransform = registry.try_get<TextureMatrixComponent>(entity);
                            SubmitMesh(mesh.get(), mesh->GetMaterial().get(), worldTransform, textureMatrixTransform ? textureMatrixTransform->GetMatrix() : Maths::Matrix4());
                        }
                    }
                }
            }
        }

        void DeferredOffScreenRenderer::Submit(const RenderCommand& command)
        {
            LUMOS_PROFILE_FUNCTION();
            m_CommandQueue.push_back(command);
        }

        void DeferredOffScreenRenderer::SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix)
        {
            LUMOS_PROFILE_FUNCTION();
            RenderCommand command;
            command.mesh = mesh;
            command.material = material;
            command.transform = transform;
            command.textureMatrix = textureMatrix;
            Submit(command);
        }

        void DeferredOffScreenRenderer::EndScene()
        {
        }

        void DeferredOffScreenRenderer::End()
        {
            LUMOS_PROFILE_FUNCTION();
            m_RenderPass->EndRenderpass(Renderer::GetSwapchain()->GetCurrentCommandBuffer());
        }

        void DeferredOffScreenRenderer::SetSystemUniforms(Shader* shader)
        {
            LUMOS_PROFILE_FUNCTION();
            m_UniformBuffer->SetData(m_VSSystemUniformBufferSize, *&m_VSSystemUniformBuffer);
            //Move as per mesh
            m_AnimUniformBuffer->SetData(m_VSSystemUniformBufferAnimSize, *&m_VSSystemUniformBufferAnim);
        }

        void DeferredOffScreenRenderer::Present()
        {
            LUMOS_PROFILE_FUNCTION();
            m_Pipeline->Bind(Renderer::GetSwapchain()->GetCurrentCommandBuffer());

            for(uint32_t i = 0; i < static_cast<uint32_t>(m_CommandQueue.size()); i++)
            {
                Engine::Get().Statistics().NumRenderedObjects++;

                auto command = m_CommandQueue[i];
                Mesh* mesh = command.mesh;

                if(command.material)
                    command.material->Bind(m_Pipeline.get());

                m_CurrentDescriptorSets[0] = m_Pipeline->GetDescriptorSet();
                m_CurrentDescriptorSets[1] = command.material ? command.material->GetDescriptorSet() : m_DefaultMaterial->GetDescriptorSet();

                auto trans = command.transform;
                auto& pushConstants = m_Shader->GetPushConstants();
                memcpy(pushConstants[0].data, &trans, sizeof(Maths::Matrix4));
                m_Shader->BindPushConstants(Renderer::GetSwapchain()->GetCurrentCommandBuffer(), m_Pipeline.get());

                mesh->GetVertexBuffer()->Bind(Renderer::GetSwapchain()->GetCurrentCommandBuffer(), m_Pipeline.get());
                mesh->GetIndexBuffer()->Bind(Renderer::GetSwapchain()->GetCurrentCommandBuffer());

                Renderer::BindDescriptorSets(m_Pipeline.get(), Renderer::GetSwapchain()->GetCurrentCommandBuffer(), 0, m_CurrentDescriptorSets);
                Renderer::DrawIndexed(Renderer::GetSwapchain()->GetCurrentCommandBuffer(), DrawType::TRIANGLE, mesh->GetIndexBuffer()->GetCount());

                mesh->GetVertexBuffer()->Unbind();
                mesh->GetIndexBuffer()->Unbind();
            }
        }

        void DeferredOffScreenRenderer::CreatePipeline()
        {
            LUMOS_PROFILE_FUNCTION();

            Graphics::PipelineInfo pipelineCreateInfo {};
            pipelineCreateInfo.shader = m_Shader;
            pipelineCreateInfo.renderpass = m_RenderPass;
            pipelineCreateInfo.polygonMode = Graphics::PolygonMode::FILL;
            pipelineCreateInfo.cullMode = Graphics::CullMode::BACK;
            pipelineCreateInfo.transparencyEnabled = false;
            pipelineCreateInfo.depthBiasEnabled = false;

            m_Pipeline = Graphics::Pipeline::Get(pipelineCreateInfo);

            Graphics::BufferLayout vertexBufferLayoutAnim;

            Graphics::PipelineInfo pipelineCreateInfoAnim {};
            pipelineCreateInfoAnim.shader = m_AnimatedShader;
            pipelineCreateInfoAnim.renderpass = m_RenderPass;
            pipelineCreateInfoAnim.polygonMode = Graphics::PolygonMode::FILL;
            pipelineCreateInfoAnim.cullMode = Graphics::CullMode::BACK;
            pipelineCreateInfoAnim.transparencyEnabled = false;
            pipelineCreateInfoAnim.depthBiasEnabled = false;

            m_AnimatedPipeline = Graphics::Pipeline::Get(pipelineCreateInfoAnim);
        }

        void DeferredOffScreenRenderer::CreateBuffer()
        {
            LUMOS_PROFILE_FUNCTION();
            if(m_UniformBuffer == nullptr)
            {
                m_UniformBuffer = Graphics::UniformBuffer::Create();

                uint32_t bufferSize = m_VSSystemUniformBufferSize;
                m_UniformBuffer->Init(bufferSize, nullptr);
            }

            if(m_AnimUniformBuffer == nullptr)
            {
                m_AnimUniformBuffer = Graphics::UniformBuffer::Create();

                uint32_t bufferSize = m_VSSystemUniformBufferAnimSize;
                m_AnimUniformBuffer->Init(bufferSize, nullptr);
            }

            std::vector<Graphics::BufferInfo> bufferInfos;

            Graphics::BufferInfo bufferInfo = {};
            bufferInfo.buffer = m_UniformBuffer;
            bufferInfo.offset = 0;
            bufferInfo.size = m_VSSystemUniformBufferSize;
            bufferInfo.type = Graphics::DescriptorType::UNIFORM_BUFFER;
            bufferInfo.binding = 0;
            bufferInfo.shaderType = ShaderType::VERTEX;
            bufferInfo.name = "UniformBufferObject";
            bufferInfos.push_back(bufferInfo);

            m_Pipeline->GetDescriptorSet()->Update(bufferInfos);

            Graphics::BufferInfo bufferInfoAnim = {};
            bufferInfoAnim.buffer = m_AnimUniformBuffer;
            bufferInfoAnim.offset = 0;
            bufferInfoAnim.size = m_VSSystemUniformBufferAnimSize;
            bufferInfoAnim.type = Graphics::DescriptorType::UNIFORM_BUFFER;
            bufferInfoAnim.binding = 1;
            bufferInfoAnim.shaderType = ShaderType::VERTEX;
            bufferInfoAnim.name = "UniformBufferObjectAnim";
            bufferInfos.push_back(bufferInfoAnim);

            m_AnimatedPipeline->GetDescriptorSet()->Update(bufferInfos);
        }

        void DeferredOffScreenRenderer::CreateFramebuffer()
        {
            LUMOS_PROFILE_FUNCTION();
            const uint32_t attachmentCount = 5;
            TextureType attachmentTypes[attachmentCount];
            attachmentTypes[0] = TextureType::COLOUR;
            attachmentTypes[1] = TextureType::COLOUR;
            attachmentTypes[2] = TextureType::COLOUR;
            attachmentTypes[3] = TextureType::COLOUR;
            attachmentTypes[4] = TextureType::DEPTH;

            FramebufferInfo bufferInfo {};
            bufferInfo.width = m_ScreenBufferWidth;
            bufferInfo.height = m_ScreenBufferHeight;
            bufferInfo.attachmentCount = attachmentCount;
            bufferInfo.renderPass = m_RenderPass.get();
            bufferInfo.attachmentTypes = attachmentTypes;

            Texture* attachments[attachmentCount];
            attachments[0] = Application::Get().GetRenderGraph()->GetGBuffer()->GetTexture(SCREENTEX_COLOUR);
            attachments[1] = Application::Get().GetRenderGraph()->GetGBuffer()->GetTexture(SCREENTEX_POSITION);
            attachments[2] = Application::Get().GetRenderGraph()->GetGBuffer()->GetTexture(SCREENTEX_NORMALS);
            attachments[3] = Application::Get().GetRenderGraph()->GetGBuffer()->GetTexture(SCREENTEX_PBR);
            attachments[4] = Application::Get().GetRenderGraph()->GetGBuffer()->GetDepthTexture();
            bufferInfo.attachments = attachments;

            m_Framebuffers.push_back(Ref<Framebuffer>(Framebuffer::Get(bufferInfo)));
        }

        void DeferredOffScreenRenderer::OnResize(uint32_t width, uint32_t height)
        {
            LUMOS_PROFILE_FUNCTION();
            m_Framebuffers.clear();

            DeferredOffScreenRenderer::SetScreenBufferSize(width, height);

            CreateFramebuffer();
        }

        void DeferredOffScreenRenderer::OnImGui()
        {
            ImGui::TextUnformatted("Deferred Offscreen Renderer");
        }
    }
}
