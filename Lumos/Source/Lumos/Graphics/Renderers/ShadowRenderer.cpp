#include "Precompiled.h"
#include "ShadowRenderer.h"

#include "Graphics/RHI/Texture.h"
#include "Graphics/RHI/Framebuffer.h"
#include "Graphics/RHI/Renderer.h"
#include "Graphics/RHI/CommandBuffer.h"
#include "Graphics/RHI/RenderPass.h"
#include "Graphics/RHI/Pipeline.h"
#include "Graphics/RHI/UniformBuffer.h"
#include "Graphics/RHI/GraphicsContext.h"
#include "Graphics/RHI/Swapchain.h"
#include "Graphics/RHI/Shader.h"

#include "Graphics/Model.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Light.h"
#include "RenderGraph.h"
#include "Maths/Transform.h"
#include "Core/Engine.h"
#include "Scene/Scene.h"
#include "Maths/Maths.h"
#include "RenderCommand.h"
#include "Core/Application.h"

#include <imgui/imgui.h>

//#define THREAD_CASCADE_GEN
#ifdef THREAD_CASCADE_GEN
#include "Core/JobSystem.h"
#endif

namespace Lumos
{
    namespace Graphics
    {
        enum VSSystemUniformIndices : int32_t
        {
            VSSystemUniformIndex_ProjectionViewMatrix = 0,
            VSSystemUniformIndex_Size
        };

        ShadowRenderer::ShadowRenderer(TextureDepthArray* texture, uint32_t shadowMapSize, uint32_t numMaps)
            : m_ShadowTex(nullptr)
            , m_ShadowMapNum(numMaps)
            , m_ShadowMapSize(shadowMapSize)
            , m_ShadowMapsInvalidated(true)
            , m_UniformBuffer(nullptr)
            , m_CascadeSplitLambda(0.91f)
            , m_SceneRadiusMultiplier(1.4f)
        {
            m_Shader = Application::Get().GetShaderLibrary()->GetResource("//CoreShaders/Shadow.shader");
            m_ShadowTex = texture ? texture : TextureDepthArray::Create(m_ShadowMapSize, m_ShadowMapSize, m_ShadowMapNum);

            m_ScreenRenderer = false;

            m_LightSize = 1.5f;
            m_MaxShadowDistance = 400.0f;
            m_ShadowFade = 40.0f;
            m_CascadeTransitionFade = 1.5f;
            m_InitialBias = 0.0013f;

            ShadowRenderer::Init();
            Application::Get().GetRenderGraph()->SetShadowRenderer(this);
        }

        ShadowRenderer::~ShadowRenderer()
        {
            delete m_ShadowTex;
            delete[] m_VSSystemUniformBuffer;
            delete m_UniformBuffer;
        }

        void ShadowRenderer::Init()
        {
            LUMOS_PROFILE_FUNCTION();
            m_VSSystemUniformBufferSize = sizeof(Maths::Matrix4) * SHADOWMAP_MAX;
            m_VSSystemUniformBuffer = new uint8_t[m_VSSystemUniformBufferSize];
            memset(m_VSSystemUniformBuffer, 0, m_VSSystemUniformBufferSize);
            m_VSSystemUniformBufferOffsets.resize(VSSystemUniformIndex_Size);

            // Per Scene System Uniforms
            m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionViewMatrix] = 0;

            AttachmentInfo textureTypes[1] = {
                { TextureType::DEPTHARRAY, TextureFormat::DEPTH }
            };

            Graphics::RenderPassDesc renderpassCI {};
            renderpassCI.attachmentCount = 1;
            renderpassCI.textureType = textureTypes;
            renderpassCI.clear = true;

            m_RenderPass = Graphics::RenderPass::Get(renderpassCI);

