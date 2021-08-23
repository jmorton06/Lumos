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

#include "CompiledSPV/Headers/DeferredColourAnimvertspv.hpp"
#include "CompiledSPV/Headers/DeferredColourvertspv.hpp"
#include "CompiledSPV/Headers/DeferredColourfragspv.hpp"

#include <imgui/imgui.h>

#define MAX_LIGHTS 32
#define MAX_SHADOWMAPS 16
#define MAX_BONES 100

namespace Lumos
{
    namespace Graphics
    {
        DeferredOffScreenRenderer::DeferredOffScreenRenderer(uint32_t width, uint32_t height)
        {
            m_ScreenRenderer = false;

            DeferredOffScreenRenderer::SetScreenBufferSize(width, height);
            DeferredOffScreenRenderer::Init();
        }

        DeferredOffScreenRenderer::~DeferredOffScreenRenderer()
        {
            delete m_DefaultMaterial;
            m_Framebuffers.clear();
        }

        void DeferredOffScreenRenderer::Init()
        {
            LUMOS_PROFILE_FUNCTION();
            m_Shader = Graphics::Shader::CreateFromEmbeddedArray(spirv_DeferredColourvertspv.data(), spirv_DeferredColourvertspv_size, spirv_DeferredColourfragspv.data(), spirv_DeferredColourfragspv_size);
            m_AnimatedShader = Graphics::Shader::CreateFromEmbeddedArray(spirv_DeferredColourAnimvertspv.data(), spirv_DeferredColourAnimvertspv_size, spirv_DeferredColourfragspv.data(), spirv_DeferredColourfragspv_size);

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

            const size_t minUboAlignment = size_t(Graphics::Renderer::GetCapabilities().UniformBufferOffsetAlignment);

            m_CommandQueue.reserve(1000);

            Graphics::DescriptorDesc info {};
            info.layoutIndex = 0;
            info.shader = m_Shader.get();
            m_DescriptorSet.resize(1);
            m_DescriptorSet[0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));

            CreatePipeline();
            CreateBuffer();
            CreateFramebuffer();

            m_DefaultMaterial->CreateDescriptorSet(1);

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
            Present();
            End();
        }

        void DeferredOffScreenRenderer::Begin()
        {
            LUMOS_PROFILE_FUNCTION();
            m_RenderPass->BeginRenderpass(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), Maths::Vector4(0.0f), m_Framebuffers.front().get(), Graphics::INLINE, m_ScreenBufferWidth, m_ScreenBufferHeight);
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
                m_DescriptorSet[0]->SetUniform("UniformBufferObject", "projView", &projView);

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
            command.material = material ? material : m_DefaultMaterial;
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
            m_RenderPass->EndRenderpass(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());
        }

        void DeferredOffScreenRenderer::Present()
        {
            LUMOS_PROFILE_FUNCTION();

            for(uint32_t i = 0; i < static_cast<uint32_t>(m_CommandQueue.size()); i++)
            {
                Engine::Get().Statistics().NumRenderedObjects++;

                auto command = m_CommandQueue[i];
                Mesh* mesh = command.mesh;

                if(!command.material || !command.material->GetShader())
                    continue;

                auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();

                Graphics::PipelineDesc pipelineDesc {};
                pipelineDesc.shader = command.material->GetShader();

                pipelineDesc.polygonMode = Graphics::PolygonMode::FILL;
                pipelineDesc.cullMode = command.material->GetFlag(Material::RenderFlags::TWOSIDED) ? Graphics::CullMode::NONE : Graphics::CullMode::BACK;
                pipelineDesc.transparencyEnabled = command.material->GetFlag(Material::RenderFlags::ALPHABLEND);
                pipelineDesc.blendMode = BlendMode::SrcAlphaOneMinusSrcAlpha;

                auto pipeline = Graphics::Pipeline::Get(pipelineDesc);

                pipeline->Bind(commandBuffer);

                command.material->Bind();

                m_CurrentDescriptorSets[SCENE_DESCRIPTORSET_ID] = m_DescriptorSet[SCENE_DESCRIPTORSET_ID].get();
                m_CurrentDescriptorSets[MATERIAL_DESCRIPTORSET_ID] = command.material->GetDescriptorSet();

                auto trans = command.transform;
                auto& pushConstants = command.material->GetShader()->GetPushConstants()[0];
                pushConstants.SetValue("transform", (void*)&trans);

                command.material->GetShader()->BindPushConstants(commandBuffer, pipeline.get());

                mesh->GetVertexBuffer()->Bind(commandBuffer, pipeline.get());
                mesh->GetIndexBuffer()->Bind(commandBuffer);

                Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, m_CurrentDescriptorSets);
                Renderer::DrawIndexed(commandBuffer, DrawType::TRIANGLE, mesh->GetIndexBuffer()->GetCount());

                mesh->GetVertexBuffer()->Unbind();
                mesh->GetIndexBuffer()->Unbind();
            }
        }

        void DeferredOffScreenRenderer::CreatePipeline()
        {
            LUMOS_PROFILE_FUNCTION();

            Graphics::PipelineDesc pipelineDesc {};
            pipelineDesc.shader = m_Shader;

            pipelineDesc.polygonMode = Graphics::PolygonMode::FILL;
            pipelineDesc.cullMode = Graphics::CullMode::BACK;
            pipelineDesc.transparencyEnabled = false;

            m_Pipeline = Graphics::Pipeline::Get(pipelineDesc);

            Graphics::BufferLayout vertexBufferLayoutAnim;

            Graphics::PipelineDesc pipelineDescAnim {};
            pipelineDescAnim.shader = m_AnimatedShader;
            pipelineDescAnim.polygonMode = Graphics::PolygonMode::FILL;
            pipelineDescAnim.cullMode = Graphics::CullMode::BACK;
            pipelineDescAnim.transparencyEnabled = false;

            m_AnimatedPipeline = Graphics::Pipeline::Get(pipelineDescAnim);
        }

        void DeferredOffScreenRenderer::CreateBuffer()
        {
            LUMOS_PROFILE_FUNCTION();

            // m_AnimatedDescriptorSets[0]->SetBuffer("UniformBufferObjectAnim", m_AnimUniformBuffer);

            //m_AnimatedDescriptorSets->Update(frameBufferDesc);
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

            FramebufferDesc frameBufferDesc {};
            frameBufferDesc.width = m_ScreenBufferWidth;
            frameBufferDesc.height = m_ScreenBufferHeight;
            frameBufferDesc.attachmentCount = attachmentCount;
            frameBufferDesc.renderPass = m_RenderPass.get();
            frameBufferDesc.attachmentTypes = attachmentTypes;

            Texture* attachments[attachmentCount];
            attachments[0] = Application::Get().GetRenderGraph()->GetGBuffer()->GetTexture(SCREENTEX_COLOUR);
            attachments[1] = Application::Get().GetRenderGraph()->GetGBuffer()->GetTexture(SCREENTEX_POSITION);
            attachments[2] = Application::Get().GetRenderGraph()->GetGBuffer()->GetTexture(SCREENTEX_NORMALS);
            attachments[3] = Application::Get().GetRenderGraph()->GetGBuffer()->GetTexture(SCREENTEX_PBR);
            attachments[4] = Application::Get().GetRenderGraph()->GetGBuffer()->GetDepthTexture();
            frameBufferDesc.attachments = attachments;

            m_Framebuffers.push_back(SharedPtr<Framebuffer>(Framebuffer::Get(frameBufferDesc)));
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
