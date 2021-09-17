#include "Precompiled.h"
#include "RenderGraph.h"
#include "Scene/Component/ModelComponent.h"
#include "Graphics/GBuffer.h"
#include "Graphics/Model.h"
#include "Graphics/Renderers/IRenderer.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Graphics/Light.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Environment.h"
#include "Graphics/Sprite.h"
#include "Graphics/AnimatedSprite.h"

//Vulkan Only
#define LUMOS_PROFILE_GPU
#include "Graphics/RHI/GPUProfile.h"

#include "Events/ApplicationEvent.h"

#include "CompiledSPV/Headers/Shadowvertspv.hpp"
#include "CompiledSPV/Headers/Shadowfragspv.hpp"
#include "CompiledSPV/Headers/ShadowAnimvertspv.hpp"

#include "CompiledSPV/Headers/ForwardPBRvertspv.hpp"
#include "CompiledSPV/Headers/ForwardPBRfragspv.hpp"
#include "Embedded/BRDFTexture.inl"
#include "Embedded/CheckerBoardTextureArray.inl"

#include "CompiledSPV/Headers/Skyboxvertspv.hpp"
#include "CompiledSPV/Headers/Skyboxfragspv.hpp"

#include "CompiledSPV/Headers/Batch2Dvertspv.hpp"
#include "CompiledSPV/Headers/Batch2Dfragspv.hpp"

#include "CompiledSPV/Headers/Batch2DPointvertspv.hpp"
#include "CompiledSPV/Headers/Batch2DPointfragspv.hpp"

#include "CompiledSPV/Headers/Batch2DLinevertspv.hpp"
#include "CompiledSPV/Headers/Batch2DLinefragspv.hpp"

#include <imgui/imgui.h>

static const uint32_t MaxPoints = 10000;
static const uint32_t MaxPointVertices = MaxPoints * 4;
static const uint32_t MaxPointIndices = MaxPoints * 6;
static const uint32_t MAX_BATCH_DRAW_CALLS = 100;
static const uint32_t RENDERER_POINT_SIZE = sizeof(Lumos::Graphics::PointVertexData) * 4;
static const uint32_t RENDERER_POINT_BUFFER_SIZE = RENDERER_POINT_SIZE * MaxPointVertices;

static const uint32_t MaxLines = 10000;
static const uint32_t MaxLineVertices = MaxLines * 2;
static const uint32_t MaxLineIndices = MaxLines * 6;
static const uint32_t MAX_LINE_BATCH_DRAW_CALLS = 100;
static const uint32_t RENDERER_LINE_SIZE = sizeof(Lumos::Graphics::LineVertexData) * 4;
static const uint32_t RENDERER_LINE_BUFFER_SIZE = RENDERER_LINE_SIZE * MaxLineVertices;