            Graphics::DescriptorDesc info {};
            info.layoutIndex = 0;
            info.shader = m_Shader.get();
            m_DescriptorSet.resize(1);
            m_DescriptorSet[0] = SharedRef<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));

            CreateGraphicsPipeline();
            CreateUniformBuffer();
            CreateFramebuffers();
            m_CurrentDescriptorSets.resize(1);

            m_CascadeCommandQueue[0].reserve(1000);
        }

        void ShadowRenderer::OnResize(uint32_t width, uint32_t height)
        {
        }

        void ShadowRenderer::Begin()
        {
            LUMOS_PROFILE_FUNCTION();
        }

        void ShadowRenderer::BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform)
        {
            LUMOS_PROFILE_FUNCTION();

            auto& registry = scene->GetRegistry();
            auto view = registry.view<Graphics::Light>();

            Light* light = nullptr;
            {
                LUMOS_PROFILE_SCOPE("Get Light");
                for(auto& lightEntity : view)
                {
                    auto& currentLight = view.get<Graphics::Light>(lightEntity);
                    if(currentLight.Type == (float)Graphics::LightType::DirectionalLight)
                        light = &currentLight;
                }

                if(!light)
                {
                    m_ShouldRender = false;
                    return;
                }
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
            {
                m_ShouldRender = false;
                return;
            }

            UpdateCascades(scene, overrideCamera, overrideCameraTransform, light);

            m_CascadeCommandQueue[0].clear();
            m_CascadeCommandQueue[1].clear();
            m_CascadeCommandQueue[2].clear();
            m_CascadeCommandQueue[3].clear();

            auto group = registry.group<Model>(entt::get<Maths::Transform>);

            for(uint32_t i = 0; i < m_ShadowMapNum; ++i)
            {
                LUMOS_PROFILE_SCOPE("Submit Meshes");
                m_Layer = i;

                Maths::Frustum f;
                f.Define(m_ShadowProjView[i]);

                if(group.empty())
                    continue;

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
                            auto inside = f.IsInsideFast(bbCopy);

                            if(inside == Maths::Intersection::OUTSIDE)
                                continue;

                            SubmitMesh(mesh.get(), nullptr, worldTransform, Maths::Matrix4(), i);
                        }
                    }
                }
            }

            m_ShouldRender = true;
        }

        void ShadowRenderer::EndScene()
        {
        }

        void ShadowRenderer::End()
        {
            LUMOS_PROFILE_FUNCTION();
        }

        void ShadowRenderer::Present()
        {
            LUMOS_PROFILE_FUNCTION();
            int index = 0;

            m_RenderPass->BeginRenderpass(Renderer::GetSwapchain()->GetCurrentCommandBuffer(), Maths::Vector4(0.0f), m_ShadowFramebuffer[m_Layer].get(), Graphics::INLINE, m_ShadowMapSize, m_ShadowMapSize);

            m_Pipeline->Bind(Renderer::GetSwapchain()->GetCurrentCommandBuffer());

            for(auto& command : m_CascadeCommandQueue[m_Layer])
            {
                Engine::Get().Statistics().NumShadowObjects++;

                Mesh* mesh = command.mesh;

                m_CurrentDescriptorSets[0] = m_DescriptorSet[0].get();

                mesh->GetVertexBuffer()->Bind(Renderer::GetSwapchain()->GetCurrentCommandBuffer(), m_Pipeline.get());
                mesh->GetIndexBuffer()->Bind(Renderer::GetSwapchain()->GetCurrentCommandBuffer());

                uint32_t layer = static_cast<uint32_t>(m_Layer);
                auto trans = command.transform;
                auto& pushConstants = m_Shader->GetPushConstants();
                memcpy(pushConstants[0].data, &trans, sizeof(Maths::Matrix4));
                memcpy(pushConstants[0].data + sizeof(Maths::Matrix4), &layer, sizeof(uint32_t));

                m_Shader->BindPushConstants(Renderer::GetSwapchain()->GetCurrentCommandBuffer(), m_Pipeline.get());

                Renderer::BindDescriptorSets(m_Pipeline.get(), Renderer::GetSwapchain()->GetCurrentCommandBuffer(), 0, m_CurrentDescriptorSets);
                Renderer::DrawIndexed(Renderer::GetSwapchain()->GetCurrentCommandBuffer(), DrawType::TRIANGLE, mesh->GetIndexBuffer()->GetCount());

                mesh->GetVertexBuffer()->Unbind();
                mesh->GetIndexBuffer()->Unbind();

                index++;
            }

            m_RenderPass->EndRenderpass(Renderer::GetSwapchain()->GetCurrentCommandBuffer());
        }

        void ShadowRenderer::SetShadowMapNum(uint32_t num)
        {
            LUMOS_PROFILE_FUNCTION();
            if(m_ShadowMapNum != num && num <= SHADOWMAP_MAX)
            {
                m_ShadowMapNum = num;
                m_ShadowMapsInvalidated = true;
            }
        }

        void ShadowRenderer::SetShadowMapSize(uint32_t size)
        {
            LUMOS_PROFILE_FUNCTION();
            if(!m_ShadowMapsInvalidated)
                m_ShadowMapsInvalidated = (size != m_ShadowMapSize);

            m_ShadowMapSize = size;
        }

        void ShadowRenderer::RenderScene()
        {
            LUMOS_PROFILE_FUNCTION();

            if(!m_ShouldRender)
                return;

            memcpy(m_VSSystemUniformBuffer + m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionViewMatrix], m_ShadowProjView, sizeof(Maths::Matrix4) * SHADOWMAP_MAX);

            Begin();

            for(uint32_t i = 0; i < m_ShadowMapNum; ++i)
            {
                m_Layer = i;
                SetSystemUniforms(m_Shader.get());
                Present();
            }
            End();
        }

        void ShadowRenderer::UpdateCascades(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform, Light* light)
        {
            LUMOS_PROFILE_FUNCTION();
            float cascadeSplits[SHADOWMAP_MAX];

            float nearClip = m_Camera->GetNear();
            float farClip = m_Camera->GetFar();
            float clipRange = farClip - nearClip;

            float minZ = nearClip;
            float maxZ = nearClip + clipRange;
            float range = maxZ - minZ;
            float ratio = maxZ / minZ;
            // Calculate split depths based on view camera frustum
            // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
            for(uint32_t i = 0; i < m_ShadowMapNum; i++)
            {
                float p = static_cast<float>(i + 1) / static_cast<float>(m_ShadowMapNum);
                float log = minZ * std::pow(ratio, p);
                float uniform = minZ + range * p;
                float d = m_CascadeSplitLambda * (log - uniform) + uniform;
                cascadeSplits[i] = (d - nearClip) / clipRange;
            }

#ifdef THREAD_CASCADE_GEN
            System::JobSystem::Context ctx;
            System::JobSystem::Dispatch(ctx, static_cast<uint32_t>(m_ShadowMapNum), 1, [&](JobDispatchArgs args)
#else
            for(uint32_t i = 0; i < m_ShadowMapNum; i++)
#endif
                {
#ifdef THREAD_CASCADE_GEN
                    int i = args.jobIndex;
#endif
                    LUMOS_PROFILE_SCOPE("Create Cascade");
                    float splitDist = cascadeSplits[i];
                    float lastSplitDist = i == 0 ? 0.0f : cascadeSplits[i - 1];

                    Maths::Vector3 frustumCorners[8] = {
                        Maths::Vector3(-1.0f, 1.0f, -1.0f),
                        Maths::Vector3(1.0f, 1.0f, -1.0f),
                        Maths::Vector3(1.0f, -1.0f, -1.0f),
                        Maths::Vector3(-1.0f, -1.0f, -1.0f),
                        Maths::Vector3(-1.0f, 1.0f, 1.0f),
                        Maths::Vector3(1.0f, 1.0f, 1.0f),
                        Maths::Vector3(1.0f, -1.0f, 1.0f),
                        Maths::Vector3(-1.0f, -1.0f, 1.0f),
                    };

                    const Maths::Matrix4 invCam = Maths::Matrix4::Inverse(m_Camera->GetProjectionMatrix() * m_CameraTransform->GetWorldMatrix().Inverse());

                    // Project frustum corners into world space
                    for(uint32_t j = 0; j < 8; j++)
                    {
                        Maths::Vector4 invCorner = invCam * Maths::Vector4(frustumCorners[j], 1.0f);
                        frustumCorners[j] = (invCorner / invCorner.w).ToVector3();
                    }

                    for(uint32_t j = 0; j < 4; j++)
                    {
                        Maths::Vector3 dist = frustumCorners[j + 4] - frustumCorners[j];
                        frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
                        frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist);
                    }

                    // Get frustum center
                    Maths::Vector3 frustumCenter = Maths::Vector3(0.0f);
                    for(uint32_t j = 0; j < 8; j++)
                    {
                        frustumCenter += frustumCorners[j];
                    }
                    frustumCenter /= 8.0f;

                    float radius = 0.0f;
                    for(uint32_t j = 0; j < 8; j++)
                    {
                        float distance = (frustumCorners[j] - frustumCenter).Length();
                        radius = Maths::Max(radius, distance);
                    }
                    radius = std::ceil(radius * 16.0f) / 16.0f;
                    float sceneBoundingRadius = m_Camera->GetShadowBoundingRadius() * m_SceneRadiusMultiplier;
                    //Extend the Z depths to catch shadow casters outside view frustum
                    radius = Maths::Max(radius, sceneBoundingRadius);

                    Maths::Vector3 maxExtents = Maths::Vector3(radius);
                    Maths::Vector3 minExtents = -maxExtents;

                    Maths::Vector3 lightDir = -light->Direction.ToVector3();
                    lightDir.Normalise();
                    Maths::Matrix4 lightViewMatrix = Maths::Quaternion::LookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter).RotationMatrix4();
                    lightViewMatrix.SetTranslation(frustumCenter);

                    Maths::Matrix4 lightOrthoMatrix = Maths::Matrix4::Orthographic(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, -(maxExtents.z - minExtents.z), maxExtents.z - minExtents.z);

                    auto shadowProj = lightOrthoMatrix * lightViewMatrix.Inverse();
                    const bool StabilizeCascades = true;
                    if(StabilizeCascades)
                    {
                        // Create the rounding matrix, by projecting the world-space origin and determining
                        // the fractional offset in texel space
                        Maths::Matrix4 shadowMatrix = shadowProj;
                        Maths::Vector3 shadowOrigin = Maths::Vector3(0.0f);
                        shadowOrigin = (shadowMatrix * Maths::Vector4(shadowOrigin, 1.0f)).ToVector3();
                        shadowOrigin *= (m_ShadowMapSize / 2.0f);

                        Maths::Vector3 roundedOrigin = Maths::VectorRound(shadowOrigin);
                        Maths::Vector3 roundOffset = roundedOrigin - shadowOrigin;
                        roundOffset = roundOffset * (2.0f / m_ShadowMapSize);
                        roundOffset.z = 0.0f;

                        shadowProj.ElementRef(0, 3) += roundOffset.x;
                        shadowProj.ElementRef(1, 3) += roundOffset.y;
                    }
                    // Store split distance and matrix in cascade
                    m_SplitDepth[i] = Maths::Vector4((m_Camera->GetNear() + splitDist * clipRange) * -1.0f);
                    m_ShadowProjView[i] = shadowProj;

                    if(i == 0)
                        m_LightMatrix = lightViewMatrix.Inverse();
                }
