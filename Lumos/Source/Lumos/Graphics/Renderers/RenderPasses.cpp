#include "Precompiled.h"
#include "RenderPasses.h"
#include "Scene/Entity.h"
#include "Scene/Component/ModelComponent.h"
#include "Graphics/Model.h"
#include "Graphics/Animation/Skeleton.h"
#include "Graphics/Renderers/IRenderer.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Graphics/Light.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Environment.h"
#include "Graphics/Sprite.h"
#include "Graphics/AnimatedSprite.h"
#include "Graphics/RHI/GPUProfile.h"
#include "Graphics/Font.h"
#include "Graphics/MSDFData.h"
#include "Graphics/ParticleManager.h"
#include "Graphics/Animation/AnimationController.h"
#include "Core/JobSystem.h"
#include "Core/OS/Window.h"
#include "Maths/BoundingSphere.h"

#include "Events/ApplicationEvent.h"

#include "Embedded/BRDFTexture.inl"
#include "Embedded/CheckerBoardTextureArray.inl"
#include "Utilities/AssetManager.h"
#include "Core/Application.h"
#include "Scene/Component/Components.h"
#include "Maths/Random.h"
#include "ImGui/ImGuiUtilities.h"

#include <imgui/imgui.h>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/string_cast.hpp>

#include <cmath>

static const uint32_t MaxPoints                  = 1000;
static const uint32_t MaxPointVertices           = MaxPoints * 4;
static const uint32_t MaxPointIndices            = MaxPoints * 6;
static const uint32_t MAX_BATCH_DRAW_CALLS       = 100;
static const uint32_t RENDERER_POINT_SIZE        = sizeof(Lumos::Graphics::PointVertexData) * 4;
static const uint32_t RENDERER_POINT_BUFFER_SIZE = RENDERER_POINT_SIZE * MaxPointVertices;

static const uint32_t MaxLines                  = 1000;
static const uint32_t MaxLineVertices           = MaxLines * 2;
static const uint32_t MaxLineIndices            = MaxLines * 2;
static const uint32_t MAX_LINE_BATCH_DRAW_CALLS = 100;
static const uint32_t RENDERER_LINE_SIZE        = sizeof(Lumos::Graphics::LineVertexData) * 4;
static const uint32_t RENDERER_LINE_BUFFER_SIZE = RENDERER_LINE_SIZE * MaxLineVertices;

