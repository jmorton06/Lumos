#include "Precompiled.h"
#include "SkyboxRenderer.h"
#include "Graphics/RHI/Shader.h"
#include "Graphics/RHI/Framebuffer.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/RHI/UniformBuffer.h"
#include "Graphics/RHI/Renderer.h"
#include "Graphics/RHI/CommandBuffer.h"
#include "Graphics/RHI/SwapChain.h"
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

#include "CompiledSPV/Headers/Skyboxvertspv.hpp"
#include "CompiledSPV/Headers/Skyboxfragspv.hpp"

#include <imgui/imgui.h>

namespace Lumos
{
    namespace Graphics
    {
        SkyboxRenderer::SkyboxRenderer(uint32_t width, uint32_t height)
            : m_CubeMap(nullptr)
        {
            m_Pipeline = nullptr;

            SetScreenBufferSize(width, height);
            Init();
        }

        SkyboxRenderer::~SkyboxRenderer()
        {
            delete m_Skybox;

            m_Framebuffers.clear();
        }

        void SkyboxRenderer::RenderScene()
        {
            LUMOS_PROFILE_FUNCTION();
            if(!m_CubeMap)
                return;

            Graphics::PipelineDesc pipelineDesc {};
            pipelineDesc.shader = m_Shader;

            pipelineDesc.polygonMode = Graphics::PolygonMode::FILL;
            pipelineDesc.cullMode = Graphics::CullMode::BACK;
            pipelineDesc.transparencyEnabled = false;

            {
                pipelineDesc.depthTarget = reinterpret_cast<Texture*>(Application::Get().GetRenderGraph()->GetGBuffer()->GetDepthTexture());
            }

            if(m_RenderTexture)
                pipelineDesc.colourTargets[0] = m_RenderTexture;
            else
                pipelineDesc.swapchainTarget = true;

            m_Pipeline = Graphics::Pipeline::Get(pipelineDesc);

            m_CurrentBufferID = 0;
            if(!m_RenderTexture)
                m_CurrentBufferID = Renderer::GetSwapChain()->GetCurrentBufferIndex();

            Begin();
            m_Pipeline->Bind(Renderer::GetSwapChain()->GetCurrentCommandBuffer());

            m_CurrentDescriptorSets[0] = m_DescriptorSet[0].get();

            m_Skybox->GetVertexBuffer()->Bind(Renderer::GetSwapChain()->GetCurrentCommandBuffer(), m_Pipeline.get());
            m_Skybox->GetIndexBuffer()->Bind(Renderer::GetSwapChain()->GetCurrentCommandBuffer());

            Renderer::BindDescriptorSets(m_Pipeline.get(), Renderer::GetSwapChain()->GetCurrentCommandBuffer(), 0, m_CurrentDescriptorSets);
            Renderer::DrawIndexed(Renderer::GetSwapChain()->GetCurrentCommandBuffer(), DrawType::TRIANGLE, m_Skybox->GetIndexBuffer()->GetCount());

            m_Skybox->GetVertexBuffer()->Unbind();
            m_Skybox->GetIndexBuffer()->Unbind();

            m_Pipeline->End(Renderer::GetSwapChain()->GetCurrentCommandBuffer());

            End();
        }

        void SkyboxRenderer::Init()
        {
            LUMOS_PROFILE_FUNCTION();
            //m_Shader = Application::Get().GetShaderLibrary()->GetResource("//CoreShaders/Skybox.shader");
            m_Shader = Graphics::Shader::CreateFromEmbeddedArray(spirv_Skyboxvertspv.data(), spirv_Skyboxvertspv_size, spirv_Skyboxfragspv.data(), spirv_Skyboxfragspv_size);
            m_Skybox = Graphics::CreateScreenQuad();

            Graphics::DescriptorDesc descriptorDesc {};
            descriptorDesc.layoutIndex = 0;
            descriptorDesc.shader = m_Shader.get();
            m_DescriptorSet.resize(1);
            m_DescriptorSet[0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
            m_CurrentDescriptorSets.resize(1);

            CreateGraphicsPipeline();
            UpdateUniformBuffer();
            CreateFramebuffers();
        }

        void SkyboxRenderer::Begin()
        {
            LUMOS_PROFILE_FUNCTION();
            //m_RenderPass->BeginRenderpass(Renderer::GetSwapChain()->GetCurrentCommandBuffer(), Maths::Vector4(0.0f), m_Framebuffers[m_CurrentBufferID].get(), Graphics::INLINE, m_ScreenBufferWidth, m_ScreenBufferHeight);
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
            m_DescriptorSet[0]->SetUniform("UniformBufferObject", "invprojview", &invViewProj);
            m_DescriptorSet[0]->Update();
            //memcpy(m_VSSystemUniformBuffer + m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_InverseProjectionViewMatrix], &invViewProj, sizeof(Maths::Matrix4));
        }

        void SkyboxRenderer::End()
        {
            LUMOS_PROFILE_FUNCTION();
            //m_RenderPass->EndRenderpass(Renderer::GetSwapChain()->GetCurrentCommandBuffer());
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
        }

        void SkyboxRenderer::UpdateUniformBuffer()
        {
            LUMOS_PROFILE_FUNCTION();
            m_DescriptorSet[0]->SetTexture("u_CubeMap", m_CubeMap, TextureType::CUBE);
            m_DescriptorSet[0]->Update();
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
