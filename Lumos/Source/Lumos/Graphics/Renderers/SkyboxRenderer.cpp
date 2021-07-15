#include "Precompiled.h"
#include "SkyboxRenderer.h"
#include "Graphics/RHI/Shader.h"
#include "Graphics/RHI/Framebuffer.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/RHI/UniformBuffer.h"
#include "Graphics/RHI/Renderer.h"
#include "Graphics/RHI/CommandBuffer.h"
#include "Graphics/RHI/Swapchain.h"
#include "Graphics/RHI/RenderPass.h"
#include "Graphics/RHI/GraphicsContext.h"
#include "Graphics/RHI/Pipeline.h"
#include "Graphics/GBuffer.h"
#include "Graphics/Mesh.h"
#include "Graphics/MeshFactory.h"
#include "RenderGraph.h"
#include "Scene/Scene.h"
#include "Core/Application.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Environment.h"

#include <imgui/imgui.h>

namespace Lumos
{
    namespace Graphics
    {
        SkyboxRenderer::SkyboxRenderer(uint32_t width, uint32_t height)
            : m_UniformBuffer(nullptr)
            , m_CubeMap(nullptr)
        {
            m_Pipeline = nullptr;

            SetScreenBufferSize(width, height);
            Init();
        }

        SkyboxRenderer::~SkyboxRenderer()
        {
            delete m_UniformBuffer;
            delete m_Skybox;
            delete[] m_VSSystemUniformBuffer;

            m_Framebuffers.clear();
        }

        void SkyboxRenderer::RenderScene()
        {
            LUMOS_PROFILE_FUNCTION();
            if(!m_CubeMap)
                return;

            m_CurrentBufferID = 0;
            if(!m_RenderTexture)
                m_CurrentBufferID = Renderer::GetSwapchain()->GetCurrentBufferIndex();

            Begin();
            SetSystemUniforms(m_Shader.get());
            m_Pipeline->Bind(Renderer::GetSwapchain()->GetCurrentCommandBuffer());

            m_CurrentDescriptorSets[0] = m_DescriptorSet[0].get();

            m_Skybox->GetVertexBuffer()->Bind(Renderer::GetSwapchain()->GetCurrentCommandBuffer(), m_Pipeline.get());
            m_Skybox->GetIndexBuffer()->Bind(Renderer::GetSwapchain()->GetCurrentCommandBuffer());

            Renderer::BindDescriptorSets(m_Pipeline.get(), Renderer::GetSwapchain()->GetCurrentCommandBuffer(), 0, m_CurrentDescriptorSets);
            Renderer::DrawIndexed(Renderer::GetSwapchain()->GetCurrentCommandBuffer(), DrawType::TRIANGLE, m_Skybox->GetIndexBuffer()->GetCount());

            m_Skybox->GetVertexBuffer()->Unbind();
            m_Skybox->GetIndexBuffer()->Unbind();

            End();

            //if(!m_RenderTexture)
            //Renderer::Present((m_CommandBuffers[m_CurrentBufferID].get()));
        }

        enum VSSystemUniformIndices : int32_t
        {
            VSSystemUniformIndex_InverseProjectionViewMatrix = 0,
            VSSystemUniformIndex_Size
        };

        void SkyboxRenderer::Init()
        {
            LUMOS_PROFILE_FUNCTION();
            m_Shader = Application::Get().GetShaderLibrary()->GetResource("//CoreShaders/Skybox.shader");
            m_Skybox = Graphics::CreateScreenQuad();

            // Vertex shader System uniforms
            m_VSSystemUniformBufferSize = sizeof(Maths::Matrix4);
            m_VSSystemUniformBuffer = new uint8_t[m_VSSystemUniformBufferSize];
            memset(m_VSSystemUniformBuffer, 0, m_VSSystemUniformBufferSize);
            m_VSSystemUniformBufferOffsets.resize(VSSystemUniformIndex_Size);

            // Per Scene System Uniforms
            m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_InverseProjectionViewMatrix] = 0;

            AttachmentInfo textureTypes[2] = {
                { TextureType::COLOUR, TextureFormat::RGBA8 },
                { TextureType::DEPTH, TextureFormat::DEPTH }
            };

            Graphics::RenderPassDesc renderpassCI {};
            renderpassCI.attachmentCount = 2;
            renderpassCI.textureType = textureTypes;
            renderpassCI.clear = false;

            m_RenderPass = Graphics::RenderPass::Get(renderpassCI);