namespace Lumos::Graphics
{
    RenderGraph::RenderGraph(uint32_t width, uint32_t height)
    {
        LUMOS_PROFILE_FUNCTION();
        SetScreenBufferSize(width, height);

        m_CubeMap = nullptr;
        m_GBuffer = new GBuffer(width, height);
        Reset();

        //Setup shadow pass data
        m_ShadowData.m_ShadowTex = (nullptr);
        m_ShadowData.m_ShadowMapNum = (4);
        m_ShadowData.m_ShadowMapSize = (4096);
        m_ShadowData.m_ShadowMapsInvalidated = (true);
        m_ShadowData.m_CascadeSplitLambda = (0.92f);
        m_ShadowData.m_SceneRadiusMultiplier = (1.4f);
        // m_Shader = Application::Get().GetShaderLibrary()->GetResource("//CoreShaders/Shadow.shader");
        m_ShadowData.m_Shader = Graphics::Shader::CreateFromEmbeddedArray(spirv_Shadowvertspv.data(), spirv_Shadowvertspv_size, spirv_Shadowfragspv.data(), spirv_Shadowfragspv_size);

        m_ShadowData.m_ShadowTex = TextureDepthArray::Create(m_ShadowData.m_ShadowMapSize, m_ShadowData.m_ShadowMapSize, m_ShadowData.m_ShadowMapNum);

        m_ShadowData.m_LightSize = 1.5f;
        m_ShadowData.m_MaxShadowDistance = 400.0f;
        m_ShadowData.m_ShadowFade = 40.0f;
        m_ShadowData.m_CascadeTransitionFade = 3.0f;
        m_ShadowData.m_InitialBias = 0.0023f;

        Graphics::DescriptorDesc descriptorDesc {};
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_ShadowData.m_Shader.get();
        m_ShadowData.m_DescriptorSet.resize(1);
        m_ShadowData.m_DescriptorSet[0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        m_ShadowData.m_CurrentDescriptorSets.resize(1);

        m_ShadowData.m_CascadeCommandQueue[0].reserve(1000);
        m_ShadowData.m_CascadeCommandQueue[1].reserve(1000);
        m_ShadowData.m_CascadeCommandQueue[2].reserve(1000);
        m_ShadowData.m_CascadeCommandQueue[3].reserve(1000);

        //Setup forward pass data
        m_ForwardData.m_DepthTest = true;
        m_ForwardData.m_Shader = Graphics::Shader::CreateFromEmbeddedArray(spirv_ForwardPBRvertspv.data(), spirv_ForwardPBRvertspv_size, spirv_ForwardPBRfragspv.data(), spirv_ForwardPBRfragspv_size);
        Application::Get().GetShaderLibrary()->AddResource("ForwardPBR", m_ForwardData.m_Shader);

        m_ForwardData.m_CommandQueue.reserve(1000);

        switch(Graphics::GraphicsContext::GetRenderAPI())
        {
#ifdef LUMOS_RENDER_API_OPENGL
        case Graphics::RenderAPI::OPENGL:
            m_ForwardData.m_BiasMatrix = Maths::Matrix4(0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f);
            break;
#endif

#ifdef LUMOS_RENDER_API_VULKAN
        case Graphics::RenderAPI::VULKAN:
            m_ForwardData.m_BiasMatrix = Maths::Matrix4(0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            break;
#endif

#ifdef LUMOS_RENDER_API_DIRECT3D
        case Graphics::RenderAPI::DIRECT3D:
            m_ForwardData.m_BiasMatrix = Maths::Matrix4(0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
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
        m_ForwardData.m_PreintegratedFG = UniquePtr<Texture2D>(Texture2D::CreateFromSource(BRDFTextureWidth, BRDFTextureHeight, (void*)BRDFTexture, param));

        m_ForwardData.m_ClearColour = Maths::Vector4(0.2f, 0.2f, 0.2f, 1.0f);

        auto descriptorSetScene = m_ForwardData.m_Shader->GetDescriptorInfo(2);
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_ForwardData.m_Shader.get();
        m_ForwardData.m_DescriptorSet.resize(3);
        m_ForwardData.m_DescriptorSet[0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        descriptorDesc.layoutIndex = 2;
        m_ForwardData.m_DescriptorSet[2] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_ForwardData.m_DefaultMaterial = new Material(m_ForwardData.m_Shader);

        Graphics::MaterialProperties properties;
        properties.albedoColour = Maths::Vector4(1.0f);
        properties.roughnessColour = Maths::Vector4(0.5f);
        properties.metallicColour = Maths::Vector4(0.5f);
        properties.usingAlbedoMap = 0.0f;
        properties.usingRoughnessMap = 0.0f;
        properties.usingNormalMap = 0.0f;
        properties.usingMetallicMap = 0.0f;

        m_ForwardData.m_DefaultMaterial->SetMaterialProperites(properties);
        m_ForwardData.m_DefaultMaterial->CreateDescriptorSet(1);

        m_ForwardData.m_CurrentDescriptorSets.resize(3);

        //Set up skybox pass data
        m_SkyboxShader = Graphics::Shader::CreateFromEmbeddedArray(spirv_Skyboxvertspv.data(), spirv_Skyboxvertspv_size, spirv_Skyboxfragspv.data(), spirv_Skyboxfragspv_size);
        m_SkyboxMesh = Graphics::CreateScreenQuad();

        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_SkyboxShader.get();
        m_SkyboxDescriptorSet = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        //Setup 2D pass data
        m_Renderer2DData.m_IndexCount = (0);
        m_Renderer2DData.m_Buffer = (nullptr);
        m_Renderer2DData.m_Clear = (false);
        m_Renderer2DData.m_RenderToDepthTexture = (true);
        m_Renderer2DData.m_TriangleIndicies = (false);
        m_Renderer2DData.m_Limits.SetMaxQuads(10000);
        m_Renderer2DData.m_Limits.MaxTextures = 16; //Renderer::GetCapabilities().MaxTextureUnits;

        m_Renderer2DData.m_Shader = Graphics::Shader::CreateFromEmbeddedArray(spirv_Batch2Dvertspv.data(), spirv_Batch2Dvertspv_size, spirv_Batch2Dfragspv.data(), spirv_Batch2Dfragspv_size);

        m_Renderer2DData.m_TransformationStack.emplace_back(Maths::Matrix4());
        m_Renderer2DData.m_TransformationBack = &m_Renderer2DData.m_TransformationStack.back();

        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_Renderer2DData.m_Shader.get();
        m_Renderer2DData.m_DescriptorSet.resize(2);
        m_Renderer2DData.m_DescriptorSet[0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        descriptorDesc.layoutIndex = 1;
        m_Renderer2DData.m_DescriptorSet[1] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_Renderer2DData.m_VertexBuffers.resize(3);

        for(int i = 0; i < Renderer::GetMainSwapChain()->GetSwapChainBufferCount(); i++)
        {
            m_Renderer2DData.m_VertexBuffers[i].resize(m_Renderer2DData.m_Limits.MaxBatchDrawCalls);

            for(uint32_t j = 0; j < m_Renderer2DData.m_Limits.MaxBatchDrawCalls; j++)
            {
                m_Renderer2DData.m_VertexBuffers[i][j] = Graphics::VertexBuffer::Create(BufferUsage::DYNAMIC);
                m_Renderer2DData.m_VertexBuffers[i][j]->Resize(m_Renderer2DData.m_Limits.BufferSize);
            }
        }

        uint32_t* indices = new uint32_t[m_Renderer2DData.m_Limits.IndiciesSize];

        if(m_Renderer2DData.m_TriangleIndicies)
        {
            for(uint32_t i = 0; i < m_Renderer2DData.m_Limits.IndiciesSize; i++)
            {
                indices[i] = i;
            }
        }
        else
        {
            uint32_t offset = 0;
            for(uint32_t i = 0; i < m_Renderer2DData.m_Limits.IndiciesSize; i += 6)
            {
                indices[i] = offset + 0;
                indices[i + 1] = offset + 1;
                indices[i + 2] = offset + 2;

                indices[i + 3] = offset + 2;
                indices[i + 4] = offset + 3;
                indices[i + 5] = offset + 0;

                offset += 4;
            }
        }
        m_Renderer2DData.m_IndexBuffer = IndexBuffer::Create(indices, m_Renderer2DData.m_Limits.IndiciesSize);

        delete[] indices;

        m_Renderer2DData.m_CurrentDescriptorSets.resize(2);

        //Debug Render

        //Points
        m_DebugDrawData.m_PointShader = Graphics::Shader::CreateFromEmbeddedArray(spirv_Batch2DPointvertspv.data(), spirv_Batch2DPointvertspv_size, spirv_Batch2DPointfragspv.data(), spirv_Batch2DPointfragspv_size);

        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_DebugDrawData.m_PointShader.get();
        m_DebugDrawData.m_PointDescriptorSet.resize(1);
        m_DebugDrawData.m_PointDescriptorSet[0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_DebugDrawData.m_PointVertexBuffers.resize(MAX_BATCH_DRAW_CALLS);

        for(auto& vertexBuffer : m_DebugDrawData.m_PointVertexBuffers)
        {
            vertexBuffer = Graphics::VertexBuffer::Create(BufferUsage::DYNAMIC);
            vertexBuffer->Resize(RENDERER_POINT_BUFFER_SIZE);
        }

        indices = new uint32_t[MaxPointIndices];

        int32_t offset = 0;
        for(int32_t i = 0; i < MaxPointIndices; i += 6)
        {
            indices[i] = offset + 0;
            indices[i + 1] = offset + 1;
            indices[i + 2] = offset + 2;

            indices[i + 3] = offset + 2;
            indices[i + 4] = offset + 3;
            indices[i + 5] = offset + 0;

            offset += 4;
        }

        m_DebugDrawData.m_PointIndexBuffer = IndexBuffer::Create(indices, MaxPointIndices);

        //Lines
        m_DebugDrawData.m_LineShader = Graphics::Shader::CreateFromEmbeddedArray(spirv_Batch2DLinevertspv.data(), spirv_Batch2DLinevertspv_size, spirv_Batch2DLinefragspv.data(), spirv_Batch2DLinefragspv_size);

        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_DebugDrawData.m_LineShader.get();
        m_DebugDrawData.m_LineDescriptorSet.resize(1);
        m_DebugDrawData.m_LineDescriptorSet[0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_DebugDrawData.m_LineVertexBuffers.resize(MAX_BATCH_DRAW_CALLS);
        for(auto& vertexBuffer : m_DebugDrawData.m_LineVertexBuffers)
        {
            vertexBuffer = Graphics::VertexBuffer::Create(BufferUsage::DYNAMIC);
            vertexBuffer->Resize(RENDERER_LINE_BUFFER_SIZE);
        }

        indices = new uint32_t[MaxLineIndices];

        for(int32_t i = 0; i < MaxLineIndices; i++)
        {
            indices[i] = i;
        }

        m_DebugDrawData.m_LineIndexBuffer = IndexBuffer::Create(indices, MaxLineIndices);
        delete[] indices;

        //Debug quads
        m_DebugDrawData.m_Renderer2DData.m_IndexCount = (0);
        m_DebugDrawData.m_Renderer2DData.m_Buffer = (nullptr);
        m_DebugDrawData.m_Renderer2DData.m_Clear = (false);
        m_DebugDrawData.m_Renderer2DData.m_RenderToDepthTexture = (true);
        m_DebugDrawData.m_Renderer2DData.m_TriangleIndicies = (false);
        m_DebugDrawData.m_Renderer2DData.m_Limits.SetMaxQuads(10000);

        m_DebugDrawData.m_Renderer2DData.m_Shader = Graphics::Shader::CreateFromEmbeddedArray(spirv_Batch2Dvertspv.data(), spirv_Batch2Dvertspv_size, spirv_Batch2Dfragspv.data(), spirv_Batch2Dfragspv_size);

        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_DebugDrawData.m_Renderer2DData.m_Shader.get();
        m_DebugDrawData.m_Renderer2DData.m_DescriptorSet.resize(2);
        m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        descriptorDesc.layoutIndex = 1;
        m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[1] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_DebugDrawData.m_Renderer2DData.m_VertexBuffers.resize(3);

        for(int i = 0; i < Renderer::GetMainSwapChain()->GetSwapChainBufferCount(); i++)
        {
            m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[i].resize(m_Renderer2DData.m_Limits.MaxBatchDrawCalls);

            for(uint32_t j = 0; j < m_DebugDrawData.m_Renderer2DData.m_Limits.MaxBatchDrawCalls; j++)
            {
                m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[i][j] = Graphics::VertexBuffer::Create(BufferUsage::DYNAMIC);
                m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[i][j]->Resize(m_DebugDrawData.m_Renderer2DData.m_Limits.BufferSize);
            }
        }

        indices = new uint32_t[m_DebugDrawData.m_Renderer2DData.m_Limits.IndiciesSize];

        {
            for(uint32_t i = 0; i < m_DebugDrawData.m_Renderer2DData.m_Limits.IndiciesSize; i++)
            {
                indices[i] = i;
            }
        }
        m_DebugDrawData.m_Renderer2DData.m_IndexBuffer = IndexBuffer::Create(indices, m_Renderer2DData.m_Limits.IndiciesSize);

        delete[] indices;

        m_DebugDrawData.m_Renderer2DData.m_CurrentDescriptorSets.resize(2);
    }

    RenderGraph::~RenderGraph()
    {
        delete m_GBuffer;
        for(auto renderer : m_Renderers)
        {
            delete renderer;
        }

        delete m_ShadowData.m_ShadowTex;
        delete m_ForwardData.m_DefaultMaterial;
        delete m_SkyboxMesh;

        delete m_Renderer2DData.m_IndexBuffer;

        for(uint32_t i = 0; i < m_Renderer2DData.m_Limits.MaxBatchDrawCalls; i++)
        {
            for(int j = 0; j < Renderer::GetMainSwapChain()->GetSwapChainBufferCount(); j++)
                delete m_Renderer2DData.m_VertexBuffers[j][i];
        }

        delete m_DebugDrawData.m_Renderer2DData.m_IndexBuffer;

        for(uint32_t i = 0; i < m_DebugDrawData.m_Renderer2DData.m_Limits.MaxBatchDrawCalls; i++)
        {
            for(int j = 0; j < Renderer::GetMainSwapChain()->GetSwapChainBufferCount(); j++)
                delete m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[j][i];
        }

        delete m_DebugDrawData.m_LineIndexBuffer;
        delete m_DebugDrawData.m_PointIndexBuffer;

        for(int i = 0; i < MAX_LINE_BATCH_DRAW_CALLS; i++)
        {
            delete m_DebugDrawData.m_PointVertexBuffers[i];
            delete m_DebugDrawData.m_LineVertexBuffers[i];
        }

        DebugRenderer::Release();
    }

    void RenderGraph::OnResize(uint32_t width, uint32_t height)
    {
        LUMOS_PROFILE_FUNCTION();
        SetScreenBufferSize(width, height);
        m_GBuffer->UpdateTextureSize(width, height);
    }

    void RenderGraph::EnableDebugRenderer(bool enable)
    {
        if(enable)
            DebugRenderer::Init();
        else
            DebugRenderer::Release();
    }

    void RenderGraph::BeginScene(Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& registry = scene->GetRegistry();

        if(m_OverrideCamera)
        {
            m_Camera = m_OverrideCamera;
            m_CameraTransform = m_OverrideCameraTransform;
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
            return;
        }

        auto view = m_CameraTransform->GetWorldMatrix().Inverse();
        auto projView = m_Camera->GetProjectionMatrix() * view;

        Scene::SceneRenderSettings& renderSettings = scene->GetSettings().RenderSettings;

        if(renderSettings.Renderer3DEnabled)
        {
            m_ForwardData.m_DescriptorSet[0]->SetUniform("UniformBufferObject", "projView", &projView);
            m_ForwardData.m_DescriptorSet[0]->Update();
        }

        if(renderSettings.SkyboxRenderEnabled || renderSettings.Renderer3DEnabled)
        {
            auto envView = registry.view<Graphics::Environment>();

            if(envView.size() == 0)
            {
                if(m_ForwardData.m_EnvironmentMap)
                {
                    m_ForwardData.m_EnvironmentMap = nullptr;
                    m_ForwardData.m_IrradianceMap = nullptr;

                    //TODO: remove need for this
                    Graphics::DescriptorDesc info {};
                    info.shader = m_ForwardData.m_Shader.get();
                    info.layoutIndex = 2;
                    m_ForwardData.m_DescriptorSet[2] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));
                    m_CubeMap = nullptr;
                    Graphics::DescriptorDesc descriptorDesc {};
                    descriptorDesc.layoutIndex = 0;
                    descriptorDesc.shader = m_SkyboxShader.get();
                    m_SkyboxDescriptorSet = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
                }
            }
            else
            {
                //Just use first
                const auto& env = envView.get<Graphics::Environment>(envView.front());

                if(m_ForwardData.m_EnvironmentMap != env.GetEnvironmentMap())
                {
                    Graphics::DescriptorDesc info {};
                    info.shader = m_ForwardData.m_Shader.get();
                    info.layoutIndex = 2;
                    m_ForwardData.m_DescriptorSet[2] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));
                    m_ForwardData.m_EnvironmentMap = env.GetEnvironmentMap();
                    m_ForwardData.m_IrradianceMap = env.GetIrradianceMap();

                    m_CubeMap = env.GetEnvironmentMap();
                }
            }

            auto invViewProj = Maths::Matrix4::Inverse(m_Camera->GetProjectionMatrix() * m_CameraTransform->GetWorldMatrix().Inverse());
            m_SkyboxDescriptorSet->SetUniform("UniformBufferObject", "invprojview", &invViewProj);
        }

        Light* directionaLight = nullptr;
        static Light lights[256];
        uint32_t numLights = 0;

        if(renderSettings.Renderer3DEnabled)
        {
            m_ForwardData.m_Frustum = m_Camera->GetFrustum(view);
            {
                LUMOS_PROFILE_SCOPE("Get Light");
                auto group = registry.group<Graphics::Light>(entt::get<Maths::Transform>);

                for(auto& lightEntity : group)
                {
                    const auto& [light, trans] = group.get<Graphics::Light, Maths::Transform>(lightEntity);
                    light.Position = trans.GetWorldPosition();
                    Maths::Vector3 forward = Maths::Vector3::FORWARD;
                    forward = trans.GetWorldOrientation() * forward;
                    light.Direction = forward.Normalised();

                    if(light.Type == (float)Graphics::LightType::DirectionalLight)
                        directionaLight = &light;

                    if(light.Type != float(LightType::DirectionalLight))
                    {
                        auto inside = m_ForwardData.m_Frustum.IsInsideFast(Maths::Sphere(light.Position.ToVector3(), light.Radius));

                        if(inside == Maths::Intersection::OUTSIDE)
                            continue;
                    }

                    lights[numLights] = light;
                    numLights++;
                }
            }

            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformBufferLight", "lights", lights, sizeof(Graphics::Light) * numLights);

            Maths::Vector4 cameraPos = Maths::Vector4(m_CameraTransform->GetWorldPosition());
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformBufferLight", "cameraPosition", &cameraPos);
        }

        if(renderSettings.ShadowsEnabled)
        {
            for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; i++)
            {
                m_ShadowData.m_CascadeCommandQueue[i].clear();
            }

            if(directionaLight)
            {
                UpdateCascades(scene, directionaLight);

                for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; i++)
                {
                    m_ShadowData.m_CascadeFrustums[i].Define(m_ShadowData.m_ShadowProjView[i]);
                }
            }
        }

        auto& shadowData = Application::Get().GetRenderGraph()->GetShadowData();

        Maths::Matrix4* shadowTransforms = shadowData.m_ShadowProjView;
        Lumos::Maths::Vector4* uSplitDepth = shadowData.m_SplitDepth;
        Maths::Matrix4 lightView = shadowData.m_LightMatrix;
        float bias = shadowData.m_InitialBias;

        float maxShadowDistance = shadowData.m_MaxShadowDistance;
        float LightSize = shadowData.m_ShadowMapSize;
        float transitionFade = shadowData.m_CascadeTransitionFade;
        float shadowFade = shadowData.m_ShadowFade;

        if(renderSettings.Renderer3DEnabled)
        {
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformBufferLight", "viewMatrix", &view);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformBufferLight", "lightView", &lightView);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformBufferLight", "uShadowTransform", shadowTransforms);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformBufferLight", "uSplitDepths", uSplitDepth);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformBufferLight", "biasMat", &m_ForwardData.m_BiasMatrix);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformBufferLight", "lightSize", &LightSize);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformBufferLight", "shadowFade", &shadowFade);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformBufferLight", "cascadeTransitionFade", &transitionFade);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformBufferLight", "maxShadowDistance", &maxShadowDistance);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformBufferLight", "initialBias", &bias);
            m_ForwardData.m_DescriptorSet[2]->SetTexture("uShadowMap", reinterpret_cast<Texture*>(shadowData.m_ShadowTex), TextureType::DEPTHARRAY);

            int numShadows = shadowData.m_ShadowMapNum;
            auto cubemapMipLevels = m_ForwardData.m_EnvironmentMap ? m_ForwardData.m_EnvironmentMap->GetMipMapLevels() : 0;
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformBufferLight", "lightCount", &numLights);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformBufferLight", "shadowCount", &numShadows);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformBufferLight", "mode", &m_ForwardData.m_RenderMode);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformBufferLight", "cubemapMipLevels", &cubemapMipLevels);
            m_ForwardData.m_DescriptorSet[2]->SetTexture("uPreintegratedFG", m_ForwardData.m_PreintegratedFG.get());
            m_ForwardData.m_DescriptorSet[2]->SetTexture("uEnvironmentMap", m_ForwardData.m_EnvironmentMap, TextureType::CUBE);
            m_ForwardData.m_DescriptorSet[2]->SetTexture("uIrradianceMap", m_ForwardData.m_IrradianceMap, TextureType::CUBE);
        }

        m_ForwardData.m_CommandQueue.clear();

        auto group = registry.group<ModelComponent>(entt::get<Maths::Transform>);

        for(auto entity : group)
        {
            const auto& [model, trans] = group.get<ModelComponent, Maths::Transform>(entity);
            const auto& meshes = model.ModelRef->GetMeshes();

            for(auto mesh : meshes)
            {
                if(mesh->GetActive())
                {
                    auto& worldTransform = trans.GetWorldMatrix();

                    auto bb = mesh->GetBoundingBox();
                    auto bbCopy = bb->Transformed(worldTransform);

                    if(directionaLight)
                    {
                        for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; i++)
                        {
                            auto inside = m_ShadowData.m_CascadeFrustums[i].IsInsideFast(bbCopy);

                            if(inside != Maths::Intersection::OUTSIDE)
                            {
                                RenderCommand command;
                                command.mesh = mesh.get();
                                command.transform = worldTransform;
                                m_ShadowData.m_CascadeCommandQueue[i].push_back(command);
                            }
                        }
                    }

                    {

                        auto inside = m_ForwardData.m_Frustum.IsInsideFast(bbCopy);

                        if(inside != Maths::Intersection::OUTSIDE)
                        {
                            RenderCommand command;
                            command.mesh = mesh;
                            command.transform = worldTransform;
                            command.material = mesh->GetMaterial() ? mesh->GetMaterial().get() : m_ForwardData.m_DefaultMaterial;

                            //Update material buffers
                            command.material->Bind();
                            m_ForwardData.m_CommandQueue.push_back(command);
                        }
                    }
                }
            }
        }

        m_Renderer2DData.m_DescriptorSet[0]->SetUniform("UniformBufferObject", "projView", &projView);
        m_Renderer2DData.m_DescriptorSet[0]->Update();
        m_Renderer2DData.m_CommandQueue2D.clear();

        auto spriteGroup = registry.group<Graphics::Sprite>(entt::get<Maths::Transform>);
        for(auto entity : spriteGroup)
        {
            const auto& [sprite, trans] = spriteGroup.get<Graphics::Sprite, Maths::Transform>(entity);

            auto bb = Maths::BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetPosition() + sprite.GetScale()));
            bb.Transform(trans.GetWorldMatrix());
            auto inside = m_ForwardData.m_Frustum.IsInside(bb);

            if(inside == Maths::Intersection::OUTSIDE)
                continue;

            RenderCommand2D command;
            command.renderable = &sprite;
            command.transform = trans.GetWorldMatrix();
            m_Renderer2DData.m_CommandQueue2D.push_back(command);
        };

        auto animSpriteGroup = registry.group<Graphics::AnimatedSprite>(entt::get<Maths::Transform>);
        for(auto entity : animSpriteGroup)
        {
            const auto& [sprite, trans] = animSpriteGroup.get<Graphics::AnimatedSprite, Maths::Transform>(entity);

            auto bb = Maths::BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetPosition() + sprite.GetScale()));
            bb.Transform(trans.GetWorldMatrix());
            auto inside = m_ForwardData.m_Frustum.IsInside(bb);

            if(inside == Maths::Intersection::OUTSIDE)
                continue;

            RenderCommand2D command;
            command.renderable = &sprite;
            command.transform = trans.GetWorldMatrix();
            m_Renderer2DData.m_CommandQueue2D.push_back(command);
        };

        {
            LUMOS_PROFILE_SCOPE("Sort Meshes by distance from camera");
            auto camTransform = m_CameraTransform;
            std::sort(m_ForwardData.m_CommandQueue.begin(), m_ForwardData.m_CommandQueue.end(),
                [camTransform](RenderCommand& a, RenderCommand& b)
                {
                    return (camTransform->GetWorldPosition() - a.transform.Translation()).Length() < (camTransform->GetWorldPosition() - b.transform.Translation()).Length();
                });
        }

        for(auto renderer : m_Renderers)
        {
            renderer->BeginScene(scene, m_OverrideCamera, m_OverrideCameraTransform);
        }
    }

    void RenderGraph::SetRenderTarget(Graphics::Texture* texture, bool onlyIfTargetsScreen, bool rebuildFramebuffer)
    {
        LUMOS_PROFILE_FUNCTION();
        for(auto renderer : m_Renderers)
        {
            if(!onlyIfTargetsScreen || renderer->GetScreenRenderer())
                renderer->SetRenderTarget(texture, rebuildFramebuffer);
        }

        m_ForwardData.m_RenderTexture = texture;
    }

    void RenderGraph::OnRender()
    {
        LUMOS_PROFILE_FUNCTION();
        GPUProfile("Render Passes");

        auto& sceneRenderSettings = Application::Get().GetCurrentScene()->GetSettings().RenderSettings;

        if(m_ForwardData.m_RenderTexture)
        {
            Renderer::GetRenderer()->ClearRenderTarget(m_ForwardData.m_RenderTexture, Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());
        }
        else
        {
            Renderer::GetRenderer()->ClearRenderTarget(Renderer::GetMainSwapChain()->GetCurrentImage(), Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());
        }

        if(sceneRenderSettings.ShadowsEnabled)
            ShadowPass();
        if(sceneRenderSettings.Renderer3DEnabled)
            ForwardPass();
        if(sceneRenderSettings.SkyboxRenderEnabled)
            SkyboxPass();
        if(sceneRenderSettings.Renderer2DEnabled)
            Render2DPass();
        if(sceneRenderSettings.DebugRenderEnabled)
            DebugPass();

        for(auto renderer : m_Renderers)
        {
            renderer->RenderScene();
        }

        DebugRenderer::Reset();
    }

    void RenderGraph::OnUpdate(const TimeStep& timeStep, Scene* scene)
    {
    }

    bool RenderGraph::OnwindowResizeEvent(WindowResizeEvent& e)
    {
        LUMOS_PROFILE_FUNCTION();
        for(auto renderer : m_Renderers)
        {
            renderer->OnResize(e.GetWidth(), e.GetHeight());
        }

        return false;
    }

    void RenderGraph::OnEvent(Event& e)
    {
        LUMOS_PROFILE_FUNCTION();
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(RenderGraph::OnwindowResizeEvent));
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

    void RenderGraph::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();
        for(auto renderer : m_Renderers)
        {
            renderer->OnImGui();
        }

        ImGui::TextUnformatted("Shadow Renderer");