namespace Lumos::Graphics
{
    RenderPasses::RenderPasses(uint32_t width, uint32_t height)
    {
        LUMOS_PROFILE_FUNCTION();
        m_CubeMap        = nullptr;
        m_ClearColour    = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
        m_SupportCompute = Renderer::GetCapabilities().SupportCompute;

        Graphics::TextureDesc mainRenderTargetDesc;
        mainRenderTargetDesc.format          = Graphics::RHIFormat::R11G11B10_Float;
        mainRenderTargetDesc.flags           = TextureFlags::Texture_RenderTarget;
        mainRenderTargetDesc.wrap            = TextureWrap::CLAMP_TO_EDGE;
        mainRenderTargetDesc.minFilter       = TextureFilter::LINEAR;
        mainRenderTargetDesc.magFilter       = TextureFilter::LINEAR;
        mainRenderTargetDesc.generateMipMaps = false;
        mainRenderTargetDesc.samples         = m_MainTextureSamples;
        m_MainTexture                        = Graphics::Texture2D::Create(mainRenderTargetDesc, width, height);
        mainRenderTargetDesc.samples         = 1;

        m_PostProcessTexture1 = Graphics::Texture2D::Create(mainRenderTargetDesc, width, height);
        m_ResolveTexture      = Graphics::Texture2D::Create(mainRenderTargetDesc, width, height);

        // Setup shadow pass data
        m_ShadowData.m_ShadowTex    = nullptr;
        m_ShadowData.m_ShadowMapNum = 4;

        uint32_t shadowMapResolution = 4096;
        switch(Application::Get().GetQualitySettings().ShadowResolution)
        {
        case ShadowResolutionSetting::Low:
            shadowMapResolution = 1024;
            break;
        case ShadowResolutionSetting::Medium:
            shadowMapResolution = 2048;
            break;
        case ShadowResolutionSetting::High:
            shadowMapResolution = 4096;
            break;
        }
        m_ShadowData.m_ShadowMapSize = shadowMapResolution;

        m_ShadowData.m_ShadowMapsInvalidated = true;
        m_ShadowData.m_CascadeSplitLambda    = 0.92f;
        m_ShadowData.m_Shader                = Application::Get().GetAssetManager()->GetAssetData("Shadow").As<Graphics::Shader>();
        m_ShadowData.m_ShaderAlpha           = Application::Get().GetAssetManager()->GetAssetData("ShadowAlpha").As<Graphics::Shader>();
        m_ShadowData.m_ShaderAnim            = Application::Get().GetAssetManager()->GetAssetData("ShadowAnim").As<Graphics::Shader>();
        m_ShadowData.m_ShaderAnimAlpha       = Application::Get().GetAssetManager()->GetAssetData("ShadowAnimAlpha").As<Graphics::Shader>();
        m_ShadowData.m_ShadowTex             = TextureDepthArray::Create(m_ShadowData.m_ShadowMapSize, m_ShadowData.m_ShadowMapSize, m_ShadowData.m_ShadowMapNum, Renderer::GetRenderer()->GetDepthFormat());
        m_ShadowData.m_LightSize             = 1.5f;
        m_ShadowData.m_MaxShadowDistance     = 500.0f;
        m_ShadowData.m_ShadowFade            = 40.0f;
        m_ShadowData.m_CascadeFade           = 3.0f;
        m_ShadowData.m_InitialBias           = 0.00f;

        Graphics::DescriptorDesc descriptorDesc {};
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_ShadowData.m_Shader.get();
        m_ShadowData.m_DescriptorSet.resize(2);
        m_ShadowData.m_DescriptorSet[0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        descriptorDesc.shader           = m_ShadowData.m_ShaderAlpha.get();
        m_ShadowData.m_DescriptorSet[1] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_ShadowData.m_CascadeCommandQueue[0].reserve(1000);
        m_ShadowData.m_CascadeCommandQueue[1].reserve(1000);
        m_ShadowData.m_CascadeCommandQueue[2].reserve(1000);
        m_ShadowData.m_CascadeCommandQueue[3].reserve(1000);

        // Setup forward pass data
        m_ForwardData.m_DepthTest    = true;
        m_ForwardData.m_Shader       = Application::Get().GetAssetManager()->GetAssetData("ForwardPBR").As<Graphics::Shader>();
        m_ForwardData.m_AnimShader   = Application::Get().GetAssetManager()->GetAssetData("ForwardPBRAnim").As<Graphics::Shader>();
        m_ForwardData.m_DepthTexture = TextureDepth::Create(width, height, Renderer::GetRenderer()->GetDepthFormat(), m_MainTextureSamples);
        m_ForwardData.m_CommandQueue.reserve(1000);

        const size_t minUboAlignment = size_t(Graphics::Renderer::GetCapabilities().UniformBufferOffsetAlignment);

        m_ForwardData.m_DynamicAlignment = sizeof(glm::mat4);
        if(minUboAlignment > 0)
        {
            m_ForwardData.m_DynamicAlignment = (m_ForwardData.m_DynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
        }

        m_ForwardData.m_TransformData = static_cast<glm::mat4*>(Memory::AlignedAlloc(static_cast<uint32_t>(MAX_OBJECTS * m_ForwardData.m_DynamicAlignment), m_ForwardData.m_DynamicAlignment));

        const int SSAO_NOISE_DIM = 4;
        std::vector<glm::vec4> noiseData(SSAO_NOISE_DIM * SSAO_NOISE_DIM);

        for(glm::vec4& noiseSample : noiseData)
        {
            // Random rotations around z-axis
            noiseSample = glm::vec4(Random32::Rand(-1.0f, 1.0f), Random32::Rand(-1.0f, 1.0f), 0.0f, 0.0f);
        }

        Graphics::TextureDesc noiseTextureDesc = {};
        noiseTextureDesc.format                = Graphics::RHIFormat::R32G32B32A32_Float;
        noiseTextureDesc.wrap                  = TextureWrap::REPEAT;
        noiseTextureDesc.minFilter             = TextureFilter::NEAREST;
        noiseTextureDesc.magFilter             = TextureFilter::NEAREST;
        noiseTextureDesc.generateMipMaps       = false;
        noiseTextureDesc.anisotropicFiltering  = false;
        noiseTextureDesc.flags                 = 0;
        m_NoiseTexture                         = Graphics::Texture2D::CreateFromSource(SSAO_NOISE_DIM, SSAO_NOISE_DIM, (void*)noiseData.data(), noiseTextureDesc);

        noiseTextureDesc.flags     = TextureFlags::Texture_RenderTarget;
        noiseTextureDesc.wrap      = TextureWrap::CLAMP_TO_EDGE;
        noiseTextureDesc.minFilter = TextureFilter::LINEAR;
        noiseTextureDesc.magFilter = TextureFilter::LINEAR;
        noiseTextureDesc.samples   = m_MainTextureSamples;
        m_NormalTexture            = Graphics::Texture2D::Create(noiseTextureDesc, width, height);

        m_SSAOTexture  = Graphics::Texture2D::Create(noiseTextureDesc, width / 2, height / 2);
        m_SSAOTexture1 = Graphics::Texture2D::Create(noiseTextureDesc, width / 2, height / 2);

        switch(Graphics::GraphicsContext::GetRenderAPI())
        {
            // TODO: Check
#ifdef LUMOS_RENDER_API_OPENGL
        case Graphics::RenderAPI::OPENGL:
            m_ForwardData.m_BiasMatrix = glm::mat4(0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f);
            m_ForwardData.m_BiasMatrix = glm::mat4(
                0.5, 0.0, 0.0, 0.0,
                0.0, 0.5, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                0.5, 0.5, 0.0, 1.0);
            break;
#endif

#ifdef LUMOS_RENDER_API_VULKAN
        case Graphics::RenderAPI::VULKAN:
            m_ForwardData.m_BiasMatrix = glm::mat4(
                0.5, 0.0, 0.0, 0.0,
                0.0, 0.5, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                0.5, 0.5, 0.0, 1.0);
#endif

#ifdef LUMOS_RENDER_API_DIRECT3D
        case Graphics::RenderAPI::DIRECT3D:
            m_ForwardData.m_BiasMatrix = glm::mat4(0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            break;
#endif
        default:
            break;
        }

        TextureDesc param;
        param.minFilter         = TextureFilter::LINEAR;
        param.magFilter         = TextureFilter::LINEAR;
        param.format            = RHIFormat::R16G16_Float;
        param.srgb              = false;
        param.wrap              = TextureWrap::CLAMP_TO_EDGE;
        param.flags             = TextureFlags::Texture_RenderTarget;
        param.generateMipMaps   = false;
        m_ForwardData.m_BRDFLUT = UniquePtr<Texture2D>(Texture2D::Create(param, BRDFTextureWidth, BRDFTextureHeight));

        m_GenerateBRDFLUT = true;

        auto descriptorSetScene    = m_ForwardData.m_Shader->GetDescriptorInfo(2);
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_ForwardData.m_Shader.get();
        m_ForwardData.m_DescriptorSet.resize(4);
        m_ForwardData.m_DescriptorSet[0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        descriptorDesc.layoutIndex       = 2;
        m_ForwardData.m_DescriptorSet[2] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        descriptorDesc.layoutIndex       = 3;
        descriptorDesc.shader            = m_ForwardData.m_AnimShader;
        m_ForwardData.m_DescriptorSet[3] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        glm::mat4 boneTransforms[100];
        for(int i = 0; i < 100; i++)
        {
            boneTransforms[i] = glm::mat4(1.0f);
        }
        m_ForwardData.m_DescriptorSet[3]->SetUniform("BoneTransforms", "BoneTransforms", boneTransforms, sizeof(glm::mat4) * 100);
        m_ForwardData.m_DescriptorSet[3]->Update();

        m_ForwardData.m_DefaultMaterial  = new Material(m_ForwardData.m_Shader);
        uint32_t blackCubeTextureData[6] = { 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000 };
        m_DefaultTextureCube             = Graphics::TextureCube::Create(1, blackCubeTextureData);

        Graphics::MaterialProperties properties;
        properties.albedoColour       = glm::vec4(1.0f);
        properties.roughness          = 0.5f;
        properties.metallic           = 0.5f;
        properties.albedoMapFactor    = 0.0f;
        properties.roughnessMapFactor = 0.0f;
        properties.normalMapFactor    = 0.0f;
        properties.metallicMapFactor  = 0.0f;

        m_ForwardData.m_DefaultMaterial->SetMaterialProperites(properties);
        // m_ForwardData.m_DefaultMaterial->CreateDescriptorSet(1);

        // Set up skybox pass data
        m_ScreenQuad               = Graphics::CreateQuad();
        m_SkyboxShader             = Application::Get().GetAssetManager()->GetAssetData("Skybox").As<Graphics::Shader>();
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_SkyboxShader.get();
        m_SkyboxDescriptorSet      = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        // Set up final pass data
        m_FinalPassShader          = Application::Get().GetAssetManager()->GetAssetData("FinalPass").As<Graphics::Shader>();
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_FinalPassShader.get();
        m_FinalPassDescriptorSet   = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        // PostProcesses

        m_ToneMappingPassShader        = Application::Get().GetAssetManager()->GetAssetData("ToneMapping").As<Graphics::Shader>();
        descriptorDesc.layoutIndex     = 0;
        descriptorDesc.shader          = m_ToneMappingPassShader.get();
        m_ToneMappingPassDescriptorSet = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_DepthOfFieldShader            = Application::Get().GetAssetManager()->GetAssetData("DepthOfField").As<Graphics::Shader>();
        descriptorDesc.layoutIndex      = 0;
        descriptorDesc.shader           = m_DepthOfFieldShader.get();
        m_DepthOfFieldPassDescriptorSet = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_SharpenShader            = Application::Get().GetAssetManager()->GetAssetData("Sharpen").As<Graphics::Shader>();
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_SharpenShader.get();
        m_SharpenPassDescriptorSet = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_BloomPassShader          = Application::Get().GetAssetManager()->GetAssetData(m_SupportCompute ? "BloomComp" : "Bloom").As<Graphics::Shader>();
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_BloomPassShader.get();

        if(m_BloomPassShader->IsCompiled())
        {
            for(int i = 0; i < 6; i++)
                m_BloomDescriptorSets.push_back(SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc)));
        }

        mainRenderTargetDesc.flags = TextureFlags::Texture_RenderTarget | TextureFlags::Texture_CreateMips | TextureFlags::Texture_MipViews;
        m_BloomTexture             = Texture2D::Create(mainRenderTargetDesc, width, height);
        m_BloomTexture1            = Texture2D::Create(mainRenderTargetDesc, width, height);
        m_BloomTexture2            = Texture2D::Create(mainRenderTargetDesc, width, height);

        m_FXAAShader               = Application::Get().GetAssetManager()->GetAssetData(m_SupportCompute ? "FXAAComp" : "FXAA").As<Graphics::Shader>();
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_FXAAShader.get();
        m_FXAAPassDescriptorSet    = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_DebandingShader            = Application::Get().GetAssetManager()->GetAssetData("Debanding").As<Graphics::Shader>();
        descriptorDesc.layoutIndex   = 0;
        descriptorDesc.shader        = m_DebandingShader.get();
        m_DebandingPassDescriptorSet = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_ChromaticAberationShader            = Application::Get().GetAssetManager()->GetAssetData("ChromaticAberation").As<Graphics::Shader>();
        descriptorDesc.layoutIndex            = 0;
        descriptorDesc.shader                 = m_ChromaticAberationShader.get();
        m_ChromaticAberationPassDescriptorSet = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_DepthPrePassShader        = Application::Get().GetAssetManager()->GetAssetData("DepthPrePass").As<Graphics::Shader>();
        descriptorDesc.layoutIndex  = 0;
        descriptorDesc.shader       = m_DepthPrePassShader.get();
        m_DepthPrePassDescriptorSet = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_DepthPrePassAlphaShader        = Application::Get().GetAssetManager()->GetAssetData("DepthPrePassAlpha").As<Graphics::Shader>();
        descriptorDesc.shader            = m_DepthPrePassAlphaShader.get();
        m_DepthPrePassAlphaDescriptorSet = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_DepthPrePassAlphaAnimShader = Application::Get().GetAssetManager()->GetAssetData("DepthPrePassAlphaAnim").As<Graphics::Shader>();

        m_DepthPrePassAnimShader       = Application::Get().GetAssetManager()->GetAssetData("DepthPrePassAnim").As<Graphics::Shader>();
        m_FilmicGrainShader            = Application::Get().GetAssetManager()->GetAssetData("FilmicGrain").As<Graphics::Shader>();
        descriptorDesc.layoutIndex     = 0;
        descriptorDesc.shader          = m_FilmicGrainShader.get();
        m_FilmicGrainPassDescriptorSet = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_SSAOShader               = Application::Get().GetAssetManager()->GetAssetData("SSAO").As<Graphics::Shader>();
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_SSAOShader.get();

        if(m_SSAOShader->IsCompiled())
            m_SSAOPassDescriptorSet = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_SSAOBlurShader           = Application::Get().GetAssetManager()->GetAssetData("SSAOBlur").As<Graphics::Shader>();
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_SSAOBlurShader.get();
        if(m_SSAOBlurShader->IsCompiled())
        {
            m_SSAOBlurPassDescriptorSet  = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
            m_SSAOBlurPassDescriptorSet2 = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        }

        // m_OutlineShader = Application::Get().GetAssetManager()->GetAssetData("Outline");
        //         descriptorDesc.layoutIndex = 0;
        //         descriptorDesc.shader      = m_OutlineShader.get();
        //         m_OutlinePassDescriptorSet    = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        // Setup 2D pass data
        m_Renderer2DData.m_IndexCount           = 0;
        m_Renderer2DData.m_Buffer               = nullptr;
        m_Renderer2DData.m_RenderToDepthTexture = true;
        m_Renderer2DData.m_TriangleIndicies     = false;
        m_Renderer2DData.m_Limits.SetMaxQuads(10000);
        m_Renderer2DData.m_Limits.MaxTextures = 16; // Renderer::GetCapabilities().MaxTextureUnits;

        m_Renderer2DData.m_Shader = Application::Get().GetAssetManager()->GetAssetData("Batch2D").As<Graphics::Shader>();

        m_Renderer2DData.m_TransformationStack.emplace_back(glm::mat4(1.0f));
        m_Renderer2DData.m_TransformationBack = &m_Renderer2DData.m_TransformationStack.back();

        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_Renderer2DData.m_Shader.get();
        m_Renderer2DData.m_DescriptorSet.resize(m_Renderer2DData.m_Limits.MaxBatchDrawCalls);
        m_Renderer2DData.m_PreviousFrameTextureCount.resize(m_Renderer2DData.m_Limits.MaxBatchDrawCalls);

        for(uint32_t i = 0; i < m_Renderer2DData.m_Limits.MaxBatchDrawCalls; i++)
        {
            m_Renderer2DData.m_PreviousFrameTextureCount[i] = 0;
            m_Renderer2DData.m_DescriptorSet[i].resize(2);
            if(i == 0)
            {
                descriptorDesc.layoutIndex             = 0;
                m_Renderer2DData.m_DescriptorSet[0][0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
            }
            descriptorDesc.layoutIndex             = 1;
            m_Renderer2DData.m_DescriptorSet[i][1] = nullptr; // SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        }

        m_Renderer2DData.m_VertexBuffers.resize(Renderer::GetMainSwapChain()->GetSwapChainBufferCount());

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
                indices[i]     = offset + 0;
                indices[i + 1] = offset + 1;
                indices[i + 2] = offset + 2;

                indices[i + 3] = offset + 2;
                indices[i + 4] = offset + 3;
                indices[i + 5] = offset + 0;

                offset += 4;
            }
        }
        m_Renderer2DData.m_IndexBuffer = IndexBuffer::Create(indices, m_Renderer2DData.m_Limits.IndiciesSize);
        m_2DBufferBase.resize(Renderer::GetMainSwapChain()->GetSwapChainBufferCount());

        for(int currentFrame = 0; currentFrame < Renderer::GetMainSwapChain()->GetSwapChainBufferCount(); currentFrame++)
        {
            auto& vertexBuffer = m_Renderer2DData.m_VertexBuffers[currentFrame].emplace_back(Graphics::VertexBuffer::Create(m_Renderer2DData.m_Limits.BufferSize, nullptr, BufferUsage::DYNAMIC));

            m_2DBufferBase[currentFrame].emplace_back(new VertexData[m_Renderer2DData.m_Limits.MaxQuads * 4]);
        }

        for(int i = 0; i < Renderer::GetMainSwapChain()->GetSwapChainBufferCount(); i++)
        {
            TextVertexBufferBase.push_back(new TextVertexData[m_TextRendererData.m_Limits.MaxQuads * 4]);
        }

        delete[] indices;

        {
            // Setup Particle pass data
            m_ParticleData.m_IndexCount           = 0;
            m_ParticleData.m_Buffer               = nullptr;
            m_ParticleData.m_RenderToDepthTexture = true;
            m_ParticleData.m_TriangleIndicies     = false;
            m_ParticleData.m_Limits.SetMaxQuads(10000);
            m_ParticleData.m_Limits.MaxTextures = 16; // Renderer::GetCapabilities().MaxTextureUnits;

            m_ParticleData.m_Shader = Application::Get().GetAssetManager()->GetAssetData("Particle").As<Graphics::Shader>();

            m_ParticleData.m_TransformationStack.emplace_back(glm::mat4(1.0f));
            m_ParticleData.m_TransformationBack = &m_ParticleData.m_TransformationStack.back();

            descriptorDesc.layoutIndex = 0;
            descriptorDesc.shader      = m_ParticleData.m_Shader.get();
            m_ParticleData.m_DescriptorSet.resize(m_ParticleData.m_Limits.MaxBatchDrawCalls);
            m_ParticleData.m_PreviousFrameTextureCount.resize(m_ParticleData.m_Limits.MaxBatchDrawCalls);

            for(uint32_t i = 0; i < m_ParticleData.m_Limits.MaxBatchDrawCalls; i++)
            {
                m_ParticleData.m_PreviousFrameTextureCount[i] = 0;
                m_ParticleData.m_DescriptorSet[i].resize(2);
                if(i == 0)
                {
                    descriptorDesc.layoutIndex           = 0;
                    m_ParticleData.m_DescriptorSet[0][0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
                }
                descriptorDesc.layoutIndex           = 1;
                m_ParticleData.m_DescriptorSet[i][1] = nullptr; // SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
            }

            m_ParticleData.m_VertexBuffers.resize(Renderer::GetMainSwapChain()->GetSwapChainBufferCount());

            uint32_t* indices = new uint32_t[m_ParticleData.m_Limits.IndiciesSize];

            if(m_ParticleData.m_TriangleIndicies)
            {
                for(uint32_t i = 0; i < m_ParticleData.m_Limits.IndiciesSize; i++)
                {
                    indices[i] = i;
                }
            }
            else
            {
                uint32_t offset = 0;
                for(uint32_t i = 0; i < m_ParticleData.m_Limits.IndiciesSize; i += 6)
                {
                    indices[i]     = offset + 0;
                    indices[i + 1] = offset + 1;
                    indices[i + 2] = offset + 2;

                    indices[i + 3] = offset + 2;
                    indices[i + 4] = offset + 3;
                    indices[i + 5] = offset + 0;

                    offset += 4;
                }
            }
            m_ParticleData.m_IndexBuffer = IndexBuffer::Create(indices, m_ParticleData.m_Limits.IndiciesSize);
            m_ParticleBufferBase.resize(Renderer::GetMainSwapChain()->GetSwapChainBufferCount());

            for(int currentFrame = 0; currentFrame < Renderer::GetMainSwapChain()->GetSwapChainBufferCount(); currentFrame++)
            {
                auto& vertexBuffer = m_ParticleData.m_VertexBuffers[currentFrame].emplace_back(Graphics::VertexBuffer::Create(m_ParticleData.m_Limits.BufferSize, nullptr, BufferUsage::DYNAMIC));

                m_ParticleBufferBase[currentFrame].emplace_back(new VertexData[m_ParticleData.m_Limits.MaxQuads * 4]);
            }

            delete[] indices;
        }

        // Setup text pass
        m_TextRendererData.m_IndexCount = 0;
        // m_TextBuffer                              = nullptr;
        m_TextRendererData.m_RenderToDepthTexture = true;
        m_TextRendererData.m_TriangleIndicies     = false;
        m_TextRendererData.m_Limits.SetMaxQuads(10000);
        m_TextRendererData.m_Limits.MaxTextures = 16; // Renderer::GetCapabilities().MaxTextureUnits;

        TextVertexBufferPtr = TextVertexBufferBase[0];

        m_TextRendererData.m_Shader = Application::Get().GetAssetManager()->GetAssetData("Text").As<Graphics::Shader>();

        m_TextRendererData.m_TransformationStack.emplace_back(glm::mat4(1.0f));
        m_TextRendererData.m_TransformationBack = &m_Renderer2DData.m_TransformationStack.back();

        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_Renderer2DData.m_Shader.get();
        m_TextRendererData.m_DescriptorSet.resize(m_Renderer2DData.m_Limits.MaxBatchDrawCalls);
        m_TextRendererData.m_PreviousFrameTextureCount.resize(m_Renderer2DData.m_Limits.MaxBatchDrawCalls);

        for(uint32_t i = 0; i < m_TextRendererData.m_Limits.MaxBatchDrawCalls; i++)
        {
            m_TextRendererData.m_PreviousFrameTextureCount[i] = 0;
            m_TextRendererData.m_DescriptorSet[i].resize(2);
            if(i == 0)
            {
                descriptorDesc.layoutIndex               = 0;
                m_TextRendererData.m_DescriptorSet[0][0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
            }
            descriptorDesc.layoutIndex               = 1;
            m_TextRendererData.m_DescriptorSet[i][1] = nullptr; // SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        }

        m_TextRendererData.m_VertexBuffers.resize(Renderer::GetMainSwapChain()->GetSwapChainBufferCount());
        indices = new uint32_t[m_TextRendererData.m_Limits.IndiciesSize];

        if(m_TextRendererData.m_TriangleIndicies)
        {
            for(uint32_t i = 0; i < m_TextRendererData.m_Limits.IndiciesSize; i++)
            {
                indices[i] = i;
            }
        }
        else
        {
            uint32_t offset = 0;
            for(uint32_t i = 0; i < m_TextRendererData.m_Limits.IndiciesSize; i += 6)
            {
                indices[i]     = offset + 0;
                indices[i + 1] = offset + 1;
                indices[i + 2] = offset + 2;

                indices[i + 3] = offset + 2;
                indices[i + 4] = offset + 3;
                indices[i + 5] = offset + 0;

                offset += 4;
            }
        }
        m_TextRendererData.m_IndexBuffer = IndexBuffer::Create(indices, m_TextRendererData.m_Limits.IndiciesSize);

        delete[] indices;

        InitDebugRenderData();
    }

    RenderPasses::~RenderPasses()
    {
        Memory::AlignedFree(m_ForwardData.m_TransformData);

        delete m_ForwardData.m_DepthTexture;
        delete m_MainTexture;
        delete m_ResolveTexture;
        delete m_PostProcessTexture1;
        delete m_SSAOTexture;
        delete m_SSAOTexture1;
        delete m_NoiseTexture;
        delete m_BloomTexture;
        delete m_BloomTexture1;
        delete m_BloomTexture2;
        delete m_NormalTexture;

        delete m_ShadowData.m_ShadowTex;
        delete m_ForwardData.m_DefaultMaterial;
        delete m_DefaultTextureCube;
        delete m_ScreenQuad;

        delete m_Renderer2DData.m_IndexBuffer;
        delete m_TextRendererData.m_IndexBuffer;
        delete m_ParticleData.m_IndexBuffer;

        if(m_DebugRenderDataInitialised)
        {
            delete m_DebugTextRendererData.m_IndexBuffer;
            delete m_DebugDrawData.m_Renderer2DData.m_IndexBuffer;
            delete m_DebugDrawData.m_LineIndexBuffer;
            delete m_DebugDrawData.m_PointIndexBuffer;
        }

        for(auto data : TextVertexBufferBase)
            delete[] data;
        for(auto data : DebugTextVertexBufferBase)
            delete[] data;

        for(auto& lineBufferBase : m_LineBufferBase)
            for(auto data : lineBufferBase)
                delete[] data;
        for(auto& pointBufferBase : m_PointBufferBase)
            for(auto data : pointBufferBase)
                delete[] data;
        for(auto& quadBufferBase : m_QuadBufferBase)
            for(auto data : quadBufferBase)
                delete[] data;

        for(int j = 0; j < Renderer::GetMainSwapChain()->GetSwapChainBufferCount(); j++)
        {
            for(size_t i = 0; i < m_Renderer2DData.m_VertexBuffers[j].size(); i++)
            {
                delete m_Renderer2DData.m_VertexBuffers[j][i];
            }

            for(uint32_t i = 0; i < m_TextRendererData.m_VertexBuffers[j].size(); i++)
            {
                delete m_TextRendererData.m_VertexBuffers[j][i];
            }

            for(size_t i = 0; i < m_ParticleData.m_VertexBuffers[j].size(); i++)
            {
                delete m_ParticleData.m_VertexBuffers[j][i];
            }

            if(m_DebugRenderDataInitialised)
                for(uint32_t i = 0; i < m_DebugTextRendererData.m_VertexBuffers[j].size(); i++)
                {
                    delete m_DebugTextRendererData.m_VertexBuffers[j][i];
                }

            if(m_DebugRenderDataInitialised)
                for(size_t i = 0; i < m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[j].size(); i++)
                {
                    delete m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[j][i];
                }

            for(auto data : m_2DBufferBase[j])
                delete[] data;

            for(auto data : m_ParticleBufferBase[j])
                delete[] data;
        }

        if(m_DebugRenderDataInitialised)
        {
            if(!m_DebugDrawData.m_PointVertexBuffers.empty())
                for(int i = 0; i < m_DebugDrawData.m_PointVertexBuffers[0].size(); i++)
                {
                    delete m_DebugDrawData.m_PointVertexBuffers[0][i];
                }

            if(!m_DebugDrawData.m_LineVertexBuffers.empty())
                for(int i = 0; i < m_DebugDrawData.m_LineVertexBuffers[0].size(); i++)
                {
                    delete m_DebugDrawData.m_LineVertexBuffers[0][i];
                }
        }

        DebugRenderer::Release();
    }

    void RenderPasses::OnResize(uint32_t width, uint32_t height)
    {
        LUMOS_PROFILE_FUNCTION();

        width -= (width % 2 != 0) ? 1 : 0;
        height -= (height % 2 != 0) ? 1 : 0;

        const QualitySettings& qs = Application::Get().GetQualitySettings();
        width                     = uint32_t(qs.RendererScale * float(width));
        height                    = uint32_t(qs.RendererScale * float(height));

        m_ForwardData.m_DepthTexture->Resize(width, height);
        m_MainTexture->Resize(width, height);
        m_ResolveTexture->Resize(width, height);
        m_PostProcessTexture1->Resize(width, height);
        m_SSAOTexture->Resize(width / 2, height / 2);
        m_SSAOTexture1->Resize(width / 2, height / 2);
        m_BloomTexture->Resize(width, height);
        m_BloomTexture1->Resize(width, height);
        m_BloomTexture2->Resize(width, height);
        m_NormalTexture->Resize(width, height);
    }

    void RenderPasses::EnableDebugRenderer(bool enable)
    {
        m_DebugRenderEnabled = enable;

        if(m_DebugRenderEnabled)
        {
            DebugRenderer::Init();
            InitDebugRenderData();
        }
        else
            DebugRenderer::Release();
    }

    void RenderPasses::BeginScene(Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& registry             = scene->GetRegistry();
        m_CurrentScene             = scene;
        m_Stats.FramesPerSecond    = 0;
        m_Stats.NumDrawCalls       = 0;
        m_Stats.NumRenderedObjects = 0;
        m_Stats.NumShadowObjects   = 0;
        m_Stats.UpdatesPerSecond   = 0;

        m_Renderer2DData.m_BatchDrawCallIndex        = 0;
        m_TextRendererData.m_BatchDrawCallIndex      = 0;
        m_DebugTextRendererData.m_BatchDrawCallIndex = 0;
        m_ParticleData.m_BatchDrawCallIndex          = 0;

        if(m_OverrideCamera)
        {
            m_Camera          = m_OverrideCamera;
            m_CameraTransform = m_OverrideCameraTransform;
        }
        else
        {
            auto cameraView = registry.view<Camera>();
            if(!cameraView.empty())
            {
                m_Camera          = &cameraView.get<Camera>(cameraView.front());
                m_CameraTransform = registry.try_get<Maths::Transform>(cameraView.front());
            }
        }

        if(!m_Camera || !m_CameraTransform)
            return;

        m_Exposure     = m_Camera->GetExposure();
        m_ToneMapIndex = scene->GetSettings().RenderSettings.m_ToneMapIndex;

        if(m_MainTextureSamples != scene->GetSettings().RenderSettings.MSAASamples)
        {
            m_MainTextureSamples = scene->GetSettings().RenderSettings.MSAASamples;

            uint32_t width  = m_MainTexture->GetWidth();
            uint32_t height = m_MainTexture->GetHeight();

            delete m_MainTexture;
            delete m_ForwardData.m_DepthTexture;
            delete m_NormalTexture;
            delete m_SSAOTexture;
            delete m_SSAOTexture1;

            Graphics::TextureDesc mainRenderTargetDesc;
            mainRenderTargetDesc.format          = Graphics::RHIFormat::R11G11B10_Float;
            mainRenderTargetDesc.flags           = TextureFlags::Texture_RenderTarget;
            mainRenderTargetDesc.wrap            = TextureWrap::CLAMP_TO_EDGE;
            mainRenderTargetDesc.minFilter       = TextureFilter::LINEAR;
            mainRenderTargetDesc.magFilter       = TextureFilter::LINEAR;
            mainRenderTargetDesc.generateMipMaps = false;
            mainRenderTargetDesc.samples         = m_MainTextureSamples;
            m_MainTexture                        = Graphics::Texture2D::Create(mainRenderTargetDesc, width, height);
            m_ForwardData.m_DepthTexture         = TextureDepth::Create(width, height, Renderer::GetRenderer()->GetDepthFormat(), m_MainTextureSamples);

            mainRenderTargetDesc.generateMipMaps      = false;
            mainRenderTargetDesc.anisotropicFiltering = false;
            mainRenderTargetDesc.flags                = TextureFlags::Texture_RenderTarget;
            mainRenderTargetDesc.wrap                 = TextureWrap::CLAMP_TO_EDGE;
            mainRenderTargetDesc.minFilter            = TextureFilter::NEAREST;
            mainRenderTargetDesc.magFilter            = TextureFilter::NEAREST;
            mainRenderTargetDesc.samples              = m_MainTextureSamples;
            mainRenderTargetDesc.format               = Graphics::RHIFormat::R32G32B32A32_Float;
            m_NormalTexture                           = Graphics::Texture2D::Create(mainRenderTargetDesc, width, height);

            m_SSAOTexture  = Graphics::Texture2D::Create(mainRenderTargetDesc, width / 2, height / 2);
            m_SSAOTexture1 = Graphics::Texture2D::Create(mainRenderTargetDesc, width / 2, height / 2);
        }

        auto view        = glm::inverse(m_CameraTransform->GetWorldMatrix());
        const auto& proj = m_Camera->GetProjectionMatrix();
        auto projView    = proj * view;

        if(m_DebugRenderEnabled)
        {
            DebugRenderer::GetInstance()->SetDimensions(m_MainTexture->GetWidth(), m_MainTexture->GetHeight());
            DebugRenderer::GetInstance()->SetProjView(projView);
        }

        Scene::SceneRenderSettings& renderSettings = scene->GetSettings().RenderSettings;

        if(renderSettings.Renderer3DEnabled)
        {
            m_ForwardData.m_DescriptorSet[0]->SetUniform("UBO", "projView", &projView);
            m_ForwardData.m_DescriptorSet[0]->Update();
        }

        if(renderSettings.SkyboxRenderEnabled || renderSettings.Renderer3DEnabled)
        {
            auto envView = registry.view<Graphics::Environment>();

            if(envView.size() == 0)
            {
                if(m_ForwardData.m_EnvironmentMap != m_DefaultTextureCube)
                {
                    m_ForwardData.m_EnvironmentMap = m_DefaultTextureCube;
                    m_ForwardData.m_IrradianceMap  = m_DefaultTextureCube;

                    // TODO: remove need for this
                    Graphics::DescriptorDesc info {};
                    info.shader                      = m_ForwardData.m_Shader.get();
                    info.layoutIndex                 = 2;
                    m_ForwardData.m_DescriptorSet[2] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));

                    m_CubeMap = nullptr;
                    Graphics::DescriptorDesc descriptorDesc {};
                    descriptorDesc.layoutIndex = 0;
                    descriptorDesc.shader      = m_SkyboxShader.get();
                    m_SkyboxDescriptorSet      = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
                }
            }
            else
            {
                // Just use first
                const auto& env = envView.get<Graphics::Environment>(envView.front());

                if(m_ForwardData.m_EnvironmentMap != env.GetEnvironmentMap())
                {
                    Graphics::DescriptorDesc info    = {};
                    info.shader                      = m_ForwardData.m_Shader.get();
                    info.layoutIndex                 = 2;
                    m_ForwardData.m_DescriptorSet[2] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));
                    m_ForwardData.m_EnvironmentMap   = env.GetEnvironmentMap();
                    m_ForwardData.m_IrradianceMap    = env.GetIrradianceMap();
                    const bool viewIrradiance        = false;

                    m_CubeMap = viewIrradiance ? env.GetIrradianceMap() : env.GetEnvironmentMap();
                }
            }

            auto invProj = glm::inverse(proj);
            auto invView = glm::inverse(view);

            m_SkyboxDescriptorSet->SetUniform("UBO", "invProjection", &invProj);
            m_SkyboxDescriptorSet->SetUniform("UBO", "invView", &invView);
        }

        Light* directionaLight = nullptr;
        static Light lights[256];
        uint32_t numLights = 0;

        m_ForwardData.m_Frustum = m_Camera->GetFrustum(view);

        auto emitterGroup = registry.group<ParticleEmitter>(entt::get<Maths::Transform>);
        for(auto& emitterEntity : emitterGroup)
        {
            const auto& [emitter, trans] = emitterGroup.get<ParticleEmitter, Maths::Transform>(emitterEntity);
            emitter.Update((float)Engine::GetTimeStep().GetSeconds(), trans.GetWorldPosition());
        }

        if(renderSettings.Renderer3DEnabled)
        {
            {
                LUMOS_PROFILE_SCOPE("Get Light");
                auto group = registry.group<Graphics::Light>(entt::get<Maths::Transform>);

                for(auto& lightEntity : group)
                {
                    if(!Entity(lightEntity, scene).Active())
                        continue;

                    if(numLights >= 64)
                        break;

                    const auto& [light, trans] = group.get<Graphics::Light, Maths::Transform>(lightEntity);
                    light.Position             = glm::vec4(trans.GetWorldPosition(), 1.0f);
                    glm::vec3 forward          = glm::vec3(0.0f, 0.0f, 1.0f);
                    forward                    = trans.GetWorldOrientation() * forward;
                    forward                    = glm::normalize(forward);
                    light.Direction            = glm::vec4(forward, 1.0f);

                    if(light.Type == (float)Graphics::LightType::DirectionalLight)
                        directionaLight = &light;

                    if(light.Type != float(LightType::DirectionalLight))
                    {
                        auto inside = m_ForwardData.m_Frustum.IsInside(Maths::BoundingSphere(glm::vec3(light.Position), light.Radius * 100));

                        if(inside == Intersection::OUTSIDE)
                            continue;
                    }

                    lights[numLights] = light;
                    lights[numLights].Intensity *= m_Exposure;
                    numLights++;
                }
            }

            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformSceneData", "lights", lights, sizeof(Graphics::Light) * numLights);

            glm::vec4 cameraPos = glm::vec4(m_CameraTransform->GetWorldPosition(), 1.0f);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformSceneData", "cameraPosition", &cameraPos);
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

        m_ForwardData.m_CommandQueue.clear();

        auto& shadowData            = GetShadowData();
        glm::mat4* shadowTransforms = shadowData.m_ShadowProjView;
        glm::vec4* uSplitDepth      = shadowData.m_SplitDepth;
        glm::mat4 LightView         = shadowData.m_LightMatrix;
        float bias                  = shadowData.m_InitialBias;
        float MaxShadowDist         = shadowData.m_MaxShadowDistance;
        float LightSize             = (float)shadowData.m_ShadowMapSize;
        float transitionFade        = shadowData.m_CascadeFade;
        float ShadowFade            = shadowData.m_ShadowFade;
        float width                 = (float)m_MainTexture->GetWidth();
        float height                = (float)m_MainTexture->GetHeight();

        int shadowEnabled = renderSettings.ShadowsEnabled ? 1 : 0;
        if(renderSettings.Renderer3DEnabled)
        {
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformSceneData", "ViewMatrix", &view);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformSceneData", "LightView", &LightView);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformSceneData", "ShadowTransform", shadowTransforms);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformSceneData", "SplitDepths", uSplitDepth);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformSceneData", "BiasMatrix", &m_ForwardData.m_BiasMatrix);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformSceneData", "LightSize", &LightSize);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformSceneData", "ShadowFade", &ShadowFade);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformSceneData", "CascadeFade", &transitionFade);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformSceneData", "MaxShadowDist", &MaxShadowDist);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformSceneData", "InitialBias", &bias);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformSceneData", "Width", &width);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformSceneData", "Height", &height);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformSceneData", "shadowEnabled", &shadowEnabled);

            m_ForwardData.m_DescriptorSet[2]->SetTexture("uShadowMap", reinterpret_cast<Texture*>(shadowData.m_ShadowTex), 0, TextureType::DEPTHARRAY);

            int numShadows   = shadowData.m_ShadowMapNum;
            auto EnvMipCount = m_ForwardData.m_EnvironmentMap ? m_ForwardData.m_EnvironmentMap->GetMipMapLevels() : 0;
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformSceneData", "LightCount", &numLights);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformSceneData", "ShadowCount", &numShadows);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformSceneData", "Mode", &m_ForwardData.m_RenderMode);
            m_ForwardData.m_DescriptorSet[2]->SetUniform("UniformSceneData", "EnvMipCount", &EnvMipCount);
            m_ForwardData.m_DescriptorSet[2]->SetTexture("uBRDFLUT", m_ForwardData.m_BRDFLUT.get());
            m_ForwardData.m_DescriptorSet[2]->SetTexture("uSSAOMap", Application::Get().GetCurrentScene()->GetSettings().RenderSettings.SSAOEnabled ? m_SSAOTexture : Material::GetDefaultTexture().get());
            m_ForwardData.m_DescriptorSet[2]->SetTexture("uEnvMap", m_ForwardData.m_EnvironmentMap, 0, TextureType::CUBE);
            m_ForwardData.m_DescriptorSet[2]->SetTexture("uIrrMap", m_ForwardData.m_IrradianceMap, 0, TextureType::CUBE);
            m_ForwardData.m_DescriptorSet[2]->Update();

            glm::mat4 boneTransforms[100];
            for(int i = 0; i < 100; i++)
            {
                boneTransforms[i] = glm::mat4(1.0f);
            }
            m_ForwardData.m_DescriptorSet[3]->SetUniform("BoneTransforms", "BoneTransforms", boneTransforms, sizeof(glm::mat4) * 100);
            m_ForwardData.m_DescriptorSet[3]->Update();

            auto group = registry.group<ModelComponent>(entt::get<Maths::Transform>);

            Graphics::PipelineDesc pipelineDesc = {};
            pipelineDesc.shader                 = m_ForwardData.m_Shader;
            pipelineDesc.polygonMode            = Graphics::PolygonMode::FILL;
            pipelineDesc.blendMode              = BlendMode::SrcAlphaOneMinusSrcAlpha;
            pipelineDesc.clearTargets           = false;
            pipelineDesc.swapchainTarget        = false;

            Graphics::PipelineDesc shadowPipelineDesc;
            shadowPipelineDesc.cullMode                = Graphics::CullMode::FRONT;
            shadowPipelineDesc.depthArrayTarget        = reinterpret_cast<Texture*>(m_ShadowData.m_ShadowTex);
            shadowPipelineDesc.depthBiasEnabled        = false;
            shadowPipelineDesc.depthBiasConstantFactor = 0.0f;
            shadowPipelineDesc.depthBiasSlopeFactor    = 0.0f;
            shadowPipelineDesc.DebugName               = "Shadow";
            shadowPipelineDesc.clearTargets            = false;

            for(auto entity : group)
            {
                if(!Entity(entity, scene).Active())
                    continue;

                const auto& [model, trans] = group.get<ModelComponent, Maths::Transform>(entity);

                if(!model.ModelRef)
                    continue;

                const auto& meshes = model.ModelRef->GetMeshes();

                for(auto mesh : meshes)
                {
                    if(!mesh->GetActive())
                        continue;

                    auto& worldTransform = trans.GetWorldMatrix();
                    auto bbCopy          = mesh->GetBoundingBox()->Transformed(worldTransform);

                    if(directionaLight)
                    {
                        for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; i++)
                        {
                            auto inside = m_ShadowData.m_CascadeFrustums[i].IsInside(bbCopy);

                            if(!inside)
                                continue;

                            RenderCommand command;
                            command.mesh      = mesh.get();
                            command.transform = worldTransform;
                            command.material  = mesh->GetMaterial() ? mesh->GetMaterial().get() : m_ForwardData.m_DefaultMaterial;

                            if(command.material->GetFlag(Material::RenderFlags::NOSHADOW))
                                continue;

                            Material* material = command.material ? command.material : m_ForwardData.m_DefaultMaterial;
                            bool alphaBlend    = material->GetFlag(Material::RenderFlags::ALPHABLEND);

                            shadowPipelineDesc.transparencyEnabled = alphaBlend;
                            shadowPipelineDesc.shader              = alphaBlend ? m_ShadowData.m_ShaderAlpha : m_ShadowData.m_Shader;

                            // Bind here in case not bound in the loop below as meshes will be inside
                            // cascade frustum and not the cameras
                            command.material->Bind();

                            if(mesh->GetAnimVertexBuffer())
                            {
                                shadowPipelineDesc.shader = alphaBlend ? m_ShadowData.m_ShaderAnimAlpha : m_ShadowData.m_ShaderAnim;

                                command.animated              = true;
                                command.AnimatedDescriptorSet = model.ModelRef->GetAnimationController() ? model.ModelRef->GetAnimationController()->GetDescriptorSet() : m_ForwardData.m_DescriptorSet[3];
                            }
                            command.pipeline = Graphics::Pipeline::Get(shadowPipelineDesc);

                            m_ShadowData.m_CascadeCommandQueue[i].push_back(command);
                        }
                    }

                    {
                        auto inside = m_ForwardData.m_Frustum.IsInside(bbCopy);

                        if(!inside)
                            continue;

                        RenderCommand command;
                        command.mesh      = mesh;
                        command.transform = worldTransform;
                        command.material  = mesh->GetMaterial() ? mesh->GetMaterial().get() : m_ForwardData.m_DefaultMaterial;

                        // Update material buffers
                        command.material->Bind();

                        pipelineDesc.colourTargets[0]    = m_MainTexture;
                        pipelineDesc.cullMode            = command.material->GetFlag(Material::RenderFlags::TWOSIDED) ? Graphics::CullMode::NONE : Graphics::CullMode::BACK;
                        pipelineDesc.transparencyEnabled = command.material->GetFlag(Material::RenderFlags::ALPHABLEND);
                        pipelineDesc.samples             = m_MainTextureSamples;
                        if(m_MainTextureSamples > 1)
                            pipelineDesc.resolveTexture = m_ResolveTexture;
                        if(m_ForwardData.m_DepthTest && command.material->GetFlag(Material::RenderFlags::DEPTHTEST))
                        {
                            pipelineDesc.depthTarget = m_ForwardData.m_DepthTexture;
                        }

                        if(mesh->GetAnimVertexBuffer())
                        {
                            pipelineDesc.shader           = m_ForwardData.m_AnimShader;
                            command.animated              = true;
                            command.AnimatedDescriptorSet = model.ModelRef->GetAnimationController() ? model.ModelRef->GetAnimationController()->GetDescriptorSet() : m_ForwardData.m_DescriptorSet[3];
                        }
                        else
                            pipelineDesc.shader = m_ForwardData.m_Shader;
#ifndef LUMOS_PRODUCTION
                        static const char* debugName0 = "Forward PBR Transparent DepthTested";
                        static const char* debugName1 = "Forward PBR DepthTested";
                        static const char* debugName2 = "Forward PBR Transparent";
                        static const char* debugName3 = "Forward PBR";

                        if(pipelineDesc.depthTarget && pipelineDesc.transparencyEnabled)
                        {
                            pipelineDesc.DebugName = debugName0;
                        }
                        else if(pipelineDesc.depthTarget)
                        {
                            pipelineDesc.DebugName = debugName1;
                        }
                        else if(pipelineDesc.transparencyEnabled)
                        {
                            pipelineDesc.DebugName = debugName2;
                        }
                        else
                        {
                            pipelineDesc.DebugName = debugName3;
                        }
#endif

                        command.pipeline = Graphics::Pipeline::Get(pipelineDesc);
                        m_ForwardData.m_CommandQueue.push_back(command);
                    }
                }
            }
        }

        m_Renderer2DData.m_CommandQueue2D.clear();

        if(renderSettings.Renderer2DEnabled)
        {
            auto spriteGroup = registry.group<Graphics::Sprite>(entt::get<Maths::Transform>);
            for(auto entity : spriteGroup)
            {
                const auto& [sprite, trans] = spriteGroup.get<Graphics::Sprite, Maths::Transform>(entity);

                auto bb = Maths::BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetScale()));
                bb.Transform(trans.GetWorldMatrix());
                auto inside = m_ForwardData.m_Frustum.IsInside(bb);

                if(!inside)
                    continue;

                RenderCommand2D command;
                command.renderable = &sprite;
                command.transform  = trans.GetWorldMatrix();
                m_Renderer2DData.m_CommandQueue2D.push_back(command);
            };

            auto animSpriteGroup = registry.group<Graphics::AnimatedSprite>(entt::get<Maths::Transform>);
            for(auto entity : animSpriteGroup)
            {
                const auto& [sprite, trans] = animSpriteGroup.get<Graphics::AnimatedSprite, Maths::Transform>(entity);

                auto bb = Maths::BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetScale()));
                bb.Transform(trans.GetWorldMatrix());
                auto inside = m_ForwardData.m_Frustum.IsInside(bb);

                if(!inside)
                    continue;

                RenderCommand2D command;
                command.renderable = &sprite;
                command.transform  = trans.GetWorldMatrix();
                m_Renderer2DData.m_CommandQueue2D.push_back(command);
            };

            {
                LUMOS_PROFILE_SCOPE("Sort Meshes by distance from camera");
                auto camTransform = m_CameraTransform;
                std::sort(m_ForwardData.m_CommandQueue.begin(), m_ForwardData.m_CommandQueue.end(),
                          [camTransform](RenderCommand& a, RenderCommand& b)
                          {
                              if(a.material->GetFlag(Material::RenderFlags::DEPTHTEST) && !b.material->GetFlag(Material::RenderFlags::DEPTHTEST))
                                  return true;
                              if(!a.material->GetFlag(Material::RenderFlags::DEPTHTEST) && b.material->GetFlag(Material::RenderFlags::DEPTHTEST))
                                  return false;

                              return glm::length(camTransform->GetWorldPosition() - glm::vec3(a.transform[3])) < glm::length(camTransform->GetWorldPosition() - glm::vec3(b.transform[3]));
                          });
            }

            {
                LUMOS_PROFILE_SCOPE("Sort sprites by z value");
                std::sort(m_Renderer2DData.m_CommandQueue2D.begin(), m_Renderer2DData.m_CommandQueue2D.end(),
                          [](RenderCommand2D& a, RenderCommand2D& b)
                          {
                              return a.transform[3].z < b.transform[3].z;
                          });
            }
        }
    }

    void RenderPasses::SetRenderTarget(Graphics::Texture* texture, bool onlyIfTargetsScreen, bool rebuildFramebuffer)
    {
        LUMOS_PROFILE_FUNCTION();
        m_ForwardData.m_RenderTexture = texture;
    }

    void RenderPasses::OnRender()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Render Passes");

        auto& sceneRenderSettings = Application::Get().GetCurrentScene()->GetSettings().RenderSettings;

        {
            LUMOS_PROFILE_GPU("Clear Main Texture Pass");
            Renderer::GetRenderer()->ClearRenderTarget(m_MainTexture, Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());
            Renderer::GetRenderer()->ClearRenderTarget(m_ResolveTexture, Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());
        }

        if(sceneRenderSettings.SSAOEnabled)
        {
            LUMOS_PROFILE_GPU("Clear Normal Texture Pass");
            Renderer::GetRenderer()->ClearRenderTarget(m_NormalTexture, Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());
        }

        if(sceneRenderSettings.ShadowsEnabled)
        {
            LUMOS_PROFILE_GPU("Clear Shadow Texture Pass");
            Renderer::GetMainSwapChain()->GetCurrentCommandBuffer()->UnBindPipeline();
            Renderer::GetRenderer()->ClearRenderTarget(m_ShadowData.m_ShadowTex, Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());
        }

        // Set to default texture if bloom disabled
        m_BloomTextureLastRenderered = Graphics::Material::GetDefaultTexture().get();

        if(m_ForwardData.m_DepthTest)
        {
            LUMOS_PROFILE_GPU("Clear Depth Texture Pass");
            Renderer::GetRenderer()->ClearRenderTarget(reinterpret_cast<Texture*>(m_ForwardData.m_DepthTexture), Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());
        }

        GenerateBRDFLUTPass();

        if(sceneRenderSettings.DepthPrePass)
            DepthPrePass();

        if(sceneRenderSettings.SSAOEnabled && !m_DisablePostProcess)
        {
            SSAOPass();

            if(sceneRenderSettings.SSAOBlur)
                SSAOBlurPass();
        }

        if(sceneRenderSettings.ShadowsEnabled)
            ShadowPass();
        if(sceneRenderSettings.Renderer3DEnabled)
            ForwardPass();
        if(sceneRenderSettings.SkyboxRenderEnabled)
            SkyboxPass();
        if(sceneRenderSettings.Renderer3DEnabled)
            ParticlePass();
        if(sceneRenderSettings.Renderer2DEnabled)
            Render2DPass();

        TextPass();

        if(m_DebugRenderEnabled && sceneRenderSettings.DebugRenderEnabled)
            DebugPass();

        m_LastRenderTarget = m_MainTextureSamples > 1 ? m_ResolveTexture : m_MainTexture;

        // if (sceneRenderSettings.EyeAdaptation)
        //    EyeAdaptationPass();

        if(sceneRenderSettings.DepthOfFieldEnabled && !m_DisablePostProcess)
            DepthOfFieldPass();

        if(sceneRenderSettings.BloomEnabled && !m_DisablePostProcess)
            BloomPass();

        if(sceneRenderSettings.DebandingEnabled && !m_DisablePostProcess)
            DebandingPass();

        ToneMappingPass();

        if(sceneRenderSettings.SharpenEnabled && !m_DisablePostProcess)
            SharpenPass();

        if(sceneRenderSettings.FXAAEnabled && !m_DisablePostProcess)
            FXAAPass();

        if(sceneRenderSettings.ChromaticAberationEnabled && !m_DisablePostProcess)
            ChromaticAberationPass();

        if(sceneRenderSettings.FilmicGrainEnabled && !m_DisablePostProcess)
            FilmicGrainPass();

        // if(sceneRenderSettings.OutlineEnabled
        // OutlinePass();

        FinalPass();
    }

    void RenderPasses::OnUpdate(const TimeStep& timeStep, Scene* scene)
    {
    }

    bool RenderPasses::OnWindowResizeEvent(WindowResizeEvent& e)
    {
        LUMOS_PROFILE_FUNCTION();

        return false;
    }

    void RenderPasses::OnEvent(Event& e)
    {
        LUMOS_PROFILE_FUNCTION();
        // EventDispatcher dispatcher(e);
        // dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(RenderPasses::OnwindowResizeEvent));
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

    void RenderPasses::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();

        ImGui::TextUnformatted("Shadow Renderer");

        if(ImGui::TreeNode("Texture"))
        {
            static int index = 0;

            // TODO: Fix - only showing layer 0
            ImGui::SliderInt("Texture Array Index", &index, 0, m_ShadowData.m_ShadowTex->GetCount() - 1);

            bool flipImage = Renderer::GetGraphicsContext()->FlipImGUITexture();

            ImGuiUtilities::Image(m_ShadowData.m_ShadowTex, index, ImVec2(128, 128));

            if(ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGuiUtilities::Image(m_ShadowData.m_ShadowTex, index, ImVec2(256, 256));
                ImGui::EndTooltip();
            }

            ImGui::TreePop();
        }

        ImGui::DragFloat("Initial Bias", &m_ShadowData.m_InitialBias, 0.00005f, 0.0f, 1.0f, "%.6f");
        ImGui::DragFloat("Light Size", &m_ShadowData.m_LightSize, 0.00005f, 0.0f, 10.0f);
        ImGui::DragFloat("Max Shadow Distance", &m_ShadowData.m_MaxShadowDistance, 0.05f, 0.0f, 10000.0f);
        ImGui::DragFloat("Shadow Fade", &m_ShadowData.m_ShadowFade, 0.0005f, 0.0f, 500.0f);
        ImGui::DragFloat("Cascade Transition Fade", &m_ShadowData.m_CascadeFade, 0.0005f, 0.0f, 5.0f);

        ImGui::DragFloat("Cascade Split Lambda", &m_ShadowData.m_CascadeSplitLambda, 0.005f, 0.0f, 3.0f);

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
        ImGui::TextUnformatted("2D renderer");
        ImGui::Columns(2);

        ImGuiUtilities::Property("Number of draw calls", (int&)m_Renderer2DData.m_BatchDrawCallIndex, ImGuiUtilities::PropertyFlag::ReadOnly);
        ImGuiUtilities::Property("Max textures Per draw call", (int&)m_Renderer2DData.m_Limits.MaxTextures, 1, 16);
        ImGuiUtilities::Property("Exposure", m_Exposure);

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    void RenderPasses::OnNewScene(Scene* scene)
    {
        m_ForwardData.m_EnvironmentMap = m_DefaultTextureCube;
        m_ForwardData.m_IrradianceMap  = m_DefaultTextureCube;

        Graphics::DescriptorDesc info    = {};
        info.shader                      = m_ForwardData.m_Shader.get();
        info.layoutIndex                 = 2;
        m_ForwardData.m_DescriptorSet[2] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));
        m_CubeMap                        = nullptr;

        Graphics::DescriptorDesc descriptorDesc = {};
        descriptorDesc.layoutIndex              = 0;
        descriptorDesc.shader                   = m_SkyboxShader.get();
        m_SkyboxDescriptorSet                   = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
    }

    float RoundUpToNearestMultipleOf5(float value)
    {
        // Divide the value by 5 and round it up to the nearest integer
        int roundedValue = static_cast<int>(std::ceil(value / 5.0));

        // Multiply the rounded value by 5 to get the nearest multiple of 5
        float result = roundedValue * 5.0f;

        return result;
    }

    void RenderPasses::UpdateCascades(Scene* scene, Light* light)
    {
        LUMOS_PROFILE_FUNCTION();
        float cascadeSplits[SHADOWMAP_MAX];
        float cascadeRadius[SHADOWMAP_MAX];

        float nearClip  = m_Camera->GetNear();
        float farClip   = m_ShadowData.m_MaxShadowDistance * 1.2f; // m_Camera->GetFar();
        float clipRange = farClip - nearClip;

        float minZ  = nearClip;
        float maxZ  = nearClip + clipRange;
        float range = maxZ - minZ;
        float ratio = maxZ / minZ;
        // Calculate split depths based on view camera frustum
        // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
        for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; i++)
        {
            float p          = static_cast<float>(i + 1) / static_cast<float>(m_ShadowData.m_ShadowMapNum);
            float log        = minZ * std::pow(ratio, p);
            float uniform    = minZ + range * p;
            float d          = m_ShadowData.m_CascadeSplitLambda * (log - uniform) + uniform;
            cascadeSplits[i] = (d - nearClip) / clipRange;
        }

        cascadeSplits[3]     = 0.35f;
        float lastSplitDist  = 0.0f;
        glm::mat4 CameraProj = glm::perspective(glm::radians(m_Camera->GetFOV()), m_Camera->GetAspectRatio(), nearClip, farClip);

        const glm::mat4 invCam = glm::inverse(CameraProj * glm::inverse(m_CameraTransform->GetWorldMatrix()));

        for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; i++)
        {
            LUMOS_PROFILE_SCOPE("Create Cascade");
            float splitDist = cascadeSplits[i];

            glm::vec3 frustumCorners[8] = {
                glm::vec3(-1.0f, 1.0f, 0.0f),
                glm::vec3(1.0f, 1.0f, 0.0f),
                glm::vec3(1.0f, -1.0f, 0.0f),
                glm::vec3(-1.0f, -1.0f, 0.0f),
                glm::vec3(-1.0f, 1.0f, 1.0f),
                glm::vec3(1.0f, 1.0f, 1.0f),
                glm::vec3(1.0f, -1.0f, 1.0f),
                glm::vec3(-1.0f, -1.0f, 1.0f),
            };

            // Project frustum corners into world space
            for(uint32_t j = 0; j < 8; j++)
            {
                glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[j], 1.0f);
                frustumCorners[j]   = (invCorner / invCorner.w);
            }

            for(uint32_t j = 0; j < 4; j++)
            {
                glm::vec3 dist        = frustumCorners[j + 4] - frustumCorners[j];
                frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
                frustumCorners[j]     = frustumCorners[j] + (dist * lastSplitDist);
            }

            lastSplitDist = cascadeSplits[i];

            // Get frustum center
            glm::vec3 frustumCenter = glm::vec3(0.0f);
            for(uint32_t j = 0; j < 8; j++)
            {
                frustumCenter += frustumCorners[j];
            }
            frustumCenter /= 8.0f;

            float radius = 0.0f;
            for(uint32_t j = 0; j < 8; j++)
            {
                float distance = glm::distance(frustumCorners[j], frustumCenter);
                radius         = Maths::Max(radius, distance);
            }

            // Temp work around to flickering when rotating camera
            // Sphere radius for lightOrthoMatrix should fix this
            // But radius changes as the camera is rotated which causes flickering
            // const float value = 16.0f;
            // radius = std::ceil(radius *value) / value;

            static const float roundTo[8] = { 5.0f, 5.0f, 20.0f, 200.0f, 400.0f, 400.0f, 400.0f, 400.0f };
            radius                        = RoundUpToNearestMultipleOf5(radius);

            cascadeRadius[i] = radius;

            glm::vec3 maxExtents = glm::vec3(radius);
            glm::vec3 minExtents = -maxExtents;

            glm::vec3 lightDir         = glm::normalize(-light->Direction);
            glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, m_ShadowData.CascadeNearPlaneOffset, maxExtents.z - minExtents.z + m_ShadowData.CascadeFarPlaneOffset);
            glm::mat4 LightViewMatrix  = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 0.0f, 1.0f));

            auto shadowProj = lightOrthoMatrix * LightViewMatrix;

            const bool StabilizeCascades = true;
            if(StabilizeCascades)
            {
                // Create the rounding matrix, by projecting the world-space origin and determining
                // the fractional offset in texel space
                glm::vec4 shadowOrigin = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                shadowOrigin           = shadowProj * shadowOrigin;
                shadowOrigin *= (m_ShadowData.m_ShadowMapSize * 0.5f);

                glm::vec4 roundedOrigin = glm::round(shadowOrigin);
                glm::vec4 roundOffset   = roundedOrigin - shadowOrigin;
                roundOffset             = roundOffset * (2.0f / m_ShadowData.m_ShadowMapSize);
                roundOffset.z           = 0.0f;
                roundOffset.w           = 0.0f;

                lightOrthoMatrix[3] += roundOffset;
            }
            // Store split distance and matrix in cascade
            m_ShadowData.m_SplitDepth[i]     = glm::vec4((m_Camera->GetNear() + splitDist * clipRange) * -1.0f);
            m_ShadowData.m_ShadowProjView[i] = lightOrthoMatrix * LightViewMatrix;

            if(i == 0)
                m_ShadowData.m_LightMatrix = glm::inverse(LightViewMatrix);
        }
    }

    void RenderPasses::GenerateBRDFLUTPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("BRDF Pass");

        if(!m_GenerateBRDFLUT)
            return;

        m_GenerateBRDFLUT = false;

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader = Application::Get().GetAssetManager()->GetAssetData("BRDFLUT").As<Graphics::Shader>();

        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.colourTargets[0]    = m_ForwardData.m_BRDFLUT.get();
        pipelineDesc.DebugName           = "BRDF Generation";

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        auto pipeline      = Graphics::Pipeline::Get(pipelineDesc);

        commandBuffer->BindPipeline(pipeline);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
        commandBuffer->UnBindPipeline();
    }

    void RenderPasses::ShadowPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Shadow Pass");

        bool empty = true;
        for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; ++i)
        {
            if(!m_ShadowData.m_CascadeCommandQueue[i].empty())
                empty = false;
        }

        if(empty)
            return;

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();
        commandBuffer->EndCurrentRenderPass();

        m_ShadowData.m_DescriptorSet[0]->SetUniform("ShadowData", "DirLightMatrices", m_ShadowData.m_ShadowProjView);
        m_ShadowData.m_DescriptorSet[0]->Update();

        m_ShadowData.m_DescriptorSet[1]->SetUniform("ShadowData", "DirLightMatrices", m_ShadowData.m_ShadowProjView);
        m_ShadowData.m_DescriptorSet[1]->Update();

        Arena* frameArena                  = Application::Get().GetFrameArena();
        DescriptorSet** currentDescriptors = PushArrayNoZero(frameArena, DescriptorSet*, 4);

        for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; ++i)
        {
            LUMOS_PROFILE_GPU("Shadow Layer Pass");

            m_ShadowData.m_Layer = i;

            for(auto& command : m_ShadowData.m_CascadeCommandQueue[m_ShadowData.m_Layer])
            {
                Material* material    = command.material ? command.material : m_ForwardData.m_DefaultMaterial;
                currentDescriptors[1] = material->GetDescriptorSet();
                bool alphaBlend       = material->GetFlag(Material::RenderFlags::ALPHABLEND);

                uint32_t layer      = static_cast<uint32_t>(m_ShadowData.m_Layer);
                auto& pushConstants = command.pipeline->GetShader()->GetPushConstants();
                memcpy(pushConstants[0].data + sizeof(glm::mat4), &layer, sizeof(uint32_t));
                currentDescriptors[0] = alphaBlend ? m_ShadowData.m_DescriptorSet[1].get() : m_ShadowData.m_DescriptorSet[0].get();

                if(command.animated)
                {
                    if(alphaBlend)
                    {
                        currentDescriptors[2] = command.AnimatedDescriptorSet;
                    }
                    else
                    {
                        currentDescriptors[1] = command.AnimatedDescriptorSet;
                    }
                }

                auto pipeline = command.pipeline;
                commandBuffer->BindPipeline(pipeline, m_ShadowData.m_Layer);

                Mesh* mesh     = command.mesh;
                auto transform = m_ShadowData.m_ShadowProjView[m_ShadowData.m_Layer] * command.transform;
                memcpy(pushConstants[0].data, &transform, sizeof(glm::mat4));

                command.pipeline->GetShader()->BindPushConstants(commandBuffer, pipeline);
                Renderer::BindDescriptorSets(pipeline, commandBuffer, 0, currentDescriptors, command.animated ? (alphaBlend ? 3 : 2) : (alphaBlend ? 2 : 1));
                Renderer::DrawMesh(commandBuffer, pipeline, mesh);
                m_Stats.NumShadowObjects++;
            }
            commandBuffer->UnBindPipeline();
            commandBuffer->EndCurrentRenderPass();
        }
    }

    void RenderPasses::DepthPrePass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Depth Pre Pass");

        Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.polygonMode      = Graphics::PolygonMode::FILL;
        pipelineDesc.blendMode        = BlendMode::SrcAlphaOneMinusSrcAlpha;
        pipelineDesc.clearTargets     = false;
        pipelineDesc.swapchainTarget  = false;
        pipelineDesc.cullMode         = Graphics::CullMode::BACK;
        pipelineDesc.depthTarget      = m_ForwardData.m_DepthTexture;
        pipelineDesc.colourTargets[0] = m_NormalTexture;
        pipelineDesc.DebugName        = "Depth Prepass";
        pipelineDesc.samples          = m_MainTextureSamples;

        DescriptorSet* sets[3];
        sets[0] = m_ForwardData.m_DescriptorSet[0].get();

        for(auto& command : m_ForwardData.m_CommandQueue)
        {
            Material* material = command.material ? command.material : m_ForwardData.m_DefaultMaterial;
            sets[1]            = material->GetDescriptorSet();
            if(!material->GetFlag(Material::RenderFlags::DEPTHTEST)) // || command.material->GetFlag(Material::RenderFlags::ALPHABLEND))
                continue;

            bool alphaBlend = material->GetFlag(Material::RenderFlags::ALPHABLEND);

            if(command.animated)
            {
                if(alphaBlend)
                    sets[2] = command.AnimatedDescriptorSet;
                else
                    sets[1] = command.AnimatedDescriptorSet;
            }

            pipelineDesc.transparencyEnabled = alphaBlend;
            pipelineDesc.shader              = command.animated ? (alphaBlend ? m_DepthPrePassAlphaAnimShader : m_DepthPrePassAnimShader) : (alphaBlend ? m_DepthPrePassAlphaShader : m_DepthPrePassShader);

            auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
            commandBuffer->BindPipeline(pipeline);

            Mesh* mesh           = command.mesh;
            auto& worldTransform = command.transform;

            auto& pushConstants = m_DepthPrePassShader->GetPushConstants()[0];
            pushConstants.SetValue("transform", (void*)&worldTransform);

            m_DepthPrePassShader->BindPushConstants(commandBuffer, pipeline);
            Renderer::BindDescriptorSets(pipeline, commandBuffer, 0, sets, command.animated ? (alphaBlend ? 3 : 2) : (alphaBlend ? 2 : 1));
            Renderer::DrawMesh(commandBuffer, pipeline, mesh);
        }
    }

    void RenderPasses::SSAOPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("SSAO Pass");

        if(!m_Camera || !m_SSAOShader || !m_SSAOShader->IsCompiled())
            return;

        Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader           = m_SSAOShader;
        pipelineDesc.colourTargets[0] = m_SSAOTexture;
        pipelineDesc.clearTargets     = true;
        pipelineDesc.DebugName        = "SSAO";
        pipelineDesc.samples          = m_MainTextureSamples;
        if(m_MainTextureSamples > 1)
            pipelineDesc.resolveTexture = m_ResolveTexture;

        auto projection = m_Camera->GetProjectionMatrix();
        auto invProj    = glm::inverse(m_Camera->GetProjectionMatrix());
        auto view       = glm::inverse(m_CameraTransform->GetWorldMatrix());

        float nearC = m_Camera->GetNear();
        float farC  = m_Camera->GetFar();

        static glm::vec4 samples[64];
        static bool init = false;

        if(!init)
        {
            for(uint32_t i = 0; i < 64; ++i)
            {
                glm::vec3 sample(Random32::Rand(-0.9f, 0.9f), Random32::Rand(-0.9f, 0.9f), Random32::Rand(0.0f, 1.0f));
                sample = glm::normalize(sample);      // Snap to surface of hemisphere
                sample *= Random32::Rand(0.0f, 1.0f); // Space out linearly
                float scale = (float)i / (float)64;
                scale       = Maths::Lerp(0.1f, 1.0f, scale * scale); // Bring distribution of samples closer to origin
                samples[i]  = glm::vec4(sample * scale, 0.0f);
            }
            init = true;
        }

        Scene::SceneRenderSettings& renderSettings = m_CurrentScene->GetSettings().RenderSettings;

        m_SSAOPassDescriptorSet->SetUniform("UniformBuffer", "invProj", &invProj);
        m_SSAOPassDescriptorSet->SetUniform("UniformBuffer", "projection", &projection);
        m_SSAOPassDescriptorSet->SetUniform("UniformBuffer", "view", &view);

        m_SSAOPassDescriptorSet->SetUniform("UniformBuffer", "samples", &samples);
        m_SSAOPassDescriptorSet->SetUniform("UniformBuffer", "ssaoRadius", &renderSettings.SSAOSampleRadius);

        m_SSAOPassDescriptorSet->SetUniform("UniformBuffer", "near", &nearC);
        m_SSAOPassDescriptorSet->SetUniform("UniformBuffer", "far", &farC);
        m_SSAOPassDescriptorSet->SetUniform("UniformBuffer", "strength", &renderSettings.SSAOStrength);

        m_SSAOPassDescriptorSet->SetTexture("in_Depth", m_ForwardData.m_DepthTexture);
        m_SSAOPassDescriptorSet->SetTexture("in_Noise", m_NoiseTexture);
        m_SSAOPassDescriptorSet->SetTexture("in_Normal", m_NormalTexture);
        m_SSAOPassDescriptorSet->Update();

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        auto set = m_SSAOPassDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
    }

    void RenderPasses::SSAOBlurPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("SSAO Blur Pass");

        if(!m_Camera || !m_SSAOBlurShader || !m_SSAOBlurShader->IsCompiled())
            return;

        Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader           = m_SSAOBlurShader;
        pipelineDesc.colourTargets[0] = m_SSAOTexture1;
        pipelineDesc.clearTargets     = true;
        pipelineDesc.DebugName        = "SSAO Blur";
        pipelineDesc.samples          = m_MainTextureSamples;
        if(m_MainTextureSamples > 1)
            pipelineDesc.resolveTexture = m_ResolveTexture;

        glm::vec2 ssaoTexelOffset = glm::vec2(2.0f / m_SSAOTexture->GetWidth(), 0.0f);

        Scene::SceneRenderSettings& renderSettings = m_CurrentScene->GetSettings().RenderSettings;
        auto view                                  = glm::inverse(m_CameraTransform->GetWorldMatrix());

        m_SSAOBlurPassDescriptorSet->SetUniform("UniformBuffer", "view", &view);
        m_SSAOBlurPassDescriptorSet->SetUniform("UniformBuffer", "ssaoTexelOffset", &ssaoTexelOffset);
        m_SSAOBlurPassDescriptorSet->SetUniform("UniformBuffer", "ssaoBlurRadius", &renderSettings.SSAOBlurRadius);

        m_SSAOBlurPassDescriptorSet->SetTexture("in_Depth", m_ForwardData.m_DepthTexture);
        m_SSAOBlurPassDescriptorSet->SetTexture("in_SSAO", m_SSAOTexture);
        m_SSAOBlurPassDescriptorSet->SetTexture("in_Normal", m_NormalTexture);
        m_SSAOBlurPassDescriptorSet->TransitionImages(commandBuffer);
        m_SSAOBlurPassDescriptorSet->Update();

        ssaoTexelOffset = glm::vec2(0.0f, 2.0f / m_SSAOTexture->GetHeight());

        m_SSAOBlurPassDescriptorSet2->SetUniform("UniformBuffer", "view", &view);
        m_SSAOBlurPassDescriptorSet2->SetUniform("UniformBuffer", "ssaoTexelOffset", &ssaoTexelOffset);
        m_SSAOBlurPassDescriptorSet2->SetUniform("UniformBuffer", "ssaoBlurRadius", &renderSettings.SSAOBlurRadius);

        m_SSAOBlurPassDescriptorSet2->SetTexture("in_Depth", m_ForwardData.m_DepthTexture);
        m_SSAOBlurPassDescriptorSet2->SetTexture("in_SSAO", m_SSAOTexture1);
        m_SSAOBlurPassDescriptorSet2->SetTexture("in_Normal", m_NormalTexture);

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        auto set = m_SSAOBlurPassDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);

        m_SSAOBlurPassDescriptorSet2->TransitionImages(commandBuffer);
        m_SSAOBlurPassDescriptorSet2->Update();

        pipelineDesc.colourTargets[0] = m_SSAOTexture;
        pipeline                      = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        set = m_SSAOBlurPassDescriptorSet2.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
    }

    void RenderPasses::ForwardPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Forward Pass");

        if(m_ForwardData.m_CommandQueue.empty())
            return;

        Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        m_ForwardData.m_DescriptorSet[2]->TransitionImages(commandBuffer);

        Arena* frameArena                  = Application::Get().GetFrameArena();
        DescriptorSet** currentDescriptors = PushArrayNoZero(frameArena, DescriptorSet*, 4);

        for(auto& command : m_ForwardData.m_CommandQueue)
        {
            m_Stats.NumRenderedObjects++;

            Mesh* mesh           = command.mesh;
            auto& worldTransform = command.transform;
            Material* material   = command.material ? command.material : m_ForwardData.m_DefaultMaterial;
            auto pipeline        = command.pipeline;
            commandBuffer->BindPipeline(pipeline);

            currentDescriptors[0] = m_ForwardData.m_DescriptorSet[0].get();
            currentDescriptors[1] = material->GetDescriptorSet();
            currentDescriptors[2] = m_ForwardData.m_DescriptorSet[2].get();

            if(command.animated)
                currentDescriptors[3] = command.AnimatedDescriptorSet ? command.AnimatedDescriptorSet : m_ForwardData.m_DescriptorSet[3].get();

            auto& pushConstants = m_ForwardData.m_Shader->GetPushConstants()[0];
            pushConstants.SetData((void*)&worldTransform);

            m_ForwardData.m_Shader->BindPushConstants(commandBuffer, pipeline);
            Renderer::BindDescriptorSets(pipeline, commandBuffer, 0, currentDescriptors, command.animated ? 4 : 3);
            Renderer::DrawMesh(commandBuffer, pipeline, mesh);
        }
    }

    void RenderPasses::SkyboxPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("SkyBox Pass");

        if(!m_CubeMap)
            return;

        if(!m_Camera || !m_CameraTransform)
            return;

        int mode        = 0;
        float exposure  = m_Exposure * 120000.0f;
        float blurLevel = m_CurrentScene->GetSettings().RenderSettings.SkyboxMipLevel;
        m_SkyboxDescriptorSet->SetUniform("UniformBuffer", "Mode", &mode);
        m_SkyboxDescriptorSet->SetUniform("UniformBuffer", "Exposure", &exposure);
        m_SkyboxDescriptorSet->SetUniform("UniformBuffer", "BlurLevel", &blurLevel);
        m_SkyboxDescriptorSet->SetTexture("u_CubeMap", m_CubeMap, 0, TextureType::CUBE);
        m_SkyboxDescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader = m_SkyboxShader;

        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = false;

        {
            pipelineDesc.depthTarget = reinterpret_cast<Texture*>(m_ForwardData.m_DepthTexture);
        }

        pipelineDesc.colourTargets[0] = m_MainTexture; // m_MainTexture;
        pipelineDesc.DebugName        = "Skybox";
        pipelineDesc.samples          = m_MainTextureSamples;
        if(m_MainTextureSamples > 1)
            pipelineDesc.resolveTexture = m_ResolveTexture;
        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        auto pipeline      = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        auto set = m_SkyboxDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::DrawMesh(commandBuffer, pipeline.get(), m_ScreenQuad);
    }

    void RenderPasses::DepthOfFieldPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Depth Of Field Pass");

        if(m_MainTextureSamples > 1)
        {
            // LUMOS_LOG_WARN("Depth of field pass currently not working with msaa");
            return;
        }

        if(!m_Camera || !m_DepthOfFieldShader || !m_DepthOfFieldShader->IsCompiled())
            return;
        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        const auto& proj                           = m_Camera->GetProjectionMatrix();
        Scene::SceneRenderSettings& renderSettings = m_CurrentScene->GetSettings().RenderSettings;

        float depthLinearizeMul = (-proj[3][2]); // float depthLinearizeMul = ( clipFar * clipNear ) / ( clipFar - clipNear );
        float depthLinearizeAdd = (proj[2][2]);  // float depthLinearizeAdd = clipFar / ( clipFar - clipNear );
        // correct the handedness issue.
        if(depthLinearizeMul * depthLinearizeAdd < 0)
            depthLinearizeAdd = -depthLinearizeAdd;
        glm::vec2 DepthConsts = { depthLinearizeMul, depthLinearizeAdd };
        glm::vec2 DOFParams   = { renderSettings.DepthOfFieldDistance, renderSettings.DepthOfFieldStrength };

        m_DepthOfFieldPassDescriptorSet->SetUniform("UniformBuffer", "DOFParams", &DOFParams);
        m_DepthOfFieldPassDescriptorSet->SetUniform("UniformBuffer", "DepthConsts", &DepthConsts);

        m_DepthOfFieldPassDescriptorSet->SetTexture("u_Texture", m_LastRenderTarget);
        m_DepthOfFieldPassDescriptorSet->SetTexture("u_DepthTexture", m_ForwardData.m_DepthTexture);
        m_DepthOfFieldPassDescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_DepthOfFieldShader;
        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.colourTargets[0]    = m_PostProcessTexture1;
        pipelineDesc.DebugName           = "DepthofField";
        pipelineDesc.samples             = m_MainTextureSamples;
        auto pipeline                    = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        auto set = m_DepthOfFieldPassDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);

        std::swap(m_LastRenderTarget, m_PostProcessTexture1);
    }

    void RenderPasses::SharpenPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Sharpen Pass");

        if(!m_Camera || !m_SharpenShader || !m_SharpenShader->IsCompiled())
            return;

        Scene::SceneRenderSettings& renderSettings = m_CurrentScene->GetSettings().RenderSettings;

        m_SharpenPassDescriptorSet->SetTexture("u_Texture", m_LastRenderTarget);
        m_SharpenPassDescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_SharpenShader;
        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.colourTargets[0]    = m_PostProcessTexture1;
        pipelineDesc.DebugName           = "Sharpen";
        auto commandBuffer               = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        auto pipeline                    = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        auto set = m_SharpenPassDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);

        std::swap(m_PostProcessTexture1, m_LastRenderTarget);
    }

    void RenderPasses::ToneMappingPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Tone Mapping Pass");

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        float bloomIntensity = m_CurrentScene->GetSettings().RenderSettings.BloomEnabled ? m_CurrentScene->GetSettings().RenderSettings.m_BloomIntensity : 0.0f;
        float Saturation     = m_CurrentScene->GetSettings().RenderSettings.Saturation;
        float Brightness     = m_CurrentScene->GetSettings().RenderSettings.Brightness;
        float Contrast       = m_CurrentScene->GetSettings().RenderSettings.Contrast;

        m_ToneMappingPassDescriptorSet->SetUniform("UniformBuffer", "BloomIntensity", &bloomIntensity);
        m_ToneMappingPassDescriptorSet->SetUniform("UniformBuffer", "ToneMapIndex", &m_ToneMapIndex);
        m_ToneMappingPassDescriptorSet->SetUniform("UniformBuffer", "Brightness", &Brightness);
        m_ToneMappingPassDescriptorSet->SetUniform("UniformBuffer", "Contrast", &Contrast);
        m_ToneMappingPassDescriptorSet->SetUniform("UniformBuffer", "Saturation", &Saturation);

        m_ToneMappingPassDescriptorSet->SetTexture("u_Texture", m_LastRenderTarget);
        m_ToneMappingPassDescriptorSet->SetTexture("u_BloomTexture", m_BloomTextureLastRenderered);
        m_ToneMappingPassDescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_ToneMappingPassShader;
        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.colourTargets[0]    = m_PostProcessTexture1;
        pipelineDesc.DebugName           = "ToneMapping";
        auto pipeline                    = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        auto set = m_ToneMappingPassDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);

        std::swap(m_PostProcessTexture1, m_LastRenderTarget);
    }

    void RenderPasses::FinalPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Final Pass");

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        Texture* finalPassTexture = m_LastRenderTarget;