            Graphics::DescriptorDesc info {};
            info.layoutIndex = 0;
            info.shader = m_Shader.get();
            m_DescriptorSet.resize(1);
            m_DescriptorSet[0] = SharedRef<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));
            m_CurrentDescriptorSets.resize(1);

            CreateGraphicsPipeline();
            UpdateUniformBuffer();
            CreateFramebuffers();
        }

        void SkyboxRenderer::Begin()
        {
            LUMOS_PROFILE_FUNCTION();
            m_RenderPass->BeginRenderpass(Renderer::GetSwapchain()->GetCurrentCommandBuffer(), Maths::Vector4(0.0f), m_Framebuffers[m_CurrentBufferID].get(), Graphics::INLINE, m_ScreenBufferWidth, m_ScreenBufferHeight);
        }

        void SkyboxRenderer::BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform)
        {
            LUMOS_PROFILE_FUNCTION();
            auto& registry = scene->GetRegistry();

            auto view = registry.view<Graphics::Environment>();

            if(view.size() != 0)
            {
                //Just use first
                const auto& env = view.get<Graphics::Environment>(view.front());

                if(m_CubeMap != env.GetEnvironmentMap())
                {
                    m_CubeMap = env.GetEnvironmentMap();
                    UpdateUniformBuffer();
                }
            }
            else
            {
                m_CubeMap = nullptr;
                return;
            }

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

            auto invViewProj = Maths::Matrix4::Inverse(m_Camera->GetProjectionMatrix() * m_CameraTransform->GetWorldMatrix().Inverse());
            memcpy(m_VSSystemUniformBuffer + m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_InverseProjectionViewMatrix], &invViewProj, sizeof(Maths::Matrix4));
        }

        void SkyboxRenderer::End()
        {
            LUMOS_PROFILE_FUNCTION();
            m_RenderPass->EndRenderpass(Renderer::GetSwapchain()->GetCurrentCommandBuffer());
        }

        void SkyboxRenderer::SetSystemUniforms(Shader* shader) const
        {
            LUMOS_PROFILE_FUNCTION();
            m_UniformBuffer->SetData(sizeof(UniformBufferObject), *&m_VSSystemUniformBuffer);
        }

        void SkyboxRenderer::OnResize(uint32_t width, uint32_t height)
        {
            LUMOS_PROFILE_FUNCTION();
            m_Framebuffers.clear();

            SetScreenBufferSize(width, height);

            m_CubeMap = nullptr;
            UpdateUniformBuffer();
            CreateFramebuffers();
        }

        void SkyboxRenderer::CreateGraphicsPipeline()
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

        void SkyboxRenderer::UpdateUniformBuffer()
        {
            LUMOS_PROFILE_FUNCTION();
            if(m_UniformBuffer == nullptr)
            {
                m_UniformBuffer = Graphics::UniformBuffer::Create();
                uint32_t bufferSize = static_cast<uint32_t>(sizeof(UniformBufferObject));
                m_UniformBuffer->Init(bufferSize, nullptr);
            }

            std::vector<Graphics::Descriptor> bufferInfos;

            Graphics::Descriptor bufferInfo = {};
            bufferInfo.buffer = m_UniformBuffer;
            bufferInfo.offset = 0;
            bufferInfo.size = sizeof(UniformBufferObject);
            bufferInfo.type = Graphics::DescriptorType::UNIFORM_BUFFER;
            bufferInfo.binding = 0;
            bufferInfo.shaderType = ShaderType::VERTEX;

            bufferInfos.push_back(bufferInfo);

            if(m_CubeMap)
            {
                Graphics::Descriptor imageInfo = {};
                imageInfo.texture = m_CubeMap;
                imageInfo.name = "u_CubeMap";
                imageInfo.binding = 1;
                imageInfo.textureType = TextureType::CUBE;
                imageInfo.type = DescriptorType::IMAGE_SAMPLER;

                bufferInfos.push_back(imageInfo);
            }

            if(m_Pipeline != nullptr)
                m_DescriptorSet[0].get()->Update(bufferInfos);
        }

        void SkyboxRenderer::SetCubeMap(Texture* cubeMap)
        {
            LUMOS_PROFILE_FUNCTION();
            m_CubeMap = cubeMap;
            UpdateUniformBuffer();
        }

        void SkyboxRenderer::SetRenderTarget(Texture* texture, bool rebuildFramebuffer)
        {
            LUMOS_PROFILE_FUNCTION();
            m_RenderTexture = texture;

            if(rebuildFramebuffer)
            {
                m_Framebuffers.clear();

                CreateFramebuffers();
            }
        }

        void SkyboxRenderer::CreateFramebuffers()
        {
            LUMOS_PROFILE_FUNCTION();
            TextureType attachmentTypes[2];
            attachmentTypes[0] = TextureType::COLOUR;
            attachmentTypes[1] = TextureType::DEPTH;

            Texture* attachments[2];
            FramebufferDesc bufferInfo {};
            bufferInfo.width = m_ScreenBufferWidth;
            bufferInfo.height = m_ScreenBufferHeight;
            bufferInfo.attachmentCount = 2;
            bufferInfo.renderPass = m_RenderPass.get();
            bufferInfo.attachmentTypes = attachmentTypes;

            attachments[1] = dynamic_cast<Texture*>(Application::Get().GetRenderGraph()->GetGBuffer()->GetDepthTexture());

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

        void SkyboxRenderer::OnImGui()
        {
            LUMOS_PROFILE_FUNCTION();
            ImGui::TextUnformatted("Skybox Renderer");
            if(ImGui::TreeNode("CubeMap"))
            {
                bool flipImage = Graphics::GraphicsContext::GetContext()->FlipImGUITexture();

                ImGui::Image(m_CubeMap->GetHandle(), ImVec2(128, 128), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));

                if(ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Image(m_CubeMap->GetHandle(), ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
                    ImGui::EndTooltip();
                }

                ImGui::TreePop();
            }
        }
    }
}