#ifdef THREAD_CASCADE_GEN
            );
            System::JobSystem::Wait(ctx);
#endif
        }

        void ShadowRenderer::CreateFramebuffers()
        {
            LUMOS_PROFILE_FUNCTION();
            if(m_ShadowMapsInvalidated && m_ShadowMapNum > 0)
            {
                m_ShadowMapsInvalidated = false;

                for(uint32_t i = 0; i < m_ShadowMapNum; ++i)
                {
                    const uint32_t attachmentCount = 1;
                    TextureType attachmentTypes[attachmentCount];
                    attachmentTypes[0] = TextureType::DEPTHARRAY;

                    FramebufferDesc bufferInfo {};
                    bufferInfo.width = m_ShadowMapSize;
                    bufferInfo.height = m_ShadowMapSize;
                    bufferInfo.attachmentCount = attachmentCount;
                    bufferInfo.renderPass = m_RenderPass.get();
                    bufferInfo.attachmentTypes = attachmentTypes;
                    bufferInfo.layer = i;
                    bufferInfo.screenFBO = false;

                    Texture* attachments[attachmentCount];
                    attachments[0] = m_ShadowTex;
                    bufferInfo.attachments = attachments;

                    m_ShadowFramebuffer[i] = Framebuffer::Get(bufferInfo);
                }
            }
        }

        void ShadowRenderer::CreateGraphicsPipeline()
        {
            LUMOS_PROFILE_FUNCTION();
            Graphics::PipelineDesc pipelineCreateInfo;
            pipelineCreateInfo.shader = m_Shader;
            pipelineCreateInfo.renderpass = m_RenderPass;
            pipelineCreateInfo.cullMode = Graphics::CullMode::NONE;
            pipelineCreateInfo.transparencyEnabled = false;
            pipelineCreateInfo.depthBiasEnabled = true;

            m_Pipeline = Graphics::Pipeline::Get(pipelineCreateInfo);
        }

        void ShadowRenderer::CreateUniformBuffer()
        {
            LUMOS_PROFILE_FUNCTION();
            if(m_UniformBuffer == nullptr)
            {
                m_UniformBuffer = Graphics::UniformBuffer::Create();

                const uint32_t bufferSize = static_cast<uint32_t>(sizeof(UniformBufferObject));
                m_UniformBuffer->Init(bufferSize, nullptr);
            }

            std::vector<Graphics::Descriptor> bufferInfos;

            Graphics::Descriptor bufferInfo;
            bufferInfo.buffer = m_UniformBuffer;
            bufferInfo.offset = 0;
            bufferInfo.name = "UniformBufferObject";
            bufferInfo.size = sizeof(UniformBufferObject);
            bufferInfo.type = Graphics::DescriptorType::UNIFORM_BUFFER;
            bufferInfo.binding = 0;
            bufferInfo.shaderType = ShaderType::VERTEX;

            bufferInfos.push_back(bufferInfo);

            m_DescriptorSet[0]->Update(bufferInfos);
        }

        void ShadowRenderer::SetSystemUniforms(Shader* shader)
        {
            LUMOS_PROFILE_FUNCTION();

            {
                LUMOS_PROFILE_SCOPE("Vertex Uniform Buffer Update");
                m_UniformBuffer->SetData(sizeof(Maths::Matrix4) * m_ShadowMapNum, *&m_VSSystemUniformBuffer);
            }
        }

        void ShadowRenderer::Submit(const RenderCommand& command)
        {
            m_CascadeCommandQueue[0].push_back(command);
        }

        void ShadowRenderer::Submit(const RenderCommand& command, uint32_t cascadeIndex)
        {
            m_CascadeCommandQueue[cascadeIndex].push_back(command);
        }

        void ShadowRenderer::SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix)
        {
            LUMOS_PROFILE_FUNCTION();
            RenderCommand command;
            command.mesh = mesh;
            command.transform = transform;
            command.material = material;
            Submit(command);
        }

        void ShadowRenderer::SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix, uint32_t cascadeIndex)
        {
            LUMOS_PROFILE_FUNCTION();
            RenderCommand command;
            command.mesh = mesh;
            command.transform = transform;
            command.material = material;
            Submit(command, cascadeIndex);
        }

        void ShadowRenderer::OnImGui()
        {
            LUMOS_PROFILE_FUNCTION();
            ImGui::TextUnformatted("Shadow Renderer");
            if(ImGui::TreeNode("Texture"))
            {
                static int index = 0;

                ImGui::InputInt("Texture Array Index", &index);

                index = Maths::Max(0, index);
                index = Maths::Min(index, 3);
                bool flipImage = Graphics::GraphicsContext::GetContext()->FlipImGUITexture();

                ImGui::Image(m_ShadowTex->GetHandleArray(uint32_t(index)), ImVec2(128, 128), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));

                if(ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Image(m_ShadowTex->GetHandleArray(uint32_t(index)), ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
                    ImGui::EndTooltip();
                }

                ImGui::TreePop();
            }

            ImGui::DragFloat("Initial Bias", &m_InitialBias, 0.00005f, 0.0f, 1.0f, "%.6f");
            ImGui::DragFloat("Light Size", &m_LightSize, 0.00005f, 0.0f, 10.0f);
            ImGui::DragFloat("Max Shadow Distance", &m_MaxShadowDistance, 0.05f, 0.0f, 10000.0f);
            ImGui::DragFloat("Shadow Fade", &m_ShadowFade, 0.0005f, 0.0f, 500.0f);
            ImGui::DragFloat("Cascade Transition Fade", &m_CascadeTransitionFade, 0.0005f, 0.0f, 5.0f);

            ImGui::DragFloat("Cascade Split Lambda", &m_CascadeSplitLambda, 0.005f, 0.0f, 3.0f);
            ImGui::DragFloat("Scene Radius Multiplier", &m_SceneRadiusMultiplier, 0.005f, 0.0f, 5.0f);
        }
    }
}