#if FIX_IMGUI_TEXTURE_ARRAY
        // Now pass texture pointer to imgui for vulkan so this handle array causes a crash
        if(ImGui::TreeNode("Texture"))
        {
            static int index = 0;

            ImGui::InputInt("Texture Array Index", &index);

            index = Maths::Max(0, index);
            index = Maths::Min(index, 3);
            bool flipImage = Renderer::GetGraphicsContext()->FlipImGUITexture();

            ImGui::Image(m_ShadowData.m_ShadowTex->GetHandleArray(uint32_t(index)), ImVec2(128, 128), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));

            if(ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Image(m_ShadowData.m_ShadowTex->GetHandleArray(uint32_t(index)), ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
                ImGui::EndTooltip();
            }

            ImGui::TreePop();
        }
#endif

        ImGui::DragFloat("Initial Bias", &m_ShadowData.m_InitialBias, 0.00005f, 0.0f, 1.0f, "%.6f");
        ImGui::DragFloat("Light Size", &m_ShadowData.m_LightSize, 0.00005f, 0.0f, 10.0f);
        ImGui::DragFloat("Max Shadow Distance", &m_ShadowData.m_MaxShadowDistance, 0.05f, 0.0f, 10000.0f);
        ImGui::DragFloat("Shadow Fade", &m_ShadowData.m_ShadowFade, 0.0005f, 0.0f, 500.0f);
        ImGui::DragFloat("Cascade Transition Fade", &m_ShadowData.m_CascadeTransitionFade, 0.0005f, 0.0f, 5.0f);

        ImGui::DragFloat("Cascade Split Lambda", &m_ShadowData.m_CascadeSplitLambda, 0.005f, 0.0f, 3.0f);
        ImGui::DragFloat("Scene Radius Multiplier", &m_ShadowData.m_SceneRadiusMultiplier, 0.005f, 0.0f, 5.0f);

        ImGui::TextUnformatted("Forward Renderer");

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Number Of Renderables");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::Text("%5.2lu", m_ForwardData.m_CommandQueue.size());
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Render Mode");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::BeginMenu(RenderModeToString(m_ForwardData.m_RenderMode).c_str()))
        {
            const int numRenderModes = 8;

            for(int i = 0; i < numRenderModes; i++)
            {
                if(ImGui::MenuItem(RenderModeToString(i).c_str(), "", m_ForwardData.m_RenderMode == i, true))
                {
                    m_ForwardData.m_RenderMode = i;
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

    void RenderGraph::OnNewScene(Scene* scene)
    {
    }

    void RenderGraph::Reset()
    {
        m_ReflectSkyBox = false;
        m_UseShadowMap = false;
    }

    void RenderGraph::AddRenderer(Graphics::IRenderer* renderer)
    {
        m_Renderers.push_back(renderer);
        //SortRenderers();
    }

    void RenderGraph::AddRenderer(Graphics::IRenderer* renderer, int renderPriority)
    {
        renderer->SetRenderPriority(renderPriority);
        m_Renderers.push_back(renderer);
        //SortRenderers();
    }

    void RenderGraph::SortRenderers()
    {
        LUMOS_PROFILE_FUNCTION();
        std::sort(m_Renderers.begin(), m_Renderers.end(), [](Graphics::IRenderer* a, Graphics::IRenderer* b)
            { return a->GetRenderPriority() > b->GetRenderPriority(); });
    }

    void RenderGraph::UpdateCascades(Scene* scene, Light* light)
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
        for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; i++)
        {
            float p = static_cast<float>(i + 1) / static_cast<float>(m_ShadowData.m_ShadowMapNum);
            float log = minZ * std::pow(ratio, p);
            float uniform = minZ + range * p;
            float d = m_ShadowData.m_CascadeSplitLambda * (log - uniform) + uniform;
            cascadeSplits[i] = (d - nearClip) / clipRange;
        }

        cascadeSplits[3] = 0.35f;

#ifdef THREAD_CASCADE_GEN
        System::JobSystem::Context ctx;
        System::JobSystem::Dispatch(ctx, static_cast<uint32_t>(m_ShadowMapNum), 1, [&](JobDispatchArgs args)
#else
        for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; i++)
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
                float sceneBoundingRadius = m_Camera->GetShadowBoundingRadius() * m_ShadowData.m_SceneRadiusMultiplier;
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
                    shadowOrigin *= (m_ShadowData.m_ShadowMapSize / 2.0f);

                    Maths::Vector3 roundedOrigin = Maths::VectorRound(shadowOrigin);
                    Maths::Vector3 roundOffset = roundedOrigin - shadowOrigin;
                    roundOffset = roundOffset * (2.0f / m_ShadowData.m_ShadowMapSize);
                    roundOffset.z = 0.0f;

                    shadowProj.ElementRef(0, 3) += roundOffset.x;
                    shadowProj.ElementRef(1, 3) += roundOffset.y;
                }
                // Store split distance and matrix in cascade
                m_ShadowData.m_SplitDepth[i] = Maths::Vector4((m_Camera->GetNear() + splitDist * clipRange) * -1.0f);
                m_ShadowData.m_ShadowProjView[i] = shadowProj;

                if(i == 0)
                    m_ShadowData.m_LightMatrix = lightViewMatrix.Inverse();
            }