#ifndef LUMOS_DIST

        int debugMode = m_CurrentScene->GetSettings().RenderSettings.DebugMode;
        switch(debugMode)
        {
        case 1:
            finalPassTexture = m_SSAOTexture;
            break;
        case 2:
            finalPassTexture = m_SSAOTexture1;
            break;
        case 3:
            finalPassTexture = m_NormalTexture;
            break;
        case 4:
            finalPassTexture = m_BloomTexture;
            break;
        case 5:
            finalPassTexture = m_NoiseTexture;
            break;
        case 6:
            finalPassTexture = m_PostProcessTexture1;
            break;
        case 0:
        default:
            break;
        }
#endif
        m_FinalPassDescriptorSet->SetTexture("u_Texture", finalPassTexture);
        m_FinalPassDescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader = m_FinalPassShader;

        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.DebugName           = "Final Pass";
        if(m_ForwardData.m_RenderTexture)
            pipelineDesc.colourTargets[0] = m_ForwardData.m_RenderTexture;
        else
            pipelineDesc.swapchainTarget = true;

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        auto set = m_FinalPassDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);

        if(m_LastRenderTarget != m_MainTexture && m_LastRenderTarget != m_ResolveTexture)
            std::swap(m_PostProcessTexture1, m_LastRenderTarget);

        commandBuffer->UnBindPipeline();
        commandBuffer->EndCurrentRenderPass();
    }

    void RenderPasses::BloomPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Bloom Pass");

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        if(!m_BloomPassShader->IsCompiled() || !m_BloomDescriptorSets[0] || m_BloomTexture->GetWidth() == 0 || m_BloomTexture->GetHeight() == 0)
            return;

        int descCount      = (int)m_BloomDescriptorSets.size();
        int targetMipCount = m_BloomTexture->GetMipMapLevels();

        int targetDescCount = 1 + (targetMipCount - 2) * 2 + 1 + (targetMipCount - 2 - 3);

        if(descCount != targetDescCount)
        {
            m_BloomDescriptorSets.clear();

            Graphics::DescriptorDesc descriptorDesc {};
            descriptorDesc.layoutIndex = 0;
            descriptorDesc.shader      = m_BloomPassShader.get();

            for(int i = 0; i < targetDescCount; i++)
            {
                m_BloomDescriptorSets.push_back(SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc)));
            }
        }

        int passCount = 0;

        struct BloomComputePushConstants
        {
            glm::vec4 Params;
            glm::vec4 Params2; // float LOD = 0.0f;
            // int Mode = 0; // 0 = prefilter, 1 = downsample, 2 = firstUpsample, 3 = upsample
        } bloomComputePushConstants;

        bloomComputePushConstants.Params    = { m_CurrentScene->GetSettings().RenderSettings.BloomThreshold, m_CurrentScene->GetSettings().RenderSettings.BloomThreshold - m_CurrentScene->GetSettings().RenderSettings.BloomKnee, m_CurrentScene->GetSettings().RenderSettings.BloomKnee * 2.0f, 0.25f / m_CurrentScene->GetSettings().RenderSettings.BloomKnee };
        bloomComputePushConstants.Params2.x = (float)0;
        bloomComputePushConstants.Params2.y = (float)0;
        bloomComputePushConstants.Params2.z = (float)m_MainTexture->GetWidth();
        bloomComputePushConstants.Params2.w = (float)m_MainTexture->GetHeight();

        uint32_t workGroupSize = 4;
        uint32_t workGroupsX   = m_BloomTexture->GetWidth() / workGroupSize;
        uint32_t workGroupsY   = m_BloomTexture->GetHeight() / workGroupSize;

        commandBuffer->UnBindPipeline();

        m_BloomDescriptorSets[0]->SetTexture("u_Texture", m_LastRenderTarget);
        m_BloomDescriptorSets[0]->SetTexture("u_BloomTexture", m_LastRenderTarget);

        if(m_SupportCompute)
        {
            m_BloomDescriptorSets[0]->SetTexture("o_Image", m_BloomTexture);
            m_BloomDescriptorSets[0]->TransitionImages(commandBuffer);
        }
        m_BloomDescriptorSets[0]->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_BloomPassShader;
        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.mipIndex            = 0;
        pipelineDesc.DebugName           = "Bloom-Prefilter";
        if(!m_SupportCompute)
            pipelineDesc.colourTargets[0] = m_BloomTexture;

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        auto& pushConstants = m_BloomPassShader->GetPushConstants();
        memcpy(pushConstants[0].data, &bloomComputePushConstants, sizeof(BloomComputePushConstants));
        m_BloomPassShader->BindPushConstants(commandBuffer, pipeline.get());

        // Pre Filter
        {

            auto set = m_BloomDescriptorSets[passCount].get();
            Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);

            if(m_SupportCompute)
            {
                Renderer::GetRenderer()->Dispatch(commandBuffer, workGroupsX, workGroupsY, 1);
            }
            else
            {
                Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
            }

            passCount++;
        }

        commandBuffer->UnBindPipeline();

        bloomComputePushConstants.Params2.y = 1;
        uint32_t mips;

        if(m_BloomTexture->GetMipMapLevels() < 2)
            mips = 1;
        else
            mips = m_BloomTexture->GetMipMapLevels() - 2;

        for(int i = 1; i < (targetMipCount - 2) * 2; i++)
        {
            if(i % 2 == 0)
            {
                m_BloomDescriptorSets[i]->SetTexture("u_Texture", m_BloomTexture1);
                m_BloomDescriptorSets[i]->SetTexture("u_BloomTexture", m_LastRenderTarget);
            }
            else
            {
                m_BloomDescriptorSets[i]->SetTexture("u_Texture", m_BloomTexture);
                m_BloomDescriptorSets[i]->SetTexture("u_BloomTexture", m_LastRenderTarget);
            }

            m_BloomDescriptorSets[i]->Update();
        }

        //        m_BloomDescriptorSets[1]->SetTexture("u_Texture", m_BloomTexture);
        //        m_BloomDescriptorSets[1]->SetTexture("u_BloomTexture", m_MainTexture);
        //
        //        m_BloomDescriptorSets[2]->SetTexture("u_Texture", m_BloomTexture1);
        //        m_BloomDescriptorSets[2]->SetTexture("u_BloomTexture", m_MainTexture);

        // m_BloomDescriptorSets[2]->Update();

        for(uint32_t i = 1; i < mips; i++)
        {
            auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
            commandBuffer->UnBindPipeline();
            {
                pipelineDesc.mipIndex  = i;
                pipelineDesc.DebugName = "Bloom-Downsample"; // fmt::format("Bloom-Downsample{0}", i);
                if(!m_SupportCompute)
                    pipelineDesc.colourTargets[0] = m_BloomTexture1;
                bloomComputePushConstants.Params2.z = (float)m_BloomTexture1->GetWidth(i);
                bloomComputePushConstants.Params2.w = (float)m_BloomTexture1->GetHeight(i);

                if(m_SupportCompute)
                {
                    m_BloomDescriptorSets[passCount]->SetTexture("o_Image", m_BloomTexture1, i);
                }

                m_BloomDescriptorSets[passCount]->TransitionImages(commandBuffer);
                m_BloomDescriptorSets[passCount]->Update();

                pipeline = Graphics::Pipeline::Get(pipelineDesc);
                commandBuffer->BindPipeline(pipeline);

                bloomComputePushConstants.Params2.x = i - 1.0f;

                auto& pushConstants = m_BloomPassShader->GetPushConstants();
                memcpy(pushConstants[0].data, &bloomComputePushConstants, sizeof(BloomComputePushConstants));
                m_BloomPassShader->BindPushConstants(commandBuffer, pipeline.get());

                auto set = m_BloomDescriptorSets[passCount].get();
                Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
                if(m_SupportCompute)
                {
                    workGroupsX = (uint32_t)glm::ceil((float)m_BloomTexture->GetWidth(i) / (float)workGroupSize);
                    workGroupsY = (uint32_t)glm::ceil((float)m_BloomTexture->GetHeight(i) / (float)workGroupSize);

                    Renderer::GetRenderer()->Dispatch(commandBuffer, workGroupsX, workGroupsY, 1);
                }
                else
                {
                    Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
                }

                passCount++;
            }
            {
                auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
                commandBuffer->UnBindPipeline();

                if(m_SupportCompute)
                {
                    m_BloomDescriptorSets[passCount]->SetTexture("o_Image", m_BloomTexture, i);
                    m_BloomDescriptorSets[passCount]->TransitionImages(commandBuffer);
                    m_BloomDescriptorSets[passCount]->Update();
                }
                else
                {
                    // m_BloomDescriptorSets[passCount]->SetTexture("u_Texture", m_BloomTexture, i);
                    m_BloomDescriptorSets[passCount]->TransitionImages(commandBuffer);
                    m_BloomDescriptorSets[passCount]->Update();
                }

                pipelineDesc.mipIndex = i;

                if(!m_SupportCompute)
                    pipelineDesc.colourTargets[0] = m_BloomTexture;
                bloomComputePushConstants.Params2.z = (float)m_BloomTexture1->GetWidth(i);
                bloomComputePushConstants.Params2.w = (float)m_BloomTexture1->GetHeight(i);

                pipeline = Graphics::Pipeline::Get(pipelineDesc);
                commandBuffer->BindPipeline(pipeline);

                bloomComputePushConstants.Params2.x = (float)i;

                memcpy(pushConstants[0].data, &bloomComputePushConstants, sizeof(BloomComputePushConstants));
                m_BloomPassShader->BindPushConstants(commandBuffer, pipeline.get());

                auto set = m_BloomDescriptorSets[passCount].get();
                Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
                if(m_SupportCompute)
                {
                    workGroupsX = (uint32_t)glm::ceil((float)m_BloomTexture->GetWidth(i) / (float)workGroupSize);
                    workGroupsY = (uint32_t)glm::ceil((float)m_BloomTexture->GetHeight(i) / (float)workGroupSize);

                    Renderer::GetRenderer()->Dispatch(commandBuffer, workGroupsX, workGroupsY, 1);
                }
                else
                {
                    Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
                }

                passCount++;
            }
        }

        // First Upsample
        {
            auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
            commandBuffer->UnBindPipeline();

            m_BloomDescriptorSets[passCount]->SetTexture("u_Texture", m_BloomTexture);
            m_BloomDescriptorSets[passCount]->SetTexture("u_BloomTexture", m_LastRenderTarget);

            if(m_SupportCompute)
            {
                m_BloomDescriptorSets[passCount]->SetTexture("o_Image", m_BloomTexture2, mips - 2);
            }
            m_BloomDescriptorSets[passCount]->TransitionImages(commandBuffer);
            m_BloomDescriptorSets[passCount]->Update();

            bloomComputePushConstants.Params2.y = (float)2;
            bloomComputePushConstants.Params2.x -= 1.0f;
            bloomComputePushConstants.Params2.z = (float)m_BloomTexture2->GetWidth(mips - 2);
            bloomComputePushConstants.Params2.w = (float)m_BloomTexture2->GetHeight(mips - 2);

            pipelineDesc.mipIndex  = mips - 2;
            pipelineDesc.DebugName = "Bloom-Upsample"; // fmt::format("Bloom-Upsample{0}", mips - 2);

            if(!m_SupportCompute)
                pipelineDesc.colourTargets[0] = m_BloomTexture2;
            pipeline = Graphics::Pipeline::Get(pipelineDesc);
            commandBuffer->BindPipeline(pipeline);

            memcpy(pushConstants[0].data, &bloomComputePushConstants, sizeof(BloomComputePushConstants));
            m_BloomPassShader->BindPushConstants(commandBuffer, pipeline.get());

            auto set2 = m_BloomDescriptorSets[passCount].get();
            Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set2, 1);
            if(m_SupportCompute)
            {
                workGroupsX = (uint32_t)glm::ceil((float)m_BloomTexture->GetWidth(mips - 2) / (float)workGroupSize);
                workGroupsY = (uint32_t)glm::ceil((float)m_BloomTexture->GetHeight(mips - 2) / (float)workGroupSize);

                Renderer::GetRenderer()->Dispatch(commandBuffer, workGroupsX, workGroupsY, 1);
            }
            else
            {
                Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
            }

            passCount++;
        }

        int test = 1;
        for(int i = passCount; i < passCount + (targetMipCount - 2 - 3) + 1; i++)
        {
            auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
            commandBuffer->UnBindPipeline();

            if(test % 2 == 0)
            {
                m_BloomDescriptorSets[i]->SetTexture("u_Texture", m_BloomTexture);
                m_BloomDescriptorSets[i]->SetTexture("u_BloomTexture", m_BloomTexture1);
            }
            else
            {
                m_BloomDescriptorSets[i]->SetTexture("u_Texture", m_BloomTexture);
                m_BloomDescriptorSets[i]->SetTexture("u_BloomTexture", m_BloomTexture2);
            }
            m_BloomDescriptorSets[i]->Update();

            test++;
        }

        // Can't transition images to right layout if render to a colour attachment
        // Need to sample from m_BloomTexture2 first then switch between that and m_BloomTexture1
        bool evenMip = true;

        // Upsample
        for(int32_t mip = mips - 3; mip >= 0; mip--)
        {
            auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
            commandBuffer->UnBindPipeline();

            bloomComputePushConstants.Params2.y = (float)3;
            bloomComputePushConstants.Params2.x = (float)mip;
            bloomComputePushConstants.Params2.z = (float)m_BloomTexture2->GetWidth(mip);
            bloomComputePushConstants.Params2.w = (float)m_BloomTexture2->GetHeight(mip);

            pipelineDesc.mipIndex  = mip;
            pipelineDesc.DebugName = "Bloom-Upsample"; // fmt::format("Bloom-Upsample{0}", mip);

            if(evenMip)
            {
                if(m_SupportCompute)
                {
                    m_BloomDescriptorSets[passCount]->SetTexture("o_Image", m_BloomTexture1, mip);
                    m_BloomDescriptorSets[passCount]->TransitionImages(commandBuffer);
                    m_BloomDescriptorSets[passCount]->Update();
                }
                else
                    m_BloomDescriptorSets[passCount]->TransitionImages(commandBuffer);
            }
            else
            {
                if(m_SupportCompute)
                {
                    m_BloomDescriptorSets[passCount]->SetTexture("o_Image", m_BloomTexture2, mip);
                    m_BloomDescriptorSets[passCount]->TransitionImages(commandBuffer);
                    m_BloomDescriptorSets[passCount]->Update();
                }
                else
                    m_BloomDescriptorSets[passCount]->TransitionImages(commandBuffer);
            }

            if(!m_SupportCompute)
                pipelineDesc.colourTargets[0] = evenMip ? m_BloomTexture1 : m_BloomTexture2;
            auto set2 = m_BloomDescriptorSets[passCount].get(); // evenMip ? m_BloomDescriptorSets[passCount].get() : m_BloomDescriptorSets[5].get();
            set2->TransitionImages(commandBuffer);

            pipeline = Graphics::Pipeline::Get(pipelineDesc);
            commandBuffer->BindPipeline(pipeline);

            memcpy(pushConstants[0].data, &bloomComputePushConstants, sizeof(BloomComputePushConstants));
            m_BloomPassShader->BindPushConstants(commandBuffer, pipeline.get());

            Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set2, 1);
            if(m_SupportCompute)
            {
                workGroupsX = (uint32_t)glm::ceil((float)m_BloomTexture2->GetWidth(mip) / (float)workGroupSize);
                workGroupsY = (uint32_t)glm::ceil((float)m_BloomTexture2->GetHeight(mip) / (float)workGroupSize);

                Renderer::GetRenderer()->Dispatch(commandBuffer, workGroupsX, workGroupsY, 1);
            }
            else
            {
                Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
            }

            passCount++;

            m_BloomTextureLastRenderered = evenMip ? m_BloomTexture1 : m_BloomTexture2;
            evenMip                      = !evenMip;
        }
    }

    void RenderPasses::FXAAPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("FXAA Pass");

        if(!m_MainTexture || !m_FXAAShader->IsCompiled())
            return;

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        m_FXAAPassDescriptorSet->SetTexture("u_Texture", m_LastRenderTarget);

        if(m_SupportCompute)
        {
            m_FXAAPassDescriptorSet->SetTexture("o_Image", m_PostProcessTexture1);
            m_FXAAPassDescriptorSet->TransitionImages(commandBuffer);
        }

        m_FXAAPassDescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_FXAAShader;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.clearTargets        = false;
        pipelineDesc.DebugName           = "FXAA";

        if(!m_SupportCompute)
            pipelineDesc.colourTargets[0] = m_PostProcessTexture1;

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        auto set = m_FXAAPassDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);

        if(m_SupportCompute)
        {
            uint32_t workGroupSize = 4;
            uint32_t workGroupsX   = m_PostProcessTexture1->GetWidth() / workGroupSize;
            uint32_t workGroupsY   = m_PostProcessTexture1->GetHeight() / workGroupSize;

            Renderer::GetRenderer()->Dispatch(commandBuffer, workGroupsX, workGroupsY, 1);
        }
        else
        {
            Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
        }

        std::swap(m_PostProcessTexture1, m_LastRenderTarget);
    }

    void RenderPasses::DebandingPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Debanding Pass");

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        if(!m_LastRenderTarget || !m_DebandingShader->IsCompiled())
            return;

        m_DebandingPassDescriptorSet->SetTexture("u_Texture", m_LastRenderTarget);
        m_DebandingPassDescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_DebandingShader;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.clearTargets        = true;
        pipelineDesc.colourTargets[0]    = m_PostProcessTexture1;
        pipelineDesc.DebugName           = "Debanding";

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        auto set = m_DebandingPassDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);

        std::swap(m_PostProcessTexture1, m_LastRenderTarget);
    }

    void RenderPasses::FilmicGrainPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Filmic Grain Pass");

        if(!m_MainTexture || !m_FilmicGrainShader->IsCompiled())
            return;

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        m_FilmicGrainPassDescriptorSet->SetTexture("u_Texture", m_LastRenderTarget);
        m_FilmicGrainPassDescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_FilmicGrainShader;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.clearTargets        = true;
        pipelineDesc.colourTargets[0]    = m_PostProcessTexture1;
        pipelineDesc.DebugName           = "FilmicGrain";

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        auto& pushConstants = m_FilmicGrainShader->GetPushConstants();
        if(!pushConstants.empty())
        {
            auto& pushConstant = m_FilmicGrainShader->GetPushConstants()[0];
            float time         = (float)Engine::GetTimeStep().GetElapsedSeconds();
            float intensity    = 0.02f;
            pushConstant.SetValue("Time", (void*)&time);
            pushConstant.SetValue("Intensity", (void*)&intensity);
            m_FilmicGrainShader->BindPushConstants(commandBuffer, pipeline.get());
        }

        auto set = m_FilmicGrainPassDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);

        std::swap(m_PostProcessTexture1, m_LastRenderTarget);
    }

    void RenderPasses::OutlinePass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Outline Pass");

        if(!m_LastRenderTarget || !m_OutlineShader || !m_OutlineShader->IsCompiled())
            return;

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        // m_OutlinePassDescriptorSet->SetTexture("u_Texture", m_LastRenderTarget);
        // m_OutlinePassDescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_OutlineShader;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.clearTargets        = true;
        pipelineDesc.colourTargets[0]    = m_PostProcessTexture1;
        pipelineDesc.DebugName           = "Outline";

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        // auto set = m_OutlinePassDescriptorSet.get();
        // Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);

        std::swap(m_PostProcessTexture1, m_LastRenderTarget);
    }

    void RenderPasses::ChromaticAberationPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("ChromaticAberation Pass");

        if(!m_Camera || !m_LastRenderTarget || !m_ChromaticAberationShader->IsCompiled())
            return;

        auto set             = m_ChromaticAberationPassDescriptorSet.get();
        float cameraAperture = m_Camera->GetAperture();
        float intensity      = 100.0f;

        set->SetTexture("u_Texture", m_LastRenderTarget);
        set->SetUniform("UniformBuffer", "chromaticAberrationIntensity", &intensity);
        set->SetUniform("UniformBuffer", "cameraAperture", &cameraAperture);
        set->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_ChromaticAberationShader;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.clearTargets        = true;
        pipelineDesc.colourTargets[0]    = m_PostProcessTexture1;
        pipelineDesc.DebugName           = "ChromaticAberation";

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        auto pipeline      = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);

        std::swap(m_PostProcessTexture1, m_LastRenderTarget);
    }

    void RenderPasses::EyeAdaptationPass()
    {
    }

    float RenderPasses::SubmitTexture(Texture* texture)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        float result = 0.0f;
        bool found   = false;

        for(uint32_t i = 0; i < m_Renderer2DData.m_TextureCount; i++)
        {
            if(m_Renderer2DData.m_Textures[i] == texture)
            {
                result = static_cast<float>(i + 1);
                found  = true;
                break;
            }
        }

        if(!found)
        {
            if(m_Renderer2DData.m_TextureCount >= m_Renderer2DData.m_Limits.MaxTextures)
            {
                Render2DFlush();
                Renderer2DBeginBatch();
            }
            m_Renderer2DData.m_Textures[m_Renderer2DData.m_TextureCount] = texture;
            m_Renderer2DData.m_TextureCount++;
            result = static_cast<float>(m_Renderer2DData.m_TextureCount);
        }
        return result;
    }

    void RenderPasses::Render2DPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Render2D Pass");

        if(m_Renderer2DData.m_CommandQueue2D.empty())
            return;

        Graphics::PipelineDesc pipelineDesc;
        pipelineDesc.shader              = m_Renderer2DData.m_Shader;
        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = true;
        pipelineDesc.blendMode           = BlendMode::SrcAlphaOneMinusSrcAlpha;
        pipelineDesc.clearTargets        = false;
        pipelineDesc.depthTarget         = reinterpret_cast<Texture*>(m_ForwardData.m_DepthTexture);
        pipelineDesc.colourTargets[0]    = m_MainTexture;
        pipelineDesc.DebugName           = "2D";
        pipelineDesc.samples             = m_MainTextureSamples;
        if(m_MainTextureSamples > 1)
            pipelineDesc.resolveTexture = m_ResolveTexture;

        m_Renderer2DData.m_Pipeline = Graphics::Pipeline::Get(pipelineDesc);

        Renderer2DBeginBatch();

        auto projView = m_Camera->GetProjectionMatrix() * glm::inverse(m_CameraTransform->GetWorldMatrix());
        m_Renderer2DData.m_DescriptorSet[0][0]->SetUniform("UBO", "projView", &projView);
        m_Renderer2DData.m_DescriptorSet[0][0]->Update();

        for(auto& command : m_Renderer2DData.m_CommandQueue2D)
        {
            m_Stats.NumRenderedObjects++;

            if(m_Renderer2DData.m_IndexCount >= m_Renderer2DData.m_Limits.IndiciesSize)
            {
                Render2DFlush();
                Renderer2DBeginBatch();
            }

            auto& renderable = command.renderable;
            auto& transform  = command.transform;

            const glm::vec2 min = renderable->GetPosition();
            const glm::vec2 max = renderable->GetPosition() + renderable->GetScale();

            const glm::vec4 colour = renderable->GetColour();
            const auto& uv         = renderable->GetUVs();
            Texture* texture       = renderable->GetTexture();

            float textureSlot = 0.0f;
            if(texture)
                textureSlot = SubmitTexture(texture);

            glm::vec3 vertex                  = transform * glm::vec4(min.x, min.y, 0.0f, 1.0f);
            m_Renderer2DData.m_Buffer->vertex = vertex;
            m_Renderer2DData.m_Buffer->uv.x   = uv[0].x;
            m_Renderer2DData.m_Buffer->uv.y   = uv[0].y;
            m_Renderer2DData.m_Buffer->tid    = glm::vec2(textureSlot, 0.0f);
            m_Renderer2DData.m_Buffer->colour = colour;
            m_Renderer2DData.m_Buffer++;

            vertex                            = transform * glm::vec4(max.x, min.y, 0.0f, 1.0f);
            m_Renderer2DData.m_Buffer->vertex = vertex;
            m_Renderer2DData.m_Buffer->uv.x   = uv[1].x;
            m_Renderer2DData.m_Buffer->uv.y   = uv[1].y;
            m_Renderer2DData.m_Buffer->tid    = glm::vec2(textureSlot, 0.0f);
            m_Renderer2DData.m_Buffer->colour = colour;
            m_Renderer2DData.m_Buffer++;

            vertex                            = transform * glm::vec4(max.x, max.y, 0.0f, 1.0f);
            m_Renderer2DData.m_Buffer->vertex = vertex;
            m_Renderer2DData.m_Buffer->uv.x   = uv[2].x;
            m_Renderer2DData.m_Buffer->uv.y   = uv[2].y;
            m_Renderer2DData.m_Buffer->tid    = glm::vec2(textureSlot, 0.0f);
            m_Renderer2DData.m_Buffer->colour = colour;
            m_Renderer2DData.m_Buffer++;

            vertex                            = transform * glm::vec4(min.x, max.y, 0.0f, 1.0f);
            m_Renderer2DData.m_Buffer->vertex = vertex;
            m_Renderer2DData.m_Buffer->uv.x   = uv[3].x;
            m_Renderer2DData.m_Buffer->uv.y   = uv[3].y;
            m_Renderer2DData.m_Buffer->tid    = glm::vec2(textureSlot, 0.0f);
            m_Renderer2DData.m_Buffer->colour = colour;
            m_Renderer2DData.m_Buffer++;

            m_Renderer2DData.m_IndexCount += 6;
        }

        if(m_Renderer2DData.m_IndexCount == 0)
        {
            return;
        }

        Render2DFlush();
    }

    void RenderPasses::Renderer2DBeginBatch()
    {
        uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

        m_Renderer2DData.m_IndexCount   = 0;
        m_Renderer2DData.m_TextureCount = 0;

        if((int)m_Renderer2DData.m_VertexBuffers[currentFrame].size() - 1 < (int)m_Renderer2DData.m_BatchDrawCallIndex)
        {
            auto& vertexBuffer = m_Renderer2DData.m_VertexBuffers[currentFrame].emplace_back(Graphics::VertexBuffer::Create(BufferUsage::DYNAMIC));
            vertexBuffer->Resize(m_Renderer2DData.m_Limits.BufferSize);

            m_2DBufferBase[currentFrame].emplace_back(new VertexData[m_Renderer2DData.m_Limits.MaxQuads * 4]);
        }

        m_Renderer2DData.m_VertexBuffers[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), m_Renderer2DData.m_Pipeline.get());
        m_Renderer2DData.m_Buffer = m_2DBufferBase[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex];
    }

    void RenderPasses::Render2DFlush()
    {
        LUMOS_PROFILE_FUNCTION();
        uint32_t currentFrame                  = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();
        Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        uint32_t dataSize = (uint32_t)((uint8_t*)m_Renderer2DData.m_Buffer - (uint8_t*)m_2DBufferBase[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex]);
        m_Renderer2DData.m_VertexBuffers[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex]->SetData(dataSize, (void*)m_2DBufferBase[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex], true);
        commandBuffer->BindPipeline(m_Renderer2DData.m_Pipeline);

        if(m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][1] == nullptr || m_Renderer2DData.m_TextureCount != m_Renderer2DData.m_PreviousFrameTextureCount[m_Renderer2DData.m_BatchDrawCallIndex])
        {
            /*
             || m_Renderer2DData.m_TextureCount != m_Renderer2DData.m_PreviousFrameTextureCount[m_Renderer2DData.m_BatchDrawCallIndex])
             When previous frame texture count was less then than the previous frame
             and the texture previously used was deleted, there was a crash - maybe moltenvk only
             May not be needed anymore
            */
            Graphics::DescriptorDesc descriptorDesc {};
            descriptorDesc.layoutIndex                                                 = 1;
            descriptorDesc.shader                                                      = m_Renderer2DData.m_Shader.get();
            m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][1] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        }

        for(int i = m_Renderer2DData.m_TextureCount; i < 16; i++)
        {
            m_Renderer2DData.m_Textures[i] = Material::GetDefaultTexture();
        }

        m_Renderer2DData.m_TextureCount = 16;

        if(m_Renderer2DData.m_TextureCount > 1)
            m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][1]->SetTexture("textures", m_Renderer2DData.m_Textures, m_Renderer2DData.m_TextureCount);
        else if(m_Renderer2DData.m_TextureCount == 0)
            m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][1]->SetTexture("textures", nullptr);
        else
            m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][1]->SetTexture("textures", m_Renderer2DData.m_Textures[0]);

        m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][1]->Update();

        m_Renderer2DData.m_PreviousFrameTextureCount[m_Renderer2DData.m_BatchDrawCallIndex] = m_Renderer2DData.m_TextureCount;

        Arena* frameArena                  = Application::Get().GetFrameArena();
        DescriptorSet** currentDescriptors = PushArrayNoZero(frameArena, DescriptorSet*, 2);

        currentDescriptors[0] = m_Renderer2DData.m_DescriptorSet[0][0].get();
        currentDescriptors[1] = m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][1].get();

        m_Renderer2DData.m_IndexBuffer->SetCount(m_Renderer2DData.m_IndexCount);
        m_Renderer2DData.m_IndexBuffer->Bind(commandBuffer);

        Renderer::BindDescriptorSets(m_Renderer2DData.m_Pipeline.get(), commandBuffer, 0, currentDescriptors, 2);
        Renderer::DrawIndexed(commandBuffer, DrawType::TRIANGLE, m_Renderer2DData.m_IndexCount);

        m_Renderer2DData.m_VertexBuffers[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex]->Unbind();
        m_Renderer2DData.m_IndexBuffer->Unbind();

        m_Renderer2DData.m_BatchDrawCallIndex++;
        m_Renderer2DData.m_TextureCount = 0;
    }

    void RenderPasses::TextFlush(Renderer2DData& textRenderData, std::vector<TextVertexData*>& textVertexBufferBase, TextVertexData*& textVertexBufferPtr)
    {
        LUMOS_PROFILE_FUNCTION();
        uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

        if(textRenderData.m_DescriptorSet[textRenderData.m_BatchDrawCallIndex][1] == nullptr)
        {
            /*
             || m_Renderer2DData.m_TextureCount != m_Renderer2DData.m_PreviousFrameTextureCount[m_Renderer2DData.m_BatchDrawCallIndex])
             When previous frame texture count was less then than the previous frame
             and the texture previously used was deleted, there was a crash - maybe moltenvk only
             May not be needed anymore
            */
            Graphics::DescriptorDesc descriptorDesc {};
            descriptorDesc.layoutIndex                                             = 1;
            descriptorDesc.shader                                                  = textRenderData.m_Shader.get();
            textRenderData.m_DescriptorSet[textRenderData.m_BatchDrawCallIndex][1] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        }

        for(int i = textRenderData.m_TextureCount; i < 16; i++)
        {
            textRenderData.m_Textures[i] = Material::GetDefaultTexture();
        }

        textRenderData.m_TextureCount = 16;

        if(textRenderData.m_TextureCount > 1)
            textRenderData.m_DescriptorSet[textRenderData.m_BatchDrawCallIndex][1]->SetTexture("textures", textRenderData.m_Textures, textRenderData.m_TextureCount);
        else if(textRenderData.m_TextureCount == 0)
            textRenderData.m_DescriptorSet[textRenderData.m_BatchDrawCallIndex][1]->SetTexture("textures", Material::GetDefaultTexture());
        else
            textRenderData.m_DescriptorSet[textRenderData.m_BatchDrawCallIndex][1]->SetTexture("textures", textRenderData.m_Textures[0]);

        textRenderData.m_DescriptorSet[textRenderData.m_BatchDrawCallIndex][1]->Update();

        textRenderData.m_PreviousFrameTextureCount[textRenderData.m_BatchDrawCallIndex] = textRenderData.m_TextureCount;

        Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();

        if((int)textRenderData.m_VertexBuffers[currentFrame].size() - 1 < (int)textRenderData.m_BatchDrawCallIndex)
        {
            auto& vertexBuffer = textRenderData.m_VertexBuffers[currentFrame].emplace_back(Graphics::VertexBuffer::Create(RENDERER_LINE_BUFFER_SIZE, nullptr, BufferUsage::DYNAMIC));
            // vertexBuffer->Resize(RENDERER_LINE_BUFFER_SIZE);
        }

        commandBuffer->UnBindPipeline();

        uint32_t dataSize = (uint32_t)((uint8_t*)textVertexBufferPtr - (uint8_t*)textVertexBufferBase[currentFrame]);
        textRenderData.m_VertexBuffers[currentFrame][textRenderData.m_BatchDrawCallIndex]->SetData(dataSize, (void*)textVertexBufferBase[currentFrame], true);

        commandBuffer->BindPipeline(textRenderData.m_Pipeline);

        Arena* frameArena                  = Application::Get().GetFrameArena();
        DescriptorSet** currentDescriptors = PushArrayNoZero(frameArena, DescriptorSet*, 2);

        currentDescriptors[0] = textRenderData.m_DescriptorSet[textRenderData.m_BatchDrawCallIndex][0].get();
        currentDescriptors[1] = textRenderData.m_DescriptorSet[textRenderData.m_BatchDrawCallIndex][1].get();

        textRenderData.m_VertexBuffers[currentFrame][textRenderData.m_BatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), textRenderData.m_Pipeline.get());

        textRenderData.m_IndexBuffer->SetCount(textRenderData.m_IndexCount);
        textRenderData.m_IndexBuffer->Bind(commandBuffer);

        Renderer::BindDescriptorSets(textRenderData.m_Pipeline.get(), commandBuffer, 0, currentDescriptors, 2);
        Renderer::DrawIndexed(commandBuffer, DrawType::TRIANGLE, textRenderData.m_IndexCount);

        textRenderData.m_VertexBuffers[currentFrame][textRenderData.m_BatchDrawCallIndex]->Unbind();
        textRenderData.m_IndexBuffer->Unbind();

        textVertexBufferPtr         = textVertexBufferBase[currentFrame];
        textRenderData.m_IndexCount = 0;
        textRenderData.m_BatchDrawCallIndex++;

        textRenderData.m_TextureCount = 0;
    }

    void RenderPasses::TextPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Text Pass");

        if(!m_Camera)
            return;

        auto textGroup = m_CurrentScene->GetRegistry().group<TextComponent>(entt::get<Maths::Transform>);
        if(textGroup.size() == 0)
            return;

        Graphics::PipelineDesc pipelineDesc;
        pipelineDesc.shader              = m_TextRendererData.m_Shader;
        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = true;
        pipelineDesc.blendMode           = BlendMode::SrcAlphaOneMinusSrcAlpha;
        pipelineDesc.clearTargets        = false;
        pipelineDesc.colourTargets[0]    = m_MainTexture;
        pipelineDesc.DebugName           = "Text";
        pipelineDesc.samples             = m_MainTextureSamples;

        if(m_MainTextureSamples > 1)
            pipelineDesc.resolveTexture = m_ResolveTexture;

        m_TextRendererData.m_Pipeline = Graphics::Pipeline::Get(pipelineDesc);

        uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

        if((int)m_TextRendererData.m_VertexBuffers[currentFrame].size() - 1 < (int)m_TextRendererData.m_BatchDrawCallIndex)
        {
            auto& vertexBuffer = m_TextRendererData.m_VertexBuffers[currentFrame].emplace_back(Graphics::VertexBuffer::Create(RENDERER_LINE_BUFFER_SIZE, nullptr, BufferUsage::DYNAMIC));
            // vertexBuffer->Resize(RENDERER_LINE_BUFFER_SIZE);
        }

        m_TextRendererData.m_VertexBuffers[currentFrame][m_TextRendererData.m_BatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), m_TextRendererData.m_Pipeline.get());
        TextVertexBufferPtr = TextVertexBufferBase[currentFrame];
        // m_TextBuffer = m_TextRendererData.m_VertexBuffers[currentFrame][m_TextRendererData.m_BatchDrawCallIndex]->GetPointer<TextVertexData>();

        auto projView = m_Camera->GetProjectionMatrix() * glm::inverse(m_CameraTransform->GetWorldMatrix());
        m_TextRendererData.m_DescriptorSet[0][0]->SetUniform("UBO", "projView", &projView);
        m_TextRendererData.m_DescriptorSet[0][0]->Update();

        m_TextRendererData.m_TextureCount = 0;
        for(auto entity : textGroup)
        {
            const auto& [textComp, trans] = textGroup.get<TextComponent, Maths::Transform>(entity);

            glm::mat4 transform = trans.GetWorldMatrix();
            m_Stats.NumRenderedObjects++;

            if(m_TextRendererData.m_IndexCount >= m_TextRendererData.m_Limits.IndiciesSize)
                TextFlush(m_TextRendererData, TextVertexBufferBase, TextVertexBufferPtr);

            int textureIndex       = -1;
            auto& string           = textComp.TextString;
            auto font              = textComp.FontHandle ? textComp.FontHandle : Font::GetDefaultFont();
            float lineHeightOffset = 0.0f;
            float kerningOffset    = 0.0f;

            float maxWidth     = textComp.MaxWidth;
            auto colour        = textComp.Colour;
            float lineSpacing  = textComp.LineSpacing;
            float kerning      = textComp.Kerning;
            auto outlineColour = textComp.OutlineColour;
            auto outlineWidth  = textComp.OutlineWidth;

            SharedPtr<Texture2D> fontAtlas = font->GetFontAtlas();
            if(!fontAtlas)
                continue;

            for(uint32_t i = 0; i < m_TextRendererData.m_TextureCount; i++)
            {
                if(m_TextRendererData.m_Textures[i] == fontAtlas.get())
                {
                    textureIndex = int(i + 1);
                    break;
                }
            }

            if(textureIndex == -1)
            {
                textureIndex                                                     = (int)m_TextRendererData.m_TextureCount + 1;
                m_TextRendererData.m_Textures[m_TextRendererData.m_TextureCount] = fontAtlas.get();
                m_TextRendererData.m_TextureCount++;
            }

            auto& fontGeometry  = font->GetMSDFData()->FontGeometry;
            const auto& metrics = fontGeometry.getMetrics();

            {
                double x       = 0.0;
                double fsScale = 1 / (metrics.ascenderY - metrics.descenderY);
                double y       = 0.0;
                for(int i = 0; i < string.size(); i++)
                {
                    char32_t character = string[i];

                    if(character == '\r')
                        continue;

                    if(character == '\n')
                    {
                        x = 0;
                        y -= fsScale * metrics.lineHeight + lineHeightOffset;
                        continue;
                    }

                    if(character == '\t')
                    {
                        auto glyph     = fontGeometry.getGlyph('a');
                        double advance = glyph->getAdvance();
                        x += 4 * fsScale * advance + kerningOffset;
                        continue;
                    }

                    auto glyph = fontGeometry.getGlyph(character);
                    if(!glyph)
                        glyph = fontGeometry.getGlyph('?');
                    if(!glyph)
                        continue;

                    double l, b, r, t;
                    glyph->getQuadAtlasBounds(l, b, r, t);

                    double pl, pb, pr, pt;
                    glyph->getQuadPlaneBounds(pl, pb, pr, pt);

                    pl *= fsScale, pb *= fsScale, pr *= fsScale, pt *= fsScale;
                    pl += x, pb += y, pr += x, pt += y;

                    double texelWidth  = 1. / fontAtlas->GetWidth();
                    double texelHeight = 1. / fontAtlas->GetHeight();
                    l *= texelWidth, b *= texelHeight, r *= texelWidth, t *= texelHeight;

                    {
                        LUMOS_PROFILE_SCOPE("Set text buffer data");
                        TextVertexBufferPtr->vertex        = transform * glm::vec4(pl, pb, 0.0f, 1.0f);
                        TextVertexBufferPtr->colour        = colour;
                        TextVertexBufferPtr->uv            = { l, b };
                        TextVertexBufferPtr->tid           = glm::vec2(textureIndex, outlineWidth);
                        TextVertexBufferPtr->outlineColour = outlineColour;
                        TextVertexBufferPtr++;

                        TextVertexBufferPtr->vertex        = transform * glm::vec4(pr, pb, 0.0f, 1.0f);
                        TextVertexBufferPtr->colour        = colour;
                        TextVertexBufferPtr->uv            = { r, b };
                        TextVertexBufferPtr->tid           = glm::vec2(textureIndex, outlineWidth);
                        TextVertexBufferPtr->outlineColour = outlineColour;
                        TextVertexBufferPtr++;

                        TextVertexBufferPtr->vertex        = transform * glm::vec4(pr, pt, 0.0f, 1.0f);
                        TextVertexBufferPtr->colour        = colour;
                        TextVertexBufferPtr->uv            = { r, t };
                        TextVertexBufferPtr->tid           = glm::vec2(textureIndex, outlineWidth);
                        TextVertexBufferPtr->outlineColour = outlineColour;
                        TextVertexBufferPtr++;

                        TextVertexBufferPtr->vertex        = transform * glm::vec4(pl, pt, 0.0f, 1.0f);
                        TextVertexBufferPtr->colour        = colour;
                        TextVertexBufferPtr->uv            = { l, t };
                        TextVertexBufferPtr->tid           = glm::vec2(textureIndex, outlineWidth);
                        TextVertexBufferPtr->outlineColour = outlineColour;
                        TextVertexBufferPtr++;
                    }

                    m_TextRendererData.m_IndexCount += 6;

                    double advance = glyph->getAdvance();
                    fontGeometry.getAdvance(advance, character, string[i + 1]);
                    x += fsScale * advance + kerningOffset;
                }
            }
        }

        if(m_TextRendererData.m_IndexCount == 0)
        {
            // m_TextRendererData.m_VertexBuffers[currentFrame][m_TextRendererData.m_BatchDrawCallIndex]->ReleasePointer();
            return;
        }

        TextFlush(m_TextRendererData, TextVertexBufferBase, TextVertexBufferPtr);

        // m_TextRendererData.m_VertexBuffers[currentFrame][m_TextRendererData.m_BatchDrawCallIndex]->ReleasePointer();
    }

    void RenderPasses::DebugPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Debug Pass");

        if(!m_Camera || !m_CameraTransform)
            return;

        if(!m_DebugRenderDataInitialised)
        {
            LUMOS_LOG_WARN("Debug Render data not initialised");
            return;
        }
        m_DebugDrawData.m_LineBatchDrawCallIndex              = 0;
        m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex = 0;
        m_DebugDrawData.m_PointBatchDrawCallIndex             = 0;
        // Loop twice for depth test and no depth test
        for(int i = 0; i < 2; i++)
        {
            bool depthTest = i == 0;

            auto& lines      = DebugRenderer::GetInstance()->GetLines(depthTest);
            auto& thickLines = DebugRenderer::GetInstance()->GetThickLines(depthTest);
            auto& triangles  = DebugRenderer::GetInstance()->GetTriangles(depthTest);
            auto& points     = DebugRenderer::GetInstance()->GetPoints(depthTest);

            auto projView = m_Camera->GetProjectionMatrix() * glm::inverse(m_CameraTransform->GetWorldMatrix());

            if(!lines.empty())
            {
                LUMOS_PROFILE_SCOPE("Debug Lines");
                m_DebugDrawData.m_LineDescriptorSet[0]->SetUniform("UBO", "projView", &projView);
                m_DebugDrawData.m_LineDescriptorSet[0]->Update();

                Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();

                Graphics::PipelineDesc pipelineDesc;
                pipelineDesc.shader = m_DebugDrawData.m_LineShader;

                pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
                pipelineDesc.cullMode            = Graphics::CullMode::BACK;
                pipelineDesc.transparencyEnabled = true;
                pipelineDesc.blendMode           = BlendMode::SrcAlphaOneMinusSrcAlpha;
                pipelineDesc.clearTargets        = false;
                pipelineDesc.drawType            = DrawType::LINES;
                pipelineDesc.colourTargets[0]    = m_MainTexture;
                pipelineDesc.DebugName           = "Debug-Lines";
                if(m_MainTextureSamples > 1)
                    pipelineDesc.resolveTexture = m_ResolveTexture;
                pipelineDesc.samples = m_MainTextureSamples;
                if(depthTest)
                    pipelineDesc.depthTarget = m_ForwardData.m_DepthTexture;

                auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
                Renderer::GetMainSwapChain()->GetCurrentCommandBuffer()->BindPipeline(pipeline);

                if(m_DebugDrawData.m_LineVertexBuffers.empty())
                    m_DebugDrawData.m_LineVertexBuffers.emplace_back();
                if((int)m_DebugDrawData.m_LineVertexBuffers[0].size() - 1 < (int)m_DebugDrawData.m_LineBatchDrawCallIndex)
                {
                    auto& vertexBuffer = m_DebugDrawData.m_LineVertexBuffers[0].emplace_back(Graphics::VertexBuffer::Create(RENDERER_LINE_BUFFER_SIZE, nullptr, BufferUsage::DYNAMIC));
                    // vertexBuffer->Resize(RENDERER_LINE_BUFFER_SIZE);
                }

                m_DebugDrawData.m_LineVertexBuffers[0][m_DebugDrawData.m_LineBatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), pipeline.get());
                m_DebugDrawData.m_LineBuffer = m_LineBufferBase[0][0]; //[m_DebugDrawData.m_LineBatchDrawCallIndex];

                // m_DebugDrawData.m_LineBuffer = m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->GetPointer<LineVertexData>();

                for(auto& line : lines)
                {
                    if(m_DebugDrawData.LineIndexCount >= MaxLineIndices)
                        DebugLineFlush(pipeline);

                    m_DebugDrawData.m_LineBuffer->vertex = line.p1;
                    m_DebugDrawData.m_LineBuffer->colour = line.col;
                    m_DebugDrawData.m_LineBuffer++;

                    m_DebugDrawData.m_LineBuffer->vertex = line.p2;
                    m_DebugDrawData.m_LineBuffer->colour = line.col;
                    m_DebugDrawData.m_LineBuffer++;

                    m_DebugDrawData.LineIndexCount += 2;
                }

                // m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->ReleasePointer();
                // m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->Unbind();
                // m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->Bind(commandBuffer, pipeline.get());

                DebugLineFlush(pipeline);
            }

            if(!thickLines.empty())
            {
                LUMOS_PROFILE_SCOPE("Debug Thick Lines");

                m_DebugDrawData.m_LineDescriptorSet[0]->SetUniform("UBO", "projView", &projView);
                m_DebugDrawData.m_LineDescriptorSet[0]->Update();

                Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();

                Graphics::PipelineDesc pipelineDesc;
                pipelineDesc.shader = m_DebugDrawData.m_LineShader;

                pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
                pipelineDesc.cullMode            = Graphics::CullMode::BACK;
                pipelineDesc.transparencyEnabled = true;
                pipelineDesc.clearTargets        = false;
                pipelineDesc.drawType            = DrawType::LINES;
                pipelineDesc.blendMode           = BlendMode::SrcAlphaOneMinusSrcAlpha;
                pipelineDesc.colourTargets[0]    = m_MainTexture;
                pipelineDesc.lineWidth           = 2.0f;
                pipelineDesc.DebugName           = "Debug-ThickLines";
                if(m_MainTextureSamples > 1)
                    pipelineDesc.resolveTexture = m_ResolveTexture;
                pipelineDesc.samples = m_MainTextureSamples;
                if(depthTest)
                    pipelineDesc.depthTarget = m_ForwardData.m_DepthTexture;

                auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
                Renderer::GetMainSwapChain()->GetCurrentCommandBuffer()->BindPipeline(pipeline);

                if(m_DebugDrawData.m_LineVertexBuffers.empty())
                    m_DebugDrawData.m_LineVertexBuffers.emplace_back();
                if((int)m_DebugDrawData.m_LineVertexBuffers[0].size() - 1 < (int)m_DebugDrawData.m_LineBatchDrawCallIndex)
                {
                    auto& vertexBuffer = m_DebugDrawData.m_LineVertexBuffers[0].emplace_back(Graphics::VertexBuffer::Create(RENDERER_LINE_BUFFER_SIZE, nullptr, BufferUsage::DYNAMIC));
                    // vertexBuffer->Resize(RENDERER_LINE_BUFFER_SIZE);
                }

                m_DebugDrawData.m_LineVertexBuffers[0][m_DebugDrawData.m_LineBatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), pipeline.get());
                m_DebugDrawData.m_LineBuffer = m_LineBufferBase[0][0]; //[m_DebugDrawData.m_LineBatchDrawCallIndex];

                for(auto& line : thickLines)
                {
                    if(m_DebugDrawData.LineIndexCount >= MaxLineIndices)
                        DebugLineFlush(pipeline);

                    m_DebugDrawData.m_LineBuffer->vertex = line.p1;
                    m_DebugDrawData.m_LineBuffer->colour = line.col;
                    m_DebugDrawData.m_LineBuffer++;

                    m_DebugDrawData.m_LineBuffer->vertex = line.p2;
                    m_DebugDrawData.m_LineBuffer->colour = line.col;
                    m_DebugDrawData.m_LineBuffer++;

                    m_DebugDrawData.LineIndexCount += 2;
                }

                DebugLineFlush(pipeline.get());
            }

            if(!points.empty())
            {
                LUMOS_PROFILE_SCOPE("Debug Points");
                m_DebugDrawData.m_PointDescriptorSet[0]->SetUniform("UBO", "projView", &projView);
                m_DebugDrawData.m_PointDescriptorSet[0]->Update();

                Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();

                Graphics::PipelineDesc pipelineDesc;
                pipelineDesc.shader = m_DebugDrawData.m_PointShader;

                pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
                pipelineDesc.cullMode            = Graphics::CullMode::BACK;
                pipelineDesc.transparencyEnabled = true;
                pipelineDesc.drawType            = DrawType::TRIANGLE;
                pipelineDesc.blendMode           = BlendMode::SrcAlphaOneMinusSrcAlpha;
                pipelineDesc.colourTargets[0]    = m_MainTexture;
                pipelineDesc.DebugName           = "Debug-Points";
                if(m_MainTextureSamples > 1)
                    pipelineDesc.resolveTexture = m_ResolveTexture;
                pipelineDesc.samples = m_MainTextureSamples;
                if(depthTest)
                    pipelineDesc.depthTarget = m_ForwardData.m_DepthTexture;

                auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
                commandBuffer->BindPipeline(pipeline);

                if(m_DebugDrawData.m_PointVertexBuffers.empty())
                    m_DebugDrawData.m_PointVertexBuffers.emplace_back();

                int bufferCount = 0;
                bufferCount     = (int)m_DebugDrawData.m_PointVertexBuffers[0].size();
                if((bufferCount - 1) < (int)m_DebugDrawData.m_PointBatchDrawCallIndex)
                {
                    m_DebugDrawData.m_PointVertexBuffers[0].emplace_back(Graphics::VertexBuffer::Create(RENDERER_POINT_BUFFER_SIZE, nullptr, BufferUsage::DYNAMIC));
                }

                m_DebugDrawData.m_PointVertexBuffers[0][m_DebugDrawData.m_PointBatchDrawCallIndex]->Bind(commandBuffer, pipeline.get());
                m_DebugDrawData.m_PointBuffer = m_PointBufferBase[0][0]; // m_DebugDrawData.m_PointVertexBuffers[m_DebugDrawData.m_PointBatchDrawCallIndex]->GetPointer<PointVertexData>();

                for(auto& pointInfo : points)
                {
                    if(m_DebugDrawData.PointIndexCount >= MaxPointIndices)
                        DebugPointFlush(pipeline);

                    glm::vec3 right = pointInfo.size * m_CameraTransform->GetRightDirection();
                    glm::vec3 up    = pointInfo.size * m_CameraTransform->GetUpDirection();

                    m_DebugDrawData.m_PointBuffer->vertex = pointInfo.p1 - right - up; // + glm::vec3(-pointInfo.size, -pointInfo.size, 0.0f));
                    m_DebugDrawData.m_PointBuffer->colour = pointInfo.col;
                    m_DebugDrawData.m_PointBuffer->size   = { pointInfo.size, 0.0f };
                    m_DebugDrawData.m_PointBuffer->uv     = { -1.0f, -1.0f };
                    m_DebugDrawData.m_PointBuffer++;

                    m_DebugDrawData.m_PointBuffer->vertex = pointInfo.p1 + right - up; //(pointInfo.p1 + glm::vec3(pointInfo.size, -pointInfo.size, 0.0f));
                    m_DebugDrawData.m_PointBuffer->colour = pointInfo.col;
                    m_DebugDrawData.m_PointBuffer->size   = { pointInfo.size, 0.0f };
                    m_DebugDrawData.m_PointBuffer->uv     = { 1.0f, -1.0f };
                    m_DebugDrawData.m_PointBuffer++;

                    m_DebugDrawData.m_PointBuffer->vertex = pointInfo.p1 + right + up; //(pointInfo.p1 + glm::vec3(pointInfo.size, pointInfo.size, 0.0f));
                    m_DebugDrawData.m_PointBuffer->colour = pointInfo.col;
                    m_DebugDrawData.m_PointBuffer->size   = { pointInfo.size, 0.0f };
                    m_DebugDrawData.m_PointBuffer->uv     = { 1.0f, 1.0f };
                    m_DebugDrawData.m_PointBuffer++;

                    m_DebugDrawData.m_PointBuffer->vertex = pointInfo.p1 - right + up; // (pointInfo.p1 + glm::vec3(-pointInfo.size, pointInfo.size, 0.0f));
                    m_DebugDrawData.m_PointBuffer->colour = pointInfo.col;
                    m_DebugDrawData.m_PointBuffer->size   = { pointInfo.size, 0.0f };
                    m_DebugDrawData.m_PointBuffer->uv     = { -1.0f, 1.0f };
                    m_DebugDrawData.m_PointBuffer++;

                    m_DebugDrawData.PointIndexCount += 6;
                }

                DebugPointFlush(pipeline.get());
            }

            if(!triangles.empty())
            {
                LUMOS_PROFILE_SCOPE("Triangles Lines");

                m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0][0]->SetUniform("UBO", "projView", &projView);
                m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0][0]->Update();
                m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0][1]->Update();

                Graphics::PipelineDesc pipelineDesc;
                pipelineDesc.shader = m_DebugDrawData.m_Renderer2DData.m_Shader;

                pipelineDesc.polygonMode             = Graphics::PolygonMode::FILL;
                pipelineDesc.cullMode                = Graphics::CullMode::BACK;
                pipelineDesc.transparencyEnabled     = true;
                pipelineDesc.blendMode               = BlendMode::SrcAlphaOneMinusSrcAlpha;
                pipelineDesc.clearTargets            = false;
                pipelineDesc.colourTargets[0]        = m_MainTexture;
                pipelineDesc.depthBiasEnabled        = true;
                pipelineDesc.depthBiasConstantFactor = 0.0f;
                pipelineDesc.depthBiasSlopeFactor    = -10.0f;
                pipelineDesc.DebugName               = "Debug-Triangles";
                if(m_MainTextureSamples > 1)
                    pipelineDesc.resolveTexture = m_ResolveTexture;
                pipelineDesc.samples = m_MainTextureSamples;
                if(depthTest)
                    pipelineDesc.depthTarget = m_ForwardData.m_DepthTexture;

                auto pipeline                                   = Graphics::Pipeline::Get(pipelineDesc);
                m_DebugDrawData.m_Renderer2DData.m_TextureCount = 0;
                uint32_t currentFrame                           = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

                if((int)m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[currentFrame].size() - 1 < (int)m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex)
                {
                    auto& vertexBuffer = m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[currentFrame].emplace_back(Graphics::VertexBuffer::Create(RENDERER_LINE_BUFFER_SIZE, nullptr, BufferUsage::DYNAMIC));
                }

                m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), pipeline.get());
                // m_DebugDrawData.m_Renderer2DData.m_Buffer = m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex]->GetPointer<VertexData>();
                m_DebugDrawData.m_Renderer2DData.m_Buffer = m_QuadBufferBase[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex];

                Arena* frameArena                  = Application::Get().GetFrameArena();
                DescriptorSet** currentDescriptors = PushArrayNoZero(frameArena, DescriptorSet*, 2);

                for(auto& triangleInfo : triangles)
                {
                    m_Stats.NumRenderedObjects++;

                    float textureSlot = 0.0f;

                    m_DebugDrawData.m_Renderer2DData.m_Buffer->vertex = triangleInfo.p1;
                    m_DebugDrawData.m_Renderer2DData.m_Buffer->uv     = { 0.0f, 0.0f, 0.0f, 0.0f };
                    m_DebugDrawData.m_Renderer2DData.m_Buffer->tid    = glm::vec2(textureSlot, 0.0f);
                    m_DebugDrawData.m_Renderer2DData.m_Buffer->colour = triangleInfo.col;
                    m_DebugDrawData.m_Renderer2DData.m_Buffer++;
                    m_DebugDrawData.m_Renderer2DData.m_Buffer->vertex = triangleInfo.p2;
                    m_DebugDrawData.m_Renderer2DData.m_Buffer->uv     = { 0.0f, 0.0f, 0.0f, 0.0f };
                    m_DebugDrawData.m_Renderer2DData.m_Buffer->tid    = glm::vec2(textureSlot, 0.0f);
                    m_DebugDrawData.m_Renderer2DData.m_Buffer->colour = triangleInfo.col;
                    m_DebugDrawData.m_Renderer2DData.m_Buffer++;
                    m_DebugDrawData.m_Renderer2DData.m_Buffer->vertex = triangleInfo.p3;
                    m_DebugDrawData.m_Renderer2DData.m_Buffer->uv     = { 0.0f, 0.0f, 0.0f, 0.0f };
                    m_DebugDrawData.m_Renderer2DData.m_Buffer->tid    = glm::vec2(textureSlot, 0.0f);
                    m_DebugDrawData.m_Renderer2DData.m_Buffer->colour = triangleInfo.col;
                    m_DebugDrawData.m_Renderer2DData.m_Buffer++;
                    m_DebugDrawData.m_Renderer2DData.m_IndexCount += 3;
                }

                Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();

                currentDescriptors[0] = m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0][0].get();
                currentDescriptors[1] = m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0][1].get();

                m_DebugDrawData.m_Renderer2DData.m_IndexBuffer->SetCount(m_DebugDrawData.m_Renderer2DData.m_IndexCount);
                m_DebugDrawData.m_Renderer2DData.m_IndexBuffer->Bind(commandBuffer);

                uint32_t dataSize = (uint32_t)((uint8_t*)m_DebugDrawData.m_Renderer2DData.m_Buffer - (uint8_t*)m_QuadBufferBase[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex]);
                m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex]->SetData(dataSize, (void*)m_QuadBufferBase[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex], true);

                commandBuffer->BindPipeline(pipeline);

                Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, currentDescriptors, 2);
                Renderer::DrawIndexed(commandBuffer, DrawType::TRIANGLE, m_DebugDrawData.m_Renderer2DData.m_IndexCount);

                m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex]->Unbind();
                m_DebugDrawData.m_Renderer2DData.m_IndexBuffer->Unbind();

                // m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex++;
                m_DebugDrawData.m_Renderer2DData.m_Buffer     = m_QuadBufferBase[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex];
                m_DebugDrawData.m_Renderer2DData.m_IndexCount = 0;
            }
        }

        m_DebugDrawData.m_PointBatchDrawCallIndex = 0;
        m_DebugDrawData.m_LineBatchDrawCallIndex  = 0;

        // Text Pass
        auto& dtDebugText = DebugRenderer::GetInstance()->GetDebugText();
        if(!dtDebugText.empty())
        {
            LUMOS_PROFILE_SCOPE("Debug Text");

            Graphics::PipelineDesc pipelineDesc;
            pipelineDesc.shader              = m_DebugTextRendererData.m_Shader;
            pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
            pipelineDesc.cullMode            = Graphics::CullMode::BACK;
            pipelineDesc.transparencyEnabled = true;
            pipelineDesc.blendMode           = BlendMode::SrcAlphaOneMinusSrcAlpha;
            pipelineDesc.clearTargets        = false;
            pipelineDesc.colourTargets[0]    = m_MainTexture;
            pipelineDesc.DebugName           = "Debug-TextDT";
            if(m_MainTextureSamples > 1)
                pipelineDesc.resolveTexture = m_ResolveTexture;
            pipelineDesc.samples = m_MainTextureSamples;
            // pipelineDesc.depthTarget           = m_ForwardData.m_DepthTexture;
            m_DebugTextRendererData.m_Pipeline = Graphics::Pipeline::Get(pipelineDesc);

            uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

            while((int)m_DebugTextRendererData.m_VertexBuffers[currentFrame].size() - 1 < (int)m_DebugTextRendererData.m_BatchDrawCallIndex)
            {
                auto& vertexBuffer = m_DebugTextRendererData.m_VertexBuffers[currentFrame].emplace_back(Graphics::VertexBuffer::Create(BufferUsage::DYNAMIC));
                vertexBuffer->Resize(RENDERER_LINE_BUFFER_SIZE);
            }

            m_DebugTextRendererData.m_VertexBuffers[currentFrame][m_DebugTextRendererData.m_BatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), m_DebugTextRendererData.m_Pipeline.get());
            DebugTextVertexBufferPtr = DebugTextVertexBufferBase[currentFrame];
            auto projView            = m_Camera->GetProjectionMatrix() * glm::inverse(m_CameraTransform->GetWorldMatrix());

            m_DebugTextRendererData.m_DescriptorSet[m_DebugTextRendererData.m_BatchDrawCallIndex][0]->SetUniform("UBO", "projView", &projView);
            m_DebugTextRendererData.m_DescriptorSet[m_DebugTextRendererData.m_BatchDrawCallIndex][0]->Update();

            m_DebugTextRendererData.m_TextureCount = 0;
            m_Stats.NumRenderedObjects++;

            if(m_DebugTextRendererData.m_IndexCount >= m_DebugTextRendererData.m_Limits.IndiciesSize)
                TextFlush(m_DebugTextRendererData, DebugTextVertexBufferBase, DebugTextVertexBufferPtr);

            int textureIndex = -1;
            // auto& string = textComp.TextString;
            auto font              = Font::GetDefaultFont();
            float lineHeightOffset = 0.0f;
            float kerningOffset    = 0.0f;
            float outlineWidth     = 0.0f;

            SharedPtr<Texture2D> fontAtlas = font->GetFontAtlas();
            if(!fontAtlas)
                return; // TODO

            for(uint32_t i = 0; i < m_DebugTextRendererData.m_TextureCount; i++)
            {
                if(m_DebugTextRendererData.m_Textures[i] == fontAtlas.get())
                {
                    textureIndex = int(i);
                    break;
                }
            }

            if(textureIndex == -1)
            {
                textureIndex                                                               = (int)m_DebugTextRendererData.m_TextureCount;
                m_DebugTextRendererData.m_Textures[m_DebugTextRendererData.m_TextureCount] = fontAtlas.get();
                m_DebugTextRendererData.m_TextureCount++;
            }

            auto& fontGeometry  = font->GetMSDFData()->FontGeometry;
            const auto& metrics = fontGeometry.getMetrics();

            for(auto& text : dtDebugText)
            {
                double x       = 0.0;
                double fsScale = 1 / (metrics.ascenderY - metrics.descenderY);
                double y       = 0.0;
                for(int i = 0; i < text.text.size(); i++)
                {
                    char32_t character      = (char32_t)text.text[i];
                    glm::vec4 pos           = text.Position;
                    glm::vec4 colour        = text.colour;
                    glm::vec4 outlineColour = text.colour;
                    float size              = text.Size;

                    if(character == '\r')
                        continue;

                    if(character == '\n')
                    {
                        x = 0;
                        y -= fsScale * metrics.lineHeight + lineHeightOffset;
                        continue;
                    }

                    if(character == '\t')
                    {
                        auto glyph     = fontGeometry.getGlyph('a');
                        double advance = glyph->getAdvance();
                        x += 4 * fsScale * advance + kerningOffset;
                        continue;
                    }

                    auto glyph = fontGeometry.getGlyph(character);
                    if(!glyph)
                        glyph = fontGeometry.getGlyph('?');
                    if(!glyph)
                        continue;

                    double l, b, r, t;
                    glyph->getQuadAtlasBounds(l, b, r, t);

                    double pl, pb, pr, pt;
                    glyph->getQuadPlaneBounds(pl, pb, pr, pt);

                    pl *= fsScale, pb *= fsScale, pr *= fsScale, pt *= fsScale;
                    pl += x, pb += y, pr += x, pt += y;

                    double texelWidth  = 1. / fontAtlas->GetWidth();
                    double texelHeight = 1. / fontAtlas->GetHeight();
                    l *= texelWidth, b *= texelHeight, r *= texelWidth, t *= texelHeight;

                    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(pos)) * glm::toMat4(m_CameraTransform->GetLocalOrientation()) * glm::scale(glm::mat4(1.0), glm::vec3(size / 10.0f));

                    {
                        LUMOS_PROFILE_SCOPE("Set text buffer data");
                        DebugTextVertexBufferPtr->vertex        = transform * glm::vec4(pl, pb, 0.0f, 1.0f);
                        DebugTextVertexBufferPtr->colour        = colour;
                        DebugTextVertexBufferPtr->uv            = { l, b };
                        DebugTextVertexBufferPtr->tid           = glm::vec2(textureIndex, outlineWidth);
                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
                        DebugTextVertexBufferPtr++;

                        DebugTextVertexBufferPtr->vertex        = transform * glm::vec4(pr, pb, 0.0f, 1.0f);
                        DebugTextVertexBufferPtr->colour        = colour;
                        DebugTextVertexBufferPtr->uv            = { r, b };
                        DebugTextVertexBufferPtr->tid           = glm::vec2(textureIndex, outlineWidth);
                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
                        DebugTextVertexBufferPtr++;

                        DebugTextVertexBufferPtr->vertex        = transform * glm::vec4(pr, pt, 0.0f, 1.0f);
                        DebugTextVertexBufferPtr->colour        = colour;
                        DebugTextVertexBufferPtr->uv            = { r, t };
                        DebugTextVertexBufferPtr->tid           = glm::vec2(textureIndex, outlineWidth);
                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
                        DebugTextVertexBufferPtr++;

                        DebugTextVertexBufferPtr->vertex        = transform * glm::vec4(pl, pt, 0.0f, 1.0f);
                        DebugTextVertexBufferPtr->colour        = colour;
                        DebugTextVertexBufferPtr->uv            = { l, t };
                        DebugTextVertexBufferPtr->tid           = glm::vec2(textureIndex, outlineWidth);
                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
                        DebugTextVertexBufferPtr++;
                    }

                    m_DebugTextRendererData.m_IndexCount += 6;

                    double advance = glyph->getAdvance();

                    fontGeometry.getAdvance(advance, character, text.text[i + 1]);
                    x += fsScale * advance + kerningOffset;
                }
            }

            if(m_DebugTextRendererData.m_IndexCount == 0)
            {
                // m_DebugTextRendererData.m_VertexBuffers[currentFrame][m_DebugTextRendererData.m_BatchDrawCallIndex]->ReleasePointer();
                return;
            }

            TextFlush(m_DebugTextRendererData, DebugTextVertexBufferBase, DebugTextVertexBufferPtr);
        }

        auto& ndtDebugText = DebugRenderer::GetInstance()->GetDebugTextNDT();
        if(!ndtDebugText.empty())
        {
            LUMOS_PROFILE_SCOPE("NDT Debug Lines");

            Graphics::PipelineDesc pipelineDesc;
            pipelineDesc.shader              = m_DebugTextRendererData.m_Shader;
            pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
            pipelineDesc.cullMode            = Graphics::CullMode::BACK;
            pipelineDesc.transparencyEnabled = true;
            pipelineDesc.blendMode           = BlendMode::SrcAlphaOneMinusSrcAlpha;
            pipelineDesc.clearTargets        = false;
            pipelineDesc.colourTargets[0]    = m_MainTexture;
            pipelineDesc.DebugName           = "Debug-TextNDT";
            if(m_MainTextureSamples > 1)
                pipelineDesc.resolveTexture = m_ResolveTexture;
            pipelineDesc.samples               = m_MainTextureSamples;
            m_DebugTextRendererData.m_Pipeline = Graphics::Pipeline::Get(pipelineDesc);

            uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

            while((int)m_DebugTextRendererData.m_VertexBuffers[currentFrame].size() - 1 < (int)m_DebugTextRendererData.m_BatchDrawCallIndex)
            {
                auto& vertexBuffer = m_DebugTextRendererData.m_VertexBuffers[currentFrame].emplace_back(Graphics::VertexBuffer::Create(BufferUsage::DYNAMIC));
                vertexBuffer->Resize(RENDERER_LINE_BUFFER_SIZE);
            }

            m_DebugTextRendererData.m_VertexBuffers[currentFrame][m_DebugTextRendererData.m_BatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), m_DebugTextRendererData.m_Pipeline.get());
            DebugTextVertexBufferPtr = DebugTextVertexBufferBase[currentFrame];
            auto projView            = m_Camera->GetProjectionMatrix() * glm::inverse(m_CameraTransform->GetWorldMatrix());
            DebugRenderer::GetInstance()->SetProjView(projView);
            DebugRenderer::SortLists();

            m_DebugTextRendererData.m_DescriptorSet[m_DebugTextRendererData.m_BatchDrawCallIndex][0]->SetUniform("UBO", "projView", &projView);
            m_DebugTextRendererData.m_DescriptorSet[m_DebugTextRendererData.m_BatchDrawCallIndex][0]->Update();

            m_DebugTextRendererData.m_TextureCount = 0;
            m_Stats.NumRenderedObjects++;

            if(m_DebugTextRendererData.m_IndexCount >= m_DebugTextRendererData.m_Limits.IndiciesSize)
                TextFlush(m_DebugTextRendererData, DebugTextVertexBufferBase, DebugTextVertexBufferPtr);

            int textureIndex = -1;
            // auto& string = textComp.TextString;
            auto font              = Font::GetDefaultFont();
            float lineHeightOffset = 0.0f;
            float kerningOffset    = 0.0f;
            float outlineWidth     = 0.0f;

            SharedPtr<Texture2D> fontAtlas = font->GetFontAtlas();
            if(!fontAtlas)
                return; // TODO

            for(uint32_t i = 0; i < m_DebugTextRendererData.m_TextureCount; i++)
            {
                if(m_DebugTextRendererData.m_Textures[i] == fontAtlas.get())
                {
                    textureIndex = int(i);
                    break;
                }
            }

            if(textureIndex == -1)
            {
                textureIndex                                                               = (int)m_DebugTextRendererData.m_TextureCount;
                m_DebugTextRendererData.m_Textures[m_DebugTextRendererData.m_TextureCount] = fontAtlas.get();
                m_DebugTextRendererData.m_TextureCount++;
            }

            auto& fontGeometry  = font->GetMSDFData()->FontGeometry;
            const auto& metrics = fontGeometry.getMetrics();

            for(auto& text : ndtDebugText)
            {
                double x       = 0.0;
                double fsScale = 1 / (metrics.ascenderY - metrics.descenderY);
                double y       = 0.0;
                for(int i = 0; i < text.text.size(); i++)
                {
                    char32_t character      = (char32_t)text.text[i];
                    glm::vec4 pos           = text.Position;
                    glm::vec4 colour        = text.colour;
                    glm::vec4 outlineColour = text.colour;
                    float size              = text.Size;
                    if(character == '\r')
                        continue;

                    if(character == '\n')
                    {
                        x = 0;
                        y -= fsScale * metrics.lineHeight + lineHeightOffset;
                        continue;
                    }

                    if(character == '\t')
                    {
                        auto glyph     = fontGeometry.getGlyph('a');
                        double advance = glyph->getAdvance();
                        x += 4 * fsScale * advance + kerningOffset;
                        continue;
                    }

                    auto glyph = fontGeometry.getGlyph(character);
                    if(!glyph)
                        glyph = fontGeometry.getGlyph('?');
                    if(!glyph)
                        continue;

                    double l, b, r, t;
                    glyph->getQuadAtlasBounds(l, b, r, t);

                    double pl, pb, pr, pt;
                    glyph->getQuadPlaneBounds(pl, pb, pr, pt);

                    pl *= fsScale, pb *= fsScale, pr *= fsScale, pt *= fsScale;
                    pl += x, pb += y, pr += x, pt += y;

                    double texelWidth  = 1. / fontAtlas->GetWidth();
                    double texelHeight = 1. / fontAtlas->GetHeight();
                    l *= texelWidth, b *= texelHeight, r *= texelWidth, t *= texelHeight;

                    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(pos)) * glm::toMat4(m_CameraTransform->GetLocalOrientation()) * glm::scale(glm::mat4(1.0), glm::vec3(size / 10.0f));

                    {
                        LUMOS_PROFILE_SCOPE("Set text buffer data");
                        DebugTextVertexBufferPtr->vertex        = transform * glm::vec4(pl, pb, 0.0f, 1.0f);
                        DebugTextVertexBufferPtr->colour        = colour;
                        DebugTextVertexBufferPtr->uv            = { l, b };
                        DebugTextVertexBufferPtr->tid           = glm::vec2(textureIndex, outlineWidth);
                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
                        DebugTextVertexBufferPtr++;

                        DebugTextVertexBufferPtr->vertex        = transform * glm::vec4(pr, pb, 0.0f, 1.0f);
                        DebugTextVertexBufferPtr->colour        = colour;
                        DebugTextVertexBufferPtr->uv            = { r, b };
                        DebugTextVertexBufferPtr->tid           = glm::vec2(textureIndex, outlineWidth);
                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
                        DebugTextVertexBufferPtr++;

                        DebugTextVertexBufferPtr->vertex        = transform * glm::vec4(pr, pt, 0.0f, 1.0f);
                        DebugTextVertexBufferPtr->colour        = colour;
                        DebugTextVertexBufferPtr->uv            = { r, t };
                        DebugTextVertexBufferPtr->tid           = glm::vec2(textureIndex, outlineWidth);
                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
                        DebugTextVertexBufferPtr++;

                        DebugTextVertexBufferPtr->vertex        = transform * glm::vec4(pl, pt, 0.0f, 1.0f);
                        DebugTextVertexBufferPtr->colour        = colour;
                        DebugTextVertexBufferPtr->uv            = { l, t };
                        DebugTextVertexBufferPtr->tid           = glm::vec2(textureIndex, outlineWidth);
                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
                        DebugTextVertexBufferPtr++;
                    }

                    m_DebugTextRendererData.m_IndexCount += 6;

                    double advance = glyph->getAdvance();

                    fontGeometry.getAdvance(advance, character, text.text[i + 1]);
                    x += fsScale * advance + kerningOffset;
                }
            }

            if(m_DebugTextRendererData.m_IndexCount == 0)
            {
                // m_DebugTextRendererData.m_VertexBuffers[currentFrame][m_DebugTextRendererData.m_BatchDrawCallIndex]->ReleasePointer();
                return;
            }

            TextFlush(m_DebugTextRendererData, DebugTextVertexBufferBase, DebugTextVertexBufferPtr);
        }

        glm::mat4 csProjection = glm::ortho(0.0f, (float)m_MainTexture->GetWidth(), 0.0f, (float)m_MainTexture->GetHeight(), -100.0f, 100.0f);
        auto projView          = m_Camera->GetProjectionMatrix();
        projView               = glm::mat4(1.0f);

        float scale       = 1.0f;
        float aspectRatio = (float)m_MainTexture->GetWidth() / (float)m_MainTexture->GetHeight();
        projView          = glm::ortho(-aspectRatio * scale, aspectRatio * scale, -scale, scale, -10.0f, 10.0f);

        DebugRenderer::GetInstance()->SetProjView(projView);
        DebugRenderer::SortLists();

        auto& csDebugText = DebugRenderer::GetInstance()->GetDebugTextCS();
        if(!csDebugText.empty())
        {
            LUMOS_PROFILE_SCOPE("CS Debug Lines");

            Graphics::PipelineDesc pipelineDesc;
            pipelineDesc.shader              = m_DebugTextRendererData.m_Shader;
            pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
            pipelineDesc.cullMode            = Graphics::CullMode::BACK;
            pipelineDesc.transparencyEnabled = true;
            pipelineDesc.blendMode           = BlendMode::SrcAlphaOneMinusSrcAlpha;
            pipelineDesc.clearTargets        = false;
            pipelineDesc.colourTargets[0]    = m_MainTexture;
            pipelineDesc.DebugName           = "Debug-TextCS";
            if(m_MainTextureSamples > 1)
                pipelineDesc.resolveTexture = m_ResolveTexture;
            pipelineDesc.samples               = m_MainTextureSamples;
            m_DebugTextRendererData.m_Pipeline = Graphics::Pipeline::Get(pipelineDesc);

            uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

            while((int)m_DebugTextRendererData.m_VertexBuffers[currentFrame].size() - 1 < (int)m_DebugTextRendererData.m_BatchDrawCallIndex)
            {
                auto& vertexBuffer = m_DebugTextRendererData.m_VertexBuffers[currentFrame].emplace_back(Graphics::VertexBuffer::Create(BufferUsage::DYNAMIC));
                vertexBuffer->Resize(RENDERER_LINE_BUFFER_SIZE);
            }

            m_DebugTextRendererData.m_VertexBuffers[currentFrame][m_DebugTextRendererData.m_BatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), m_DebugTextRendererData.m_Pipeline.get());
            DebugTextVertexBufferPtr = DebugTextVertexBufferBase[currentFrame];

            m_DebugTextRendererData.m_DescriptorSet[m_DebugTextRendererData.m_BatchDrawCallIndex][0]->SetUniform("UBO", "projView", &projView);
            m_DebugTextRendererData.m_DescriptorSet[m_DebugTextRendererData.m_BatchDrawCallIndex][0]->Update();

            m_DebugTextRendererData.m_TextureCount = 0;
            m_Stats.NumRenderedObjects++;

            if(m_DebugTextRendererData.m_IndexCount >= m_DebugTextRendererData.m_Limits.IndiciesSize)
                TextFlush(m_DebugTextRendererData, DebugTextVertexBufferBase, DebugTextVertexBufferPtr);

            int textureIndex = -1;
            // auto& string = textComp.TextString;
            auto font              = Font::GetDefaultFont();
            float lineHeightOffset = 0.0f;
            float kerningOffset    = 0.0f;
            float outlineWidth     = 0.0f;

            SharedPtr<Texture2D> fontAtlas = font->GetFontAtlas();
            if(!fontAtlas)
                return; // TODO

            for(uint32_t i = 0; i < m_DebugTextRendererData.m_TextureCount; i++)
            {
                if(m_DebugTextRendererData.m_Textures[i] == fontAtlas.get())
                {
                    textureIndex = int(i);
                    break;
                }
            }

            if(textureIndex == -1)
            {
                textureIndex                                                               = (int)m_DebugTextRendererData.m_TextureCount;
                m_DebugTextRendererData.m_Textures[m_DebugTextRendererData.m_TextureCount] = fontAtlas.get();
                m_DebugTextRendererData.m_TextureCount++;
            }

            auto& fontGeometry  = font->GetMSDFData()->FontGeometry;
            const auto& metrics = fontGeometry.getMetrics();

            for(auto& text : csDebugText)
            {
                double x       = 0.0;
                double fsScale = 1 / (metrics.ascenderY - metrics.descenderY);
                double y       = 0.0;
                for(int i = 0; i < text.text.size(); i++)
                {
                    char32_t character      = (char32_t)text.text[i];
                    glm::vec4 pos           = text.Position;
                    glm::vec4 colour        = text.colour;
                    glm::vec4 outlineColour = text.colour;

                    if(character == '\r')
                        continue;

                    if(character == '\n')
                    {
                        x = 0;
                        y -= fsScale * metrics.lineHeight + lineHeightOffset;
                        continue;
                    }

                    if(character == '\t')
                    {
                        auto glyph     = fontGeometry.getGlyph('a');
                        double advance = glyph->getAdvance();
                        x += 4 * fsScale * advance + kerningOffset;
                        continue;
                    }

                    auto glyph = fontGeometry.getGlyph(character);
                    if(!glyph)
                        glyph = fontGeometry.getGlyph('?');
                    if(!glyph)
                        continue;

                    double l, b, r, t;
                    glyph->getQuadAtlasBounds(l, b, r, t);

                    double pl, pb, pr, pt;
                    glyph->getQuadPlaneBounds(pl, pb, pr, pt);

                    pl *= fsScale, pb *= fsScale, pr *= fsScale, pt *= fsScale;
                    pl += x, pb += y, pr += x, pt += y;

                    double texelWidth  = 1. / fontAtlas->GetWidth();
                    double texelHeight = 1. / fontAtlas->GetHeight();
                    l *= texelWidth, b *= texelHeight, r *= texelWidth, t *= texelHeight;

                    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(pos)) * glm::scale(glm::mat4(1.0), glm::vec3(0.05f, 0.05f, 0.05f));

                    {
                        LUMOS_PROFILE_SCOPE("Set text buffer data");
                        DebugTextVertexBufferPtr->vertex        = transform * glm::vec4(pl, pb, 0.0f, 1.0f);
                        DebugTextVertexBufferPtr->colour        = colour;
                        DebugTextVertexBufferPtr->uv            = { l, b };
                        DebugTextVertexBufferPtr->tid           = glm::vec2(textureIndex, outlineWidth);
                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
                        DebugTextVertexBufferPtr++;

                        DebugTextVertexBufferPtr->vertex        = transform * glm::vec4(pr, pb, 0.0f, 1.0f);
                        DebugTextVertexBufferPtr->colour        = colour;
                        DebugTextVertexBufferPtr->uv            = { r, b };
                        DebugTextVertexBufferPtr->tid           = glm::vec2(textureIndex, outlineWidth);
                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
                        DebugTextVertexBufferPtr++;

                        DebugTextVertexBufferPtr->vertex        = transform * glm::vec4(pr, pt, 0.0f, 1.0f);
                        DebugTextVertexBufferPtr->colour        = colour;
                        DebugTextVertexBufferPtr->uv            = { r, t };
                        DebugTextVertexBufferPtr->tid           = glm::vec2(textureIndex, outlineWidth);
                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
                        DebugTextVertexBufferPtr++;

                        DebugTextVertexBufferPtr->vertex        = transform * glm::vec4(pl, pt, 0.0f, 1.0f);
                        DebugTextVertexBufferPtr->colour        = colour;
                        DebugTextVertexBufferPtr->uv            = { l, t };
                        DebugTextVertexBufferPtr->tid           = glm::vec2(textureIndex, outlineWidth);
                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
                        DebugTextVertexBufferPtr++;
                    }

                    m_DebugTextRendererData.m_IndexCount += 6;

                    double advance = glyph->getAdvance();

                    fontGeometry.getAdvance(advance, character, text.text[i + 1]);
                    x += fsScale * advance + kerningOffset;
                }
            }

            if(m_DebugTextRendererData.m_IndexCount == 0)
            {
                // m_DebugTextRendererData.m_VertexBuffers[currentFrame][m_DebugTextRendererData.m_BatchDrawCallIndex]->ReleasePointer();
                return;
            }

            TextFlush(m_DebugTextRendererData, DebugTextVertexBufferBase, DebugTextVertexBufferPtr);
        }
    }

    void RenderPasses::DebugLineFlush(Graphics::Pipeline* pipeline)
    {
        LUMOS_PROFILE_FUNCTION();

        m_DebugDrawData.m_LineIndexBuffer->SetCount(m_DebugDrawData.LineIndexCount);
        m_DebugDrawData.m_LineIndexBuffer->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());

        uint32_t dataSize = (uint32_t)((uint8_t*)m_DebugDrawData.m_LineBuffer - (uint8_t*)m_LineBufferBase[0][0]);
        m_DebugDrawData.m_LineVertexBuffers[0][m_DebugDrawData.m_LineBatchDrawCallIndex]->SetData(dataSize, (void*)m_LineBufferBase[0][0]);

        auto* desc = m_DebugDrawData.m_LineDescriptorSet[0].get();
        Renderer::BindDescriptorSets(pipeline, Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), 0, &desc, 1);
        Renderer::DrawIndexed(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), DrawType::LINES, m_DebugDrawData.LineIndexCount);

        m_DebugDrawData.m_LineVertexBuffers[0][m_DebugDrawData.m_LineBatchDrawCallIndex]->Unbind();
        m_DebugDrawData.m_LineIndexBuffer->Unbind();

        m_DebugDrawData.m_LineBatchDrawCallIndex++;
        m_DebugDrawData.m_LineBuffer   = m_LineBufferBase[0][0]; //[m_DebugDrawData.m_LineBatchDrawCallIndex];
        m_DebugDrawData.LineIndexCount = 0;

        if(m_DebugDrawData.m_LineVertexBuffers.empty())
            m_DebugDrawData.m_LineVertexBuffers.emplace_back();
        if((int)m_DebugDrawData.m_LineVertexBuffers[0].size() - 1 < (int)m_DebugDrawData.m_LineBatchDrawCallIndex)
        {
            auto& vertexBuffer = m_DebugDrawData.m_LineVertexBuffers[0].emplace_back(Graphics::VertexBuffer::Create(RENDERER_LINE_BUFFER_SIZE, nullptr, BufferUsage::DYNAMIC));
            // vertexBuffer->Resize(RENDERER_LINE_BUFFER_SIZE);
        }

        m_DebugDrawData.m_LineVertexBuffers[0][m_DebugDrawData.m_LineBatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), pipeline);
        m_DebugDrawData.m_LineBuffer   = m_LineBufferBase[0][0]; //[m_DebugDrawData.m_LineBatchDrawCallIndex];
        m_DebugDrawData.LineIndexCount = 0;
    }

    void RenderPasses::DebugPointFlush(Graphics::Pipeline* pipeline)
    {
        LUMOS_PROFILE_FUNCTION();

        // m_DebugDrawData.m_PointVertexBuffers[m_DebugDrawData.m_PointBatchDrawCallIndex]->ReleasePointer();
        m_DebugDrawData.m_PointIndexBuffer->SetCount(m_DebugDrawData.PointIndexCount);
        m_DebugDrawData.m_PointIndexBuffer->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());

        uint32_t dataSize = (uint32_t)((uint8_t*)m_DebugDrawData.m_PointBuffer - (uint8_t*)m_PointBufferBase[0][0]);
        m_DebugDrawData.m_PointVertexBuffers[0][m_DebugDrawData.m_PointBatchDrawCallIndex]->SetData(dataSize, (void*)m_PointBufferBase[0][0]);

        auto* desc = m_DebugDrawData.m_PointDescriptorSet[0].get();
        Renderer::BindDescriptorSets(pipeline, Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), 0, &desc, 1);
        Renderer::DrawIndexed(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), DrawType::TRIANGLE, m_DebugDrawData.PointIndexCount);

        m_DebugDrawData.m_PointVertexBuffers[0][m_DebugDrawData.m_PointBatchDrawCallIndex]->Unbind();
        m_DebugDrawData.m_PointIndexBuffer->Unbind();

        m_DebugDrawData.m_PointBatchDrawCallIndex++;

        if(m_DebugDrawData.m_PointVertexBuffers.empty())
            m_DebugDrawData.m_PointVertexBuffers.emplace_back();

        if((int)m_DebugDrawData.m_PointVertexBuffers[0].size() - 1 < (int)m_DebugDrawData.m_PointBatchDrawCallIndex)
        {
            m_DebugDrawData.m_PointVertexBuffers[0].emplace_back(Graphics::VertexBuffer::Create(RENDERER_POINT_BUFFER_SIZE, nullptr, BufferUsage::DYNAMIC));
        }

        m_DebugDrawData.m_PointVertexBuffers[0][m_DebugDrawData.m_PointBatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), pipeline);
        m_DebugDrawData.m_PointBuffer   = m_PointBufferBase[0][0];
        m_DebugDrawData.PointIndexCount = 0;
    }

    void RenderPasses::CreateCubeMap(const std::string& filePath, const glm::vec4& params, SharedPtr<TextureCube>& outEnv, SharedPtr<TextureCube>& outIrr)
    {
        // Create shader and pipeline
        // Create Empty Cube Map
        auto environmentMap         = TextureCube::Create(Application::Get().GetRenderConfigSettings().EnvironmentMapSize, nullptr, true);
        auto environmentMapFiltered = TextureCube::Create(Application::Get().GetRenderConfigSettings().EnvironmentMapSize, nullptr, true);
        auto irradianceMap          = TextureCube::Create(Application::Get().GetRenderConfigSettings().IrradianceMapSize, nullptr, true);

        auto commandBuffer = CommandBuffer::Create();
        commandBuffer->Init(true);
        commandBuffer->BeginRecording();
        {

            auto shader = Application::Get().GetAssetManager()->GetAssetData("CreateEnvironmentMap").As<Graphics::Shader>();

            Graphics::DescriptorDesc descriptorDesc {};
            descriptorDesc.layoutIndex = 0;
            descriptorDesc.shader      = shader.get();
            auto descriptorSet         = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

            SharedPtr<Texture> hdri = nullptr;

            if(!filePath.empty())
            {
                // Load hdri image
                TextureDesc textureParams;
                textureParams.srgb = false;
                hdri               = Texture2D::CreateFromFile("Environment", filePath, textureParams);
                descriptorSet->SetTexture("u_Texture", hdri);
            }
            else
            {
                descriptorSet->SetTexture("u_Texture", Material::GetDefaultTexture());
            }

            descriptorSet->SetUniform("UniformBuffer", "u_Parameters", (void*)&params);
            descriptorSet->Update(commandBuffer);

            Graphics::PipelineDesc pipelineDesc {};
            pipelineDesc.shader              = shader;
            pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
            pipelineDesc.cullMode            = Graphics::CullMode::BACK;
            pipelineDesc.transparencyEnabled = false;
            pipelineDesc.cubeMapTarget       = environmentMap;
            pipelineDesc.DebugName           = "Create Cubemap";

            for(uint32_t i = 0; i < 6; i++)
            {
                pipelineDesc.cubeMapIndex = i;

                auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
                commandBuffer->BindPipeline(pipeline, i);

                auto& pushConstants = shader->GetPushConstants();
                if(!pushConstants.empty())
                {
                    auto& pushConstant = shader->GetPushConstants()[0];
                    pushConstant.SetValue("cubeFaceIndex", (void*)&i);
                    shader->BindPushConstants(commandBuffer, pipeline.get());
                }

                auto set = descriptorSet.get();
                Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
                Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
            }
        }

        // Generate Mips
        {
            commandBuffer->UnBindPipeline();
            commandBuffer->EndCurrentRenderPass();
            environmentMap->GenerateMipMaps(commandBuffer);
            auto shader = Application::Get().GetAssetManager()->GetAssetData("EnvironmentMipFilter").As<Graphics::Shader>();

            Graphics::DescriptorDesc descriptorDesc {};
            descriptorDesc.layoutIndex = 0;
            descriptorDesc.shader      = shader.get();
            auto descriptorSet         = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
            descriptorSet->SetTexture("u_Texture", environmentMap);
            descriptorSet->Update(commandBuffer);

            const float deltaRoughness = 1.0f / Maths::Max((float)environmentMapFiltered->GetMipMapLevels() - 1.0f, 1.0f);

            for(uint32_t mip = 0; mip < environmentMapFiltered->GetMipMapLevels(); mip++)
            {
                Graphics::PipelineDesc pipelineDesc {};
                pipelineDesc.shader              = shader;
                pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
                pipelineDesc.cullMode            = Graphics::CullMode::BACK;
                pipelineDesc.transparencyEnabled = false;
                pipelineDesc.cubeMapTarget       = environmentMapFiltered;
                pipelineDesc.DebugName           = "Mip Generation";

                for(uint32_t i = 0; i < 6; i++)
                {
                    pipelineDesc.cubeMapIndex = i;
                    pipelineDesc.mipIndex     = mip;
                    auto pipeline             = Graphics::Pipeline::Get(pipelineDesc);
                    commandBuffer->BindPipeline(pipeline, i);

                    auto& pushConstants = shader->GetPushConstants();
                    if(!pushConstants.empty())
                    {
                        float roughness = mip * deltaRoughness;
                        roughness       = glm::max(roughness, 0.05f);

                        auto& pushConstant = shader->GetPushConstants()[0];
                        pushConstant.SetValue("Roughness", (void*)&roughness);
                        pushConstant.SetValue("cubeFaceIndex", (void*)&i);
                        shader->BindPushConstants(commandBuffer, pipeline.get());
                    }

                    auto set = descriptorSet.get();
                    Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
                    Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
                }
            }
        }

        {
            commandBuffer->UnBindPipeline();
            commandBuffer->EndCurrentRenderPass();

            auto shader = Application::Get().GetAssetManager()->GetAssetData("EnvironmentIrradiance").As<Graphics::Shader>();

            Graphics::DescriptorDesc descriptorDesc {};
            descriptorDesc.layoutIndex = 0;
            descriptorDesc.shader      = shader;
            auto descriptorSet         = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

            descriptorSet->SetTexture("u_Texture", environmentMapFiltered);

            uint32_t samples = 512;
            descriptorSet->SetUniform("UniformBuffer", "Samples", (void*)&samples);
            descriptorSet->Update(commandBuffer);

            Graphics::PipelineDesc pipelineDesc {};
            pipelineDesc.shader              = shader;
            pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
            pipelineDesc.cullMode            = Graphics::CullMode::BACK;
            pipelineDesc.transparencyEnabled = false;
            pipelineDesc.cubeMapTarget       = irradianceMap;

            for(uint32_t i = 0; i < 6; i++)
            {
                pipelineDesc.cubeMapIndex = i;

                auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
                commandBuffer->BindPipeline(pipeline, i);

                auto& pushConstants = shader->GetPushConstants();
                if(!pushConstants.empty())
                {
                    auto& pushConstant = shader->GetPushConstants()[0];
                    pushConstant.SetValue("cubeFaceIndex", (void*)&i);
                    shader->BindPushConstants(commandBuffer, pipeline.get());
                }

                auto set = descriptorSet.get();
                Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
                Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
            }
        }

        commandBuffer->UnBindPipeline();
        commandBuffer->EndCurrentRenderPass();
        irradianceMap->GenerateMipMaps(commandBuffer);

        commandBuffer->EndRecording();
        commandBuffer->Submit();

        Graphics::Renderer::GetGraphicsContext()->WaitIdle();
        environmentMap->Destroy(false);
        delete environmentMap;
        delete commandBuffer;
        outEnv = SharedPtr<TextureCube>(environmentMapFiltered);
        outIrr = SharedPtr<TextureCube>(irradianceMap);
    }

    void RenderPasses::InitDebugRenderData()
    {
        if(m_DebugRenderDataInitialised || !m_DebugRenderEnabled)
            return;

        for(int i = 0; i < Renderer::GetMainSwapChain()->GetSwapChainBufferCount(); i++)
        {
            if(m_DebugRenderEnabled)
            {
                m_QuadBufferBase.emplace_back().push_back(new VertexData[m_DebugDrawData.m_Renderer2DData.m_Limits.MaxQuads * 4]);

                // m_2DBufferBase.push_back(new VertexData[m_Renderer2DData.m_Limits.MaxQuads  * 4]);
                m_LineBufferBase.emplace_back().push_back(new LineVertexData[m_DebugDrawData.m_Renderer2DData.m_Limits.MaxQuads * 4]);
                m_PointBufferBase.emplace_back().push_back(new PointVertexData[m_DebugDrawData.m_Renderer2DData.m_Limits.MaxQuads * 4]);
                DebugTextVertexBufferBase.push_back(new TextVertexData[m_DebugTextRendererData.m_Limits.MaxQuads * 4]);
            }
        }
        // Points
        m_DebugDrawData.m_PointShader           = Application::Get().GetAssetManager()->GetAssetData("Batch2DPoint").As<Graphics::Shader>();
        Graphics::DescriptorDesc descriptorDesc = {};
        descriptorDesc.layoutIndex              = 0;
        descriptorDesc.shader                   = m_DebugDrawData.m_PointShader.get();

        m_DebugDrawData.m_PointDescriptorSet.resize(1);
        m_DebugDrawData.m_PointDescriptorSet[0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        uint32_t* indices = new uint32_t[MaxPointIndices];

        int32_t offset = 0;
        for(int32_t i = 0; i < MaxPointIndices; i += 6)
        {
            indices[i]     = offset + 0;
            indices[i + 1] = offset + 1;
            indices[i + 2] = offset + 2;

            indices[i + 3] = offset + 2;
            indices[i + 4] = offset + 3;
            indices[i + 5] = offset + 0;

            offset += 4;
        }

        m_DebugDrawData.m_PointIndexBuffer = IndexBuffer::Create(indices, MaxPointIndices);
        delete[] indices;

        // Lines
        m_DebugDrawData.m_LineShader = Application::Get().GetAssetManager()->GetAssetData("Batch2DLine").As<Graphics::Shader>();
        descriptorDesc.layoutIndex   = 0;
        descriptorDesc.shader        = m_DebugDrawData.m_LineShader.get();
        m_DebugDrawData.m_LineDescriptorSet.resize(1);
        m_DebugDrawData.m_LineDescriptorSet[0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        indices = new uint32_t[MaxLineIndices];

        for(int32_t i = 0; i < MaxLineIndices; i++)
        {
            indices[i] = i;
        }

        m_DebugDrawData.m_LineIndexBuffer = IndexBuffer::Create(indices, MaxLineIndices);
        delete[] indices;

        // Debug quads
        m_DebugDrawData.m_Renderer2DData.m_IndexCount           = 0;
        m_DebugDrawData.m_Renderer2DData.m_Buffer               = nullptr;
        m_DebugDrawData.m_Renderer2DData.m_RenderToDepthTexture = true;
        m_DebugDrawData.m_Renderer2DData.m_TriangleIndicies     = false;
        m_DebugDrawData.m_Renderer2DData.m_Limits.SetMaxQuads(10000);
        m_DebugDrawData.m_Renderer2DData.m_Shader = Application::Get().GetAssetManager()->GetAssetData("Batch2D").As<Graphics::Shader>();

        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_DebugDrawData.m_Renderer2DData.m_Shader.get();
        m_DebugDrawData.m_Renderer2DData.m_DescriptorSet.resize(1);

        m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0].resize(2);
        m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0][0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        descriptorDesc.layoutIndex                             = 1;
        m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0][1] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_DebugDrawData.m_Renderer2DData.m_VertexBuffers.resize(3);

        indices = new uint32_t[m_DebugDrawData.m_Renderer2DData.m_Limits.IndiciesSize];

        {
            for(uint32_t i = 0; i < m_DebugDrawData.m_Renderer2DData.m_Limits.IndiciesSize; i++)
            {
                indices[i] = i;
            }
        }
        m_DebugDrawData.m_Renderer2DData.m_IndexBuffer = IndexBuffer::Create(indices, m_Renderer2DData.m_Limits.IndiciesSize);

        delete[] indices;

        // Setup debug text pass
        m_DebugTextRendererData.m_IndexCount = 0;
        // m_TextBuffer                              = nullptr;
        m_DebugTextRendererData.m_RenderToDepthTexture = true;
        m_DebugTextRendererData.m_TriangleIndicies     = false;
        m_DebugTextRendererData.m_Limits.SetMaxQuads(10000);
        m_DebugTextRendererData.m_Limits.MaxTextures = 16; // Renderer::GetCapabilities().MaxTextureUnits;

        DebugTextVertexBufferPtr = DebugTextVertexBufferBase[0];

        m_DebugTextRendererData.m_Shader = Application::Get().GetAssetManager()->GetAssetData("Text").As<Graphics::Shader>();

        m_DebugTextRendererData.m_TransformationStack.emplace_back(glm::mat4(1.0f));
        m_DebugTextRendererData.m_TransformationBack = &m_DebugTextRendererData.m_TransformationStack.back();

        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_Renderer2DData.m_Shader.get();
        m_DebugTextRendererData.m_DescriptorSet.resize(m_DebugTextRendererData.m_Limits.MaxBatchDrawCalls);
        m_DebugTextRendererData.m_PreviousFrameTextureCount.resize(m_DebugTextRendererData.m_Limits.MaxBatchDrawCalls);

        for(uint32_t i = 0; i < m_DebugTextRendererData.m_Limits.MaxBatchDrawCalls; i++)
        {
            m_DebugTextRendererData.m_PreviousFrameTextureCount[i] = 0;
            m_DebugTextRendererData.m_DescriptorSet[i].resize(2);
            // if (i == 0)
            {
                descriptorDesc.layoutIndex                    = 0;
                m_DebugTextRendererData.m_DescriptorSet[i][0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
            }
            descriptorDesc.layoutIndex                    = 1;
            m_DebugTextRendererData.m_DescriptorSet[i][1] = nullptr; // SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        }

        m_DebugTextRendererData.m_VertexBuffers.resize(Renderer::GetMainSwapChain()->GetSwapChainBufferCount());
        indices = new uint32_t[m_DebugTextRendererData.m_Limits.IndiciesSize];

        if(m_DebugTextRendererData.m_TriangleIndicies)
        {
            for(uint32_t i = 0; i < m_DebugTextRendererData.m_Limits.IndiciesSize; i++)
            {
                indices[i] = i;
            }
        }
        else
        {
            uint32_t offset = 0;
            for(uint32_t i = 0; i < m_DebugTextRendererData.m_Limits.IndiciesSize; i += 6)
            {
                indices[i]     = offset + 0;
                indices[i + 1] = offset + 1;
                indices[i + 2] = offset + 2;

                indices[i + 3] = offset + 2;
                indices[i + 4] = offset + 3;
                indices[i + 5] = offset + 0;

                offset += 4;
            }
        }
        m_DebugTextRendererData.m_IndexBuffer = IndexBuffer::Create(indices, m_DebugTextRendererData.m_Limits.IndiciesSize);

        delete[] indices;

        m_DebugRenderDataInitialised = true;
    }

    void RenderPasses::Init2DRenderData()
    {
        m_DebugRenderDataInitialised = true;
    }

    float RenderPasses::SubmitParticleTexture(Texture* texture)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        float result = 0.0f;
        bool found   = false;

        for(uint32_t i = 0; i < m_ParticleData.m_TextureCount; i++)
        {
            if(m_ParticleData.m_Textures[i] == texture)
            {
                result = static_cast<float>(i + 1);
                found  = true;
                break;
            }
        }

        if(!found)
        {
            if(m_ParticleData.m_TextureCount >= m_ParticleData.m_Limits.MaxTextures)
            {
                Render2DFlush();
                ParticleBeginBatch();
            }
            m_ParticleData.m_Textures[m_ParticleData.m_TextureCount] = texture;
            m_ParticleData.m_TextureCount++;
            result = static_cast<float>(m_ParticleData.m_TextureCount);
        }
        return result;
    }

    bool compareParticles(const Particle& a, const Particle& b, const glm::vec3& cameraPosition)
    {
        // Calculate squared distance from camera to particle
        float distanceSqA = glm::length2(a.Position - cameraPosition);
        float distanceSqB = glm::length2(b.Position - cameraPosition);

        // Sort in descending order based on squared distance
        return distanceSqA > distanceSqB;
    }

    void RenderPasses::ParticlePass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Particle Pass");

        if(!m_Camera || !m_CameraTransform)
            return;

        auto emitterGroup = m_CurrentScene->GetRegistry().group<ParticleEmitter>(entt::get<Maths::Transform>);

        if(emitterGroup.empty())
            return;

        Graphics::PipelineDesc pipelineDesc;
        pipelineDesc.shader              = m_ParticleData.m_Shader;
        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = true;
        pipelineDesc.blendMode           = BlendMode::SrcAlphaOne;
        pipelineDesc.clearTargets        = false;
        pipelineDesc.depthTarget         = reinterpret_cast<Texture*>(m_ForwardData.m_DepthTexture);
        pipelineDesc.DepthTest           = true;
        pipelineDesc.DepthWrite          = false;
        pipelineDesc.colourTargets[0]    = m_MainTexture;
        if(m_MainTextureSamples > 1)
            pipelineDesc.resolveTexture = m_ResolveTexture;
        pipelineDesc.DebugName = "Particle";
        pipelineDesc.samples   = m_MainTextureSamples;
        ParticleBeginBatch();

        auto projView = m_Camera->GetProjectionMatrix() * glm::inverse(m_CameraTransform->GetWorldMatrix());

        auto cameraPos = m_CameraTransform->GetWorldPosition();
        m_ParticleData.m_DescriptorSet[0][0]->SetUniform("UBO", "projView", &projView);
        m_ParticleData.m_DescriptorSet[0][0]->Update();

        for(auto& emitterEntity : emitterGroup)
        {
            const auto& [emitter, trans] = emitterGroup.get<ParticleEmitter, Maths::Transform>(emitterEntity);
            Particle* particles          = emitter.GetParticles();
            uint32_t particleCount       = emitter.GetParticleCount();

            if(!particleCount)
                continue;

            if(emitter.GetBlendType() == ParticleEmitter::BlendType::Additive)
                pipelineDesc.blendMode = BlendMode::SrcAlphaOne;
            else
                pipelineDesc.blendMode = BlendMode::SrcAlphaOneMinusSrcAlpha;

            pipelineDesc.clearTargets            = false;
            pipelineDesc.DepthTest               = true;
            pipelineDesc.DepthWrite              = false;
            pipelineDesc.DebugName               = "Particle";
            pipelineDesc.depthBiasEnabled        = true;
            pipelineDesc.depthBiasConstantFactor = -1.25f;
            pipelineDesc.depthBiasSlopeFactor    = -1.75f;
            m_ParticleData.m_Pipeline            = Graphics::Pipeline::Get(pipelineDesc);

            if(emitter.GetSortParticles())
            {
                std::sort(&particles[0], &particles[particleCount - 1], [&](const Particle& a, const Particle& b)
                          { return compareParticles(a, b, m_CameraTransform->GetWorldPosition()); });
            }

            for(uint32_t i = 0; i < particleCount; i++)
            {
                auto& particle = particles[i];
                if(particle.Life <= 0.0f)
                    continue;

                m_Stats.NumRenderedObjects++;

                if(m_ParticleData.m_IndexCount >= m_ParticleData.m_Limits.IndiciesSize)
                {
                    ParticleFlush();
                    ParticleBeginBatch();
                }
                glm::vec3 v1;
                glm::vec3 v2;
                glm::vec3 v3;
                glm::vec3 v4;
                auto alignType = emitter.GetAlignedType();
                if(alignType == ParticleEmitter::Aligned2D)
                {
                    glm::vec3 rightOffset = glm::vec3(1.0f, 0.0f, 0.0f) * particle.Size * 0.5f;
                    glm::vec3 upOffset    = glm::vec3(0.0f, 1.0f, 0.0f) * particle.Size * 0.5f;

                    v1 = particle.Position - rightOffset - upOffset;
                    v2 = particle.Position + rightOffset - upOffset;
                    v3 = particle.Position + rightOffset + upOffset;
                    v4 = particle.Position - rightOffset + upOffset;
                }
                else if(alignType == ParticleEmitter::Aligned3D)
                {
                    glm::vec3 cameraRight = glm::normalize(m_CameraTransform->GetRightDirection());
                    glm::vec3 cameraUp    = glm::normalize(m_CameraTransform->GetUpDirection());

                    glm::vec3 rightOffset = cameraRight * particle.Size * 0.5f;
                    glm::vec3 upOffset    = cameraUp * particle.Size * 0.5f;

                    v1 = particle.Position - rightOffset - upOffset;
                    v2 = particle.Position + rightOffset - upOffset;
                    v3 = particle.Position + rightOffset + upOffset;
                    v4 = particle.Position - rightOffset + upOffset;
                }
                else
                {
                    glm::vec3 rightOffset = glm::vec3(particle.Size * 0.5f, 0.0f, 0.0f);
                    glm::vec3 upOffset    = glm::vec3(0.0f, particle.Size * 0.5f, 0.0f);

                    v1 = particle.Position - rightOffset - upOffset;
                    v2 = particle.Position + rightOffset - upOffset;
                    v3 = particle.Position + rightOffset + upOffset;
                    v4 = particle.Position - rightOffset + upOffset;
                }

                const glm::vec4 colour = particle.Colour;
                bool animated          = emitter.GetIsAnimated();
                std::array<glm::vec2, 4> uv;
                std::array<glm::vec4, 4> blendedUVs;

                float blendAmount = -1.0f;

                if(animated)
                {
                    blendedUVs = emitter.GetBlendedAnimatedUVs(1.0f - (particle.Life / emitter.GetParticleLife()), emitter.GetAnimatedTextureRows(), blendAmount);
                }
                else
                {
                    uv = emitter.GetDefaultUVs();
                }

                Texture* texture = emitter.GetTexture();

                if(!texture)
                    texture = Graphics::Material::GetDefaultTexture();

                float textureSlot = 0.0f;
                if(texture)
                    textureSlot = SubmitParticleTexture(texture);

                m_ParticleData.m_Buffer->vertex = v1;
                if(animated)
                {
                    m_ParticleData.m_Buffer->uv = blendedUVs[0];
                }
                else
                {
                    m_ParticleData.m_Buffer->uv.x = uv[0].x;
                    m_ParticleData.m_Buffer->uv.y = uv[0].y;
                }

                m_ParticleData.m_Buffer->tid    = glm::vec2(textureSlot, blendAmount);
                m_ParticleData.m_Buffer->colour = colour;
                m_ParticleData.m_Buffer++;

                m_ParticleData.m_Buffer->vertex = v2;
                if(animated)
                {
                    m_ParticleData.m_Buffer->uv = blendedUVs[1];
                }
                else
                {
                    m_ParticleData.m_Buffer->uv.x = uv[1].x;
                    m_ParticleData.m_Buffer->uv.y = uv[1].y;
                }
                m_ParticleData.m_Buffer->tid    = glm::vec2(textureSlot, blendAmount);
                m_ParticleData.m_Buffer->colour = colour;
                m_ParticleData.m_Buffer++;

                m_ParticleData.m_Buffer->vertex = v3;
                if(animated)
                {
                    m_ParticleData.m_Buffer->uv = blendedUVs[2];
                }
                else
                {
                    m_ParticleData.m_Buffer->uv.x = uv[2].x;
                    m_ParticleData.m_Buffer->uv.y = uv[2].y;
                }
                m_ParticleData.m_Buffer->tid    = glm::vec2(textureSlot, blendAmount);
                m_ParticleData.m_Buffer->colour = colour;
                m_ParticleData.m_Buffer++;

                m_ParticleData.m_Buffer->vertex = v4;
                if(animated)
                {
                    m_ParticleData.m_Buffer->uv = blendedUVs[3];
                }
                else
                {
                    m_ParticleData.m_Buffer->uv.x = uv[3].x;
                    m_ParticleData.m_Buffer->uv.y = uv[3].y;
                }
                m_ParticleData.m_Buffer->tid    = glm::vec2(textureSlot, blendAmount);
                m_ParticleData.m_Buffer->colour = colour;
                m_ParticleData.m_Buffer++;

                m_ParticleData.m_IndexCount += 6;
            }
        }

        if(m_ParticleData.m_IndexCount == 0)
        {
            return;
        }

        ParticleFlush();
    }

    void RenderPasses::ParticleBeginBatch()
    {
        uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

        m_ParticleData.m_IndexCount   = 0;
        m_ParticleData.m_TextureCount = 0;

        while((int)m_ParticleData.m_VertexBuffers[currentFrame].size() - 1 < (int)m_ParticleData.m_BatchDrawCallIndex)
        {
            auto& vertexBuffer = m_ParticleData.m_VertexBuffers[currentFrame].emplace_back(Graphics::VertexBuffer::Create(BufferUsage::DYNAMIC));
            vertexBuffer->Resize(m_ParticleData.m_Limits.BufferSize);

            m_ParticleBufferBase[currentFrame].emplace_back(new VertexData[m_ParticleData.m_Limits.MaxQuads * 4]);
        }

        m_ParticleData.m_VertexBuffers[currentFrame][m_ParticleData.m_BatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), m_ParticleData.m_Pipeline.get());
        m_ParticleData.m_Buffer = m_ParticleBufferBase[currentFrame][m_ParticleData.m_BatchDrawCallIndex];
    }

    void RenderPasses::ParticleFlush()
    {
        LUMOS_PROFILE_FUNCTION();
        uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();
        for(int i = m_ParticleData.m_TextureCount; i < 16; i++)
        {
            m_ParticleData.m_Textures[i] = Material::GetDefaultTexture();
        }

        m_ParticleData.m_TextureCount = 16;

        if(m_ParticleData.m_DescriptorSet[m_ParticleData.m_BatchDrawCallIndex][1] == nullptr || m_ParticleData.m_TextureCount != m_ParticleData.m_PreviousFrameTextureCount[m_ParticleData.m_BatchDrawCallIndex])
        {
            /*
             || m_ParticleData.m_TextureCount != m_ParticleData.m_PreviousFrameTextureCount[m_ParticleData.m_BatchDrawCallIndex])
             When previous frame texture count was less then than the previous frame
             and the texture previously used was deleted, there was a crash - maybe moltenvk only
             May not be needed anymore
            */
            Graphics::DescriptorDesc descriptorDesc {};
            descriptorDesc.layoutIndex                                             = 1;
            descriptorDesc.shader                                                  = m_ParticleData.m_Shader.get();
            m_ParticleData.m_DescriptorSet[m_ParticleData.m_BatchDrawCallIndex][1] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        }

        if(m_ParticleData.m_TextureCount > 1)
            m_ParticleData.m_DescriptorSet[m_ParticleData.m_BatchDrawCallIndex][1]->SetTexture("textures", m_ParticleData.m_Textures, m_ParticleData.m_TextureCount);
        else if(m_ParticleData.m_TextureCount == 0)
            m_ParticleData.m_DescriptorSet[m_ParticleData.m_BatchDrawCallIndex][1]->SetTexture("textures", nullptr);
        else
            m_ParticleData.m_DescriptorSet[m_ParticleData.m_BatchDrawCallIndex][1]->SetTexture("textures", m_ParticleData.m_Textures[0]);

        m_ParticleData.m_DescriptorSet[m_ParticleData.m_BatchDrawCallIndex][1]->Update();

        m_ParticleData.m_PreviousFrameTextureCount[m_ParticleData.m_BatchDrawCallIndex] = m_ParticleData.m_TextureCount;

        Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();

        commandBuffer->UnBindPipeline();
        uint32_t dataSize = (uint32_t)((uint8_t*)m_ParticleData.m_Buffer - (uint8_t*)m_ParticleBufferBase[currentFrame][m_ParticleData.m_BatchDrawCallIndex]);
        m_ParticleData.m_VertexBuffers[currentFrame][m_ParticleData.m_BatchDrawCallIndex]->SetData(dataSize, (void*)m_ParticleBufferBase[currentFrame][m_ParticleData.m_BatchDrawCallIndex], true);
        commandBuffer->BindPipeline(m_ParticleData.m_Pipeline);

        Arena* frameArena                  = Application::Get().GetFrameArena();
        DescriptorSet** currentDescriptors = PushArrayNoZero(frameArena, DescriptorSet*, 2);

        currentDescriptors[0] = m_ParticleData.m_DescriptorSet[0][0].get();
        currentDescriptors[1] = m_ParticleData.m_DescriptorSet[m_ParticleData.m_BatchDrawCallIndex][1].get();

        m_ParticleData.m_IndexBuffer->SetCount(m_ParticleData.m_IndexCount);
        m_ParticleData.m_IndexBuffer->Bind(commandBuffer);

        Renderer::BindDescriptorSets(m_ParticleData.m_Pipeline.get(), commandBuffer, 0, currentDescriptors, 2);
        Renderer::DrawIndexed(commandBuffer, DrawType::TRIANGLE, m_ParticleData.m_IndexCount);

        m_ParticleData.m_VertexBuffers[currentFrame][m_ParticleData.m_BatchDrawCallIndex]->Unbind();
        m_ParticleData.m_IndexBuffer->Unbind();

        m_ParticleData.m_BatchDrawCallIndex++;
        m_ParticleData.m_TextureCount = 0;
    }
}