#ifdef THREAD_CASCADE_GEN
        );
        System::JobSystem::Wait(ctx);
#endif
    }

    void RenderGraph::ShadowPass()
    {
        LUMOS_PROFILE_FUNCTION();
        GPUProfile("Shadow Pass");

        m_ShadowData.m_DescriptorSet[0]->SetUniform("UniformBufferObject", "projView", m_ShadowData.m_ShadowProjView);
        m_ShadowData.m_DescriptorSet[0]->Update();

        Graphics::PipelineDesc pipelineDesc;
        pipelineDesc.shader = m_ShadowData.m_Shader;

        pipelineDesc.cullMode = Graphics::CullMode::NONE;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.depthBiasEnabled = false;
        pipelineDesc.depthArrayTarget = reinterpret_cast<Texture*>(m_ShadowData.m_ShadowTex);
        pipelineDesc.clearTargets = true;

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);

        for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; ++i)
        {
            m_ShadowData.m_Layer = i;

            pipeline->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), m_ShadowData.m_Layer);

            for(auto& command : m_ShadowData.m_CascadeCommandQueue[m_ShadowData.m_Layer])
            {
                Engine::Get().Statistics().NumShadowObjects++;

                Mesh* mesh = command.mesh;

                m_ShadowData.m_CurrentDescriptorSets[0] = m_ShadowData.m_DescriptorSet[0].get();

                mesh->GetVertexBuffer()->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), pipeline.get());
                mesh->GetIndexBuffer()->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());

                uint32_t layer = static_cast<uint32_t>(m_ShadowData.m_Layer);
                auto trans = command.transform;
                auto& pushConstants = m_ShadowData.m_Shader->GetPushConstants();
                memcpy(pushConstants[0].data, &trans, sizeof(Maths::Matrix4));
                memcpy(pushConstants[0].data + sizeof(Maths::Matrix4), &layer, sizeof(uint32_t));

                m_ShadowData.m_Shader->BindPushConstants(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), pipeline.get());

                Renderer::BindDescriptorSets(pipeline.get(), Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), 0, m_ShadowData.m_CurrentDescriptorSets.data(), 1);
                Renderer::DrawIndexed(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), DrawType::TRIANGLE, mesh->GetIndexBuffer()->GetCount());

                mesh->GetVertexBuffer()->Unbind();
                mesh->GetIndexBuffer()->Unbind();
            }

            pipeline->End(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());
        }
    }
    void RenderGraph::ForwardPass()
    {
        LUMOS_PROFILE_FUNCTION();
        GPUProfile("Forward Pass");

        m_ForwardData.m_DescriptorSet[2]->Update();

        Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader = m_ForwardData.m_Shader;
        pipelineDesc.polygonMode = Graphics::PolygonMode::FILL;
        pipelineDesc.blendMode = BlendMode::SrcAlphaOneMinusSrcAlpha;
        pipelineDesc.clearTargets = false;
        pipelineDesc.swapchainTarget = false;

        if(m_ForwardData.m_DepthTest)
        {
            pipelineDesc.depthTarget = reinterpret_cast<Texture*>(Application::Get().GetRenderGraph()->GetGBuffer()->GetDepthTexture());
            Renderer::GetRenderer()->ClearRenderTarget(pipelineDesc.depthTarget, commandBuffer);
        }

        if(m_ForwardData.m_RenderTexture)
            pipelineDesc.colourTargets[0] = m_ForwardData.m_RenderTexture;
        else
            pipelineDesc.swapchainTarget = true;

        for(auto& command : m_ForwardData.m_CommandQueue)
        {
            Mesh* mesh = command.mesh;
            auto& worldTransform = command.transform;

            Material* material = command.material ? command.material : m_ForwardData.m_DefaultMaterial;
            pipelineDesc.cullMode = material->GetFlag(Material::RenderFlags::TWOSIDED) ? Graphics::CullMode::NONE : Graphics::CullMode::BACK;
            pipelineDesc.transparencyEnabled = material->GetFlag(Material::RenderFlags::ALPHABLEND);

            auto pipeline = Graphics::Pipeline::Get(pipelineDesc);

            //material->Bind();

            commandBuffer->BindPipeline(pipeline);

            m_ForwardData.m_CurrentDescriptorSets[0] = m_ForwardData.m_DescriptorSet[0].get();
            m_ForwardData.m_CurrentDescriptorSets[1] = material->GetDescriptorSet();
            m_ForwardData.m_CurrentDescriptorSets[2] = m_ForwardData.m_DescriptorSet[2].get();

            mesh->GetVertexBuffer()->Bind(commandBuffer, pipeline.get());
            mesh->GetIndexBuffer()->Bind(commandBuffer);

            auto& pushConstants = m_ForwardData.m_Shader->GetPushConstants()[0];
            pushConstants.SetValue("transform", (void*)&worldTransform);

            m_ForwardData.m_Shader->BindPushConstants(commandBuffer, pipeline.get());
            Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, m_ForwardData.m_CurrentDescriptorSets.data(), 3);
            Renderer::DrawIndexed(commandBuffer, DrawType::TRIANGLE, mesh->GetIndexBuffer()->GetCount());

            mesh->GetVertexBuffer()->Unbind();
            mesh->GetIndexBuffer()->Unbind();
        }

        commandBuffer->UnBindPipeline();
    }

    void RenderGraph::SkyboxPass()
    {
        LUMOS_PROFILE_FUNCTION();

        if(!m_CubeMap)
            return;

        m_SkyboxDescriptorSet->SetTexture("u_CubeMap", m_CubeMap, TextureType::CUBE);
        m_SkyboxDescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader = m_SkyboxShader;

        pipelineDesc.polygonMode = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = false;

        {
            pipelineDesc.depthTarget = reinterpret_cast<Texture*>(Application::Get().GetRenderGraph()->GetGBuffer()->GetDepthTexture());
        }

        if(m_ForwardData.m_RenderTexture)
            pipelineDesc.colourTargets[0] = m_ForwardData.m_RenderTexture;
        else
            pipelineDesc.swapchainTarget = true;

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        pipeline->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());

        m_SkyboxMesh->GetVertexBuffer()->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), pipeline.get());
        m_SkyboxMesh->GetIndexBuffer()->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());

        auto set = m_SkyboxDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), 0, &set, 1);
        Renderer::DrawIndexed(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), DrawType::TRIANGLE, m_SkyboxMesh->GetIndexBuffer()->GetCount());

        m_SkyboxMesh->GetVertexBuffer()->Unbind();
        m_SkyboxMesh->GetIndexBuffer()->Unbind();

        pipeline->End(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());
    }

    float RenderGraph::SubmitTexture(Texture* texture)
    {
        LUMOS_PROFILE_FUNCTION();
        float result = 0.0f;
        bool found = false;
        for(uint32_t i = 0; i < m_Renderer2DData.m_TextureCount; i++)
        {
            if(m_Renderer2DData.m_Textures[i] == texture)
            {
                result = static_cast<float>(i + 1);
                found = true;
                break;
            }
        }

        if(!found)
        {
            if(m_Renderer2DData.m_TextureCount >= m_Renderer2DData.m_Limits.MaxTextures)
            {
                LUMOS_LOG_ERROR("Unimplemented");
                //FlushAndReset();
            }
            m_Renderer2DData.m_Textures[m_Renderer2DData.m_TextureCount] = texture;
            m_Renderer2DData.m_TextureCount++;
            result = static_cast<float>(m_Renderer2DData.m_TextureCount);
        }
        return result;
    }

    void RenderGraph::Render2DPass()
    {
        LUMOS_PROFILE_FUNCTION();
        GPUProfile("Render2D Pass");

        Graphics::PipelineDesc pipelineDesc;
        pipelineDesc.shader = m_Renderer2DData.m_Shader;

        pipelineDesc.polygonMode = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = true;
        pipelineDesc.blendMode = BlendMode::SrcAlphaOneMinusSrcAlpha;
        pipelineDesc.clearTargets = false;

        if(true)
        {
            pipelineDesc.depthTarget = reinterpret_cast<Texture*>(Application::Get().GetRenderGraph()->GetGBuffer()->GetDepthTexture());
        }

        if(m_ForwardData.m_RenderTexture)
            pipelineDesc.colourTargets[0] = m_ForwardData.m_RenderTexture;
        else
            pipelineDesc.swapchainTarget = true;

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);

        m_Renderer2DData.m_TextureCount = 0;
        //m_Triangles.clear();
        uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

        m_Renderer2DData.m_VertexBuffers[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), pipeline.get());
        m_Renderer2DData.m_Buffer = m_Renderer2DData.m_VertexBuffers[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex]->GetPointer<VertexData>();

        if(m_Renderer2DData.m_CommandQueue2D.empty())
            return;

        std::sort(m_Renderer2DData.m_CommandQueue2D.begin(), m_Renderer2DData.m_CommandQueue2D.end(),
            [](RenderCommand2D& a, RenderCommand2D& b)
            {
                return a.transform.Translation().z < b.transform.Translation().z;
            });

        for(auto& command : m_Renderer2DData.m_CommandQueue2D)
        {
            Engine::Get().Statistics().NumRenderedObjects++;

            //if(m_Renderer2DData.m_IndexCount >= m_Renderer2DData.m_Limits.IndiciesSize)
            //  FlushAndReset();

            auto& renderable = command.renderable;
            auto& transform = command.transform;

            const Maths::Vector2 min = renderable->GetPosition();
            const Maths::Vector2 max = renderable->GetPosition() + renderable->GetScale();

            const Maths::Vector4 colour = renderable->GetColour();
            const auto& uv = renderable->GetUVs();
            const Texture* texture = renderable->GetTexture();

            float textureSlot = 0.0f;
            if(texture)
                textureSlot = SubmitTexture(renderable->GetTexture());

            Maths::Vector3 vertex = transform * Maths::Vector3(min.x, min.y, 0.0f);
            m_Renderer2DData.m_Buffer->vertex = vertex;
            m_Renderer2DData.m_Buffer->uv = uv[0];
            m_Renderer2DData.m_Buffer->tid = Maths::Vector2(textureSlot, 0.0f);
            m_Renderer2DData.m_Buffer->colour = colour;
            m_Renderer2DData.m_Buffer++;

            vertex = transform * Maths::Vector3(max.x, min.y, 0.0f);
            m_Renderer2DData.m_Buffer->vertex = vertex;
            m_Renderer2DData.m_Buffer->uv = uv[1];
            m_Renderer2DData.m_Buffer->tid = Maths::Vector2(textureSlot, 0.0f);
            m_Renderer2DData.m_Buffer->colour = colour;
            m_Renderer2DData.m_Buffer++;

            vertex = transform * Maths::Vector3(max.x, max.y, 0.0f);
            m_Renderer2DData.m_Buffer->vertex = vertex;
            m_Renderer2DData.m_Buffer->uv = uv[2];
            m_Renderer2DData.m_Buffer->tid = Maths::Vector2(textureSlot, 0.0f);
            m_Renderer2DData.m_Buffer->colour = colour;
            m_Renderer2DData.m_Buffer++;

            vertex = transform * Maths::Vector3(min.x, max.y, 0.0f);
            m_Renderer2DData.m_Buffer->vertex = vertex;
            m_Renderer2DData.m_Buffer->uv = uv[3];
            m_Renderer2DData.m_Buffer->tid = Maths::Vector2(textureSlot, 0.0f);
            m_Renderer2DData.m_Buffer->colour = colour;
            m_Renderer2DData.m_Buffer++;

            m_Renderer2DData.m_IndexCount += 6;
        }

        if(m_Renderer2DData.m_IndexCount == 0)
        {
            m_Renderer2DData.m_VertexBuffers[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex]->ReleasePointer();
            m_Renderer2DData.m_Empty = true;
            return;
        }

        m_Renderer2DData.m_Empty = false;

        if(m_Renderer2DData.m_TextureCount != m_Renderer2DData.m_PreviousFrameTextureCount)
        {
            // When previous frame texture count was less then than the previous frame
            // and the texture previously used was deleted, there was a crash - maybe moltenvk only
            Graphics::DescriptorDesc descriptorDesc {};
            descriptorDesc.layoutIndex = 1;
            descriptorDesc.shader = m_Renderer2DData.m_Shader.get();
            m_Renderer2DData.m_DescriptorSet[1] = Graphics::DescriptorSet::Create(descriptorDesc);
        }

        if(m_Renderer2DData.m_TextureCount > 1)
            m_Renderer2DData.m_DescriptorSet[1]->SetTexture("textures", m_Renderer2DData.m_Textures, m_Renderer2DData.m_TextureCount);
        else
            m_Renderer2DData.m_DescriptorSet[1]->SetTexture("textures", m_Renderer2DData.m_Textures[0]);

        m_Renderer2DData.m_DescriptorSet[1]->Update();
        m_Renderer2DData.m_PreviousFrameTextureCount = m_Renderer2DData.m_TextureCount;

        // m_DescriptorSet[0]->Update();
        // m_DescriptorSet[1]->Update();

        Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();

        pipeline->Bind(commandBuffer);

        m_Renderer2DData.m_CurrentDescriptorSets[0] = m_Renderer2DData.m_DescriptorSet[0].get();
        m_Renderer2DData.m_CurrentDescriptorSets[1] = m_Renderer2DData.m_DescriptorSet[1].get();

        m_Renderer2DData.m_IndexBuffer->SetCount(m_Renderer2DData.m_IndexCount);
        m_Renderer2DData.m_IndexBuffer->Bind(commandBuffer);

        m_Renderer2DData.m_VertexBuffers[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex]->ReleasePointer();

        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, m_Renderer2DData.m_CurrentDescriptorSets.data(), 2);
        Renderer::DrawIndexed(commandBuffer, DrawType::TRIANGLE, m_Renderer2DData.m_IndexCount);

        m_Renderer2DData.m_VertexBuffers[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex]->Unbind();
        m_Renderer2DData.m_IndexBuffer->Unbind();

        pipeline->End(commandBuffer);

        m_Renderer2DData.m_IndexCount = 0;

        m_Renderer2DData.m_BatchDrawCallIndex++;
        //TODO: loop if many sprites

        //End
        m_Renderer2DData.m_BatchDrawCallIndex = 0;
    }

    void RenderGraph::DebugPass()
    {
        LUMOS_PROFILE_FUNCTION();
        GPUProfile("Debug Pass");

        const auto& lines = DebugRenderer::GetInstance()->GetLines();
        const auto& triangles = DebugRenderer::GetInstance()->GetTriangles();
        const auto& points = DebugRenderer::GetInstance()->GetPoints();

        auto projView = m_Camera->GetProjectionMatrix() * m_CameraTransform->GetWorldMatrix().Inverse();

        if(!lines.empty())
        {
            m_DebugDrawData.m_LineDescriptorSet[0]->SetUniform("UniformBufferObject", "projView", &projView);
            m_DebugDrawData.m_LineDescriptorSet[0]->Update();

            m_DebugDrawData.m_LineBuffer = m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->GetPointer<LineVertexData>();

            Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();

            Graphics::PipelineDesc pipelineDesc;
            pipelineDesc.shader = m_DebugDrawData.m_LineShader;

            pipelineDesc.polygonMode = Graphics::PolygonMode::FILL;
            pipelineDesc.cullMode = Graphics::CullMode::BACK;
            pipelineDesc.transparencyEnabled = false;
            pipelineDesc.clearTargets = false;
            pipelineDesc.drawType = DrawType::LINES;
            if(m_ForwardData.m_RenderTexture)
                pipelineDesc.colourTargets[0] = m_ForwardData.m_RenderTexture;
            else
                pipelineDesc.swapchainTarget = true;

            auto pipeline = Graphics::Pipeline::Get(pipelineDesc);

            pipeline->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());
            m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), pipeline.get());

            for(auto& line : lines)
            {
                m_DebugDrawData.m_LineBuffer->vertex = line.p1;
                m_DebugDrawData.m_LineBuffer->colour = line.col;
                m_DebugDrawData.m_LineBuffer++;

                m_DebugDrawData.m_LineBuffer->vertex = line.p2;
                m_DebugDrawData.m_LineBuffer->colour = line.col;
                m_DebugDrawData.m_LineBuffer++;

                m_DebugDrawData.LineIndexCount += 2;
            }

            m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->ReleasePointer();
            m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->Unbind();

            m_DebugDrawData.m_LineIndexBuffer->SetCount(m_DebugDrawData.LineIndexCount);

            m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->Bind(commandBuffer, pipeline.get());
            m_DebugDrawData.m_LineIndexBuffer->Bind(commandBuffer);
            auto* desc = m_DebugDrawData.m_LineDescriptorSet[0].get();
            Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &desc, 1);
            Renderer::DrawIndexed(commandBuffer, DrawType::LINES, m_DebugDrawData.LineIndexCount);

            m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->Unbind();
            m_DebugDrawData.m_LineIndexBuffer->Unbind();

            m_DebugDrawData.LineIndexCount = 0;

            pipeline->End(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());

            m_DebugDrawData.m_LineBatchDrawCallIndex++;
        }

        if(!points.empty())
        {
            m_DebugDrawData.m_PointDescriptorSet[0]->SetUniform("UniformBufferObject", "projView", &projView);
            m_DebugDrawData.m_PointDescriptorSet[0]->Update();
            m_DebugDrawData.m_PointBuffer = m_DebugDrawData.m_PointVertexBuffers[m_DebugDrawData.m_PointBatchDrawCallIndex]->GetPointer<PointVertexData>();

            Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();

            Graphics::PipelineDesc pipelineDesc;
            pipelineDesc.shader = m_DebugDrawData.m_PointShader;

            pipelineDesc.polygonMode = Graphics::PolygonMode::FILL;
            pipelineDesc.cullMode = Graphics::CullMode::BACK;
            pipelineDesc.transparencyEnabled = true;
            pipelineDesc.drawType = DrawType::TRIANGLE;
            pipelineDesc.blendMode = BlendMode::SrcAlphaOneMinusSrcAlpha;

            if(m_ForwardData.m_RenderTexture)
                pipelineDesc.colourTargets[0] = m_ForwardData.m_RenderTexture;
            else
                pipelineDesc.swapchainTarget = true;

            auto pipeline = Graphics::Pipeline::Get(pipelineDesc);

            pipeline->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());
            m_DebugDrawData.m_PointVertexBuffers[m_DebugDrawData.m_PointBatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), pipeline.get());

            for(auto& pointInfo : points)
            {
                Maths::Vector3 right = pointInfo.size * m_CameraTransform->GetRightDirection();
                Maths::Vector3 up = pointInfo.size * m_CameraTransform->GetUpDirection();

                m_DebugDrawData.m_PointBuffer->vertex = pointInfo.p1 - right - up; // + Maths::Vector3(-pointInfo.size, -pointInfo.size, 0.0f));
                m_DebugDrawData.m_PointBuffer->colour = pointInfo.col;
                m_DebugDrawData.m_PointBuffer->size = { pointInfo.size, 0.0f };
                m_DebugDrawData.m_PointBuffer->uv = { -1.0f, -1.0f };
                m_DebugDrawData.m_PointBuffer++;

                m_DebugDrawData.m_PointBuffer->vertex = pointInfo.p1 + right - up; //(pointInfo.p1 + Maths::Vector3(pointInfo.size, -pointInfo.size, 0.0f));
                m_DebugDrawData.m_PointBuffer->colour = pointInfo.col;
                m_DebugDrawData.m_PointBuffer->size = { pointInfo.size, 0.0f };
                m_DebugDrawData.m_PointBuffer->uv = { 1.0f, -1.0f };
                m_DebugDrawData.m_PointBuffer++;

                m_DebugDrawData.m_PointBuffer->vertex = pointInfo.p1 + right + up; //(pointInfo.p1 + Maths::Vector3(pointInfo.size, pointInfo.size, 0.0f));
                m_DebugDrawData.m_PointBuffer->colour = pointInfo.col;
                m_DebugDrawData.m_PointBuffer->size = { pointInfo.size, 0.0f };
                m_DebugDrawData.m_PointBuffer->uv = { 1.0f, 1.0f };
                m_DebugDrawData.m_PointBuffer++;

                m_DebugDrawData.m_PointBuffer->vertex = pointInfo.p1 - right + up; // (pointInfo.p1 + Maths::Vector3(-pointInfo.size, pointInfo.size, 0.0f));
                m_DebugDrawData.m_PointBuffer->colour = pointInfo.col;
                m_DebugDrawData.m_PointBuffer->size = { pointInfo.size, 0.0f };
                m_DebugDrawData.m_PointBuffer->uv = { -1.0f, 1.0f };
                m_DebugDrawData.m_PointBuffer++;

                m_DebugDrawData.PointIndexCount += 6;
            }

            m_DebugDrawData.m_PointVertexBuffers[m_DebugDrawData.m_PointBatchDrawCallIndex]->ReleasePointer();
            m_DebugDrawData.m_PointVertexBuffers[m_DebugDrawData.m_PointBatchDrawCallIndex]->Unbind();
            m_DebugDrawData.m_PointIndexBuffer->SetCount(m_DebugDrawData.PointIndexCount);

            m_DebugDrawData.m_PointVertexBuffers[m_DebugDrawData.m_PointBatchDrawCallIndex]->Bind(commandBuffer, pipeline.get());
            m_DebugDrawData.m_PointIndexBuffer->Bind(commandBuffer);
            auto* desc = m_DebugDrawData.m_PointDescriptorSet[0].get();
            Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &desc, 1);
            Renderer::DrawIndexed(commandBuffer, DrawType::TRIANGLE, m_DebugDrawData.PointIndexCount);

            m_DebugDrawData.m_PointVertexBuffers[m_DebugDrawData.m_PointBatchDrawCallIndex]->Unbind();
            m_DebugDrawData.m_PointIndexBuffer->Unbind();

            m_DebugDrawData.PointIndexCount = 0;

            pipeline->End(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());

            m_DebugDrawData.m_PointBatchDrawCallIndex++;
        }

        if(!triangles.empty())
        {
            m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0]->SetUniform("UniformBufferObject", "projView", &projView);
            m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0]->Update();
            m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[1]->Update();

            Graphics::PipelineDesc pipelineDesc;
            pipelineDesc.shader = m_DebugDrawData.m_Renderer2DData.m_Shader;

            pipelineDesc.polygonMode = Graphics::PolygonMode::FILL;
            pipelineDesc.cullMode = Graphics::CullMode::BACK;
            pipelineDesc.transparencyEnabled = true;
            pipelineDesc.blendMode = BlendMode::SrcAlphaOneMinusSrcAlpha;
            pipelineDesc.clearTargets = false;

            if(m_ForwardData.m_RenderTexture)
                pipelineDesc.colourTargets[0] = m_ForwardData.m_RenderTexture;
            else
                pipelineDesc.swapchainTarget = true;

            auto pipeline = Graphics::Pipeline::Get(pipelineDesc);

            m_DebugDrawData.m_Renderer2DData.m_TextureCount = 0;
            uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

            m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), pipeline.get());
            m_DebugDrawData.m_Renderer2DData.m_Buffer = m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex]->GetPointer<VertexData>();

            for(auto& triangleInfo : triangles)
            {
                Engine::Get().Statistics().NumRenderedObjects++;

                float textureSlot = 0.0f;

                m_DebugDrawData.m_Renderer2DData.m_Buffer->vertex = triangleInfo.p1;
                m_DebugDrawData.m_Renderer2DData.m_Buffer->uv = { 0.0f, 0.0f };
                m_DebugDrawData.m_Renderer2DData.m_Buffer->tid = Maths::Vector2(textureSlot, 0.0f);
                m_DebugDrawData.m_Renderer2DData.m_Buffer->colour = triangleInfo.col;
                m_DebugDrawData.m_Renderer2DData.m_Buffer++;
                m_DebugDrawData.m_Renderer2DData.m_Buffer->vertex = triangleInfo.p2;
                m_DebugDrawData.m_Renderer2DData.m_Buffer->uv = { 0.0f, 0.0f };
                m_DebugDrawData.m_Renderer2DData.m_Buffer->tid = Maths::Vector2(textureSlot, 0.0f);
                m_DebugDrawData.m_Renderer2DData.m_Buffer->colour = triangleInfo.col;
                m_DebugDrawData.m_Renderer2DData.m_Buffer++;
                m_DebugDrawData.m_Renderer2DData.m_Buffer->vertex = triangleInfo.p3;
                m_DebugDrawData.m_Renderer2DData.m_Buffer->uv = { 0.0f, 0.0f };
                m_DebugDrawData.m_Renderer2DData.m_Buffer->tid = Maths::Vector2(textureSlot, 0.0f);
                m_DebugDrawData.m_Renderer2DData.m_Buffer->colour = triangleInfo.col;
                m_DebugDrawData.m_Renderer2DData.m_Buffer++;
                m_DebugDrawData.m_Renderer2DData.m_IndexCount += 3;
            }

            m_DebugDrawData.m_Renderer2DData.m_Empty = false;
            Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();

            pipeline->Bind(commandBuffer);

            m_DebugDrawData.m_Renderer2DData.m_CurrentDescriptorSets[0] = m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0].get();
            m_DebugDrawData.m_Renderer2DData.m_CurrentDescriptorSets[1] = m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[1].get();

            m_DebugDrawData.m_Renderer2DData.m_IndexBuffer->SetCount(m_DebugDrawData.m_Renderer2DData.m_IndexCount);
            m_DebugDrawData.m_Renderer2DData.m_IndexBuffer->Bind(commandBuffer);

            m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex]->ReleasePointer();

            Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, m_DebugDrawData.m_Renderer2DData.m_CurrentDescriptorSets.data(), 2);
            Renderer::DrawIndexed(commandBuffer, DrawType::TRIANGLE, m_DebugDrawData.m_Renderer2DData.m_IndexCount);

            m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex]->Unbind();
            m_DebugDrawData.m_Renderer2DData.m_IndexBuffer->Unbind();

            pipeline->End(commandBuffer);

            m_DebugDrawData.m_Renderer2DData.m_IndexCount = 0;
        }

        m_DebugDrawData.m_PointBatchDrawCallIndex = 0;
        m_DebugDrawData.m_LineBatchDrawCallIndex = 0;
    }

}
