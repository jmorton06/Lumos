
#include "Precompiled.h"
#include "Renderer.h"

#include "Graphics/Mesh.h"
#include "Core/Application.h"
#include "Core/Asset/AssetManager.h"
#include "Core/OS/Window.h"
#include "IndexBuffer.h"
#include "VertexBuffer.h"

#include "CompiledSPV/Headers/Shadowvertspv.hpp"
#include "CompiledSPV/Headers/Shadowfragspv.hpp"
#include "CompiledSPV/Headers/ShadowAlphafragspv.hpp"
#include "CompiledSPV/Headers/ShadowAnimvertspv.hpp"

#include "CompiledSPV/Headers/ForwardPBRAnimvertspv.hpp"
#include "CompiledSPV/Headers/ForwardPBRvertspv.hpp"
#include "CompiledSPV/Headers/ForwardPBRfragspv.hpp"

#include "CompiledSPV/Headers/Skyboxvertspv.hpp"
#include "CompiledSPV/Headers/Skyboxfragspv.hpp"

#include "CompiledSPV/Headers/Batch2DPointvertspv.hpp"
#include "CompiledSPV/Headers/Batch2DPointfragspv.hpp"

#include "CompiledSPV/Headers/Batch2DLinevertspv.hpp"
#include "CompiledSPV/Headers/Batch2DLinefragspv.hpp"

#include "CompiledSPV/Headers/Batch2Dvertspv.hpp"
#include "CompiledSPV/Headers/Batch2Dfragspv.hpp"

#include "CompiledSPV/Headers/Particlevertspv.hpp"
#include "CompiledSPV/Headers/Particlefragspv.hpp"

#include "CompiledSPV/Headers/ScreenPassvertspv.hpp"
#include "CompiledSPV/Headers/ScreenPassfragspv.hpp"

#include "CompiledSPV/Headers/Gridvertspv.hpp"
#include "CompiledSPV/Headers/Gridfragspv.hpp"

#include "CompiledSPV/Headers/CreateEnvironmentMapfragspv.hpp"
#include "CompiledSPV/Headers/EnvironmentMipFilterfragspv.hpp"
#include "CompiledSPV/Headers/EnvironmentIrradiancefragspv.hpp"
#include "CompiledSPV/Headers/FXAAfragspv.hpp"
#include "CompiledSPV/Headers/FXAAComputecompspv.hpp"
#include "CompiledSPV/Headers/Debandingfragspv.hpp"
#include "CompiledSPV/Headers/ChromaticAberationfragspv.hpp"
#include "CompiledSPV/Headers/DepthPrePassfragspv.hpp"
#include "CompiledSPV/Headers/DepthPrePassAlphafragspv.hpp"
#include "CompiledSPV/Headers/ToneMappingfragspv.hpp"
#include "CompiledSPV/Headers/Bloomfragspv.hpp"
#include "CompiledSPV/Headers/Bloomcompspv.hpp"
#include "CompiledSPV/Headers/FilmicGrainfragspv.hpp"
// #include "CompiledSPV/Headers/Outlinefragspv.hpp"
#include "CompiledSPV/Headers/BRDFLUTfragspv.hpp"

#include "CompiledSPV/Headers/Textvertspv.hpp"
#include "CompiledSPV/Headers/Textfragspv.hpp"
#include "CompiledSPV/Headers/DepthOfFieldfragspv.hpp"
#include "CompiledSPV/Headers/Sharpenfragspv.hpp"
#include "CompiledSPV/Headers/SSAOfragspv.hpp"
#include "CompiledSPV/Headers/SSAOBlurfragspv.hpp"

namespace Lumos
{
    namespace Graphics
    {
#define LoadShaderEmbedded(name, vertName, fragName) shaderLibrary->AddAsset(name, SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_##vertName##vertspv.data(), spirv_##vertName##vertspv_size, spirv_##fragName##fragspv.data(), spirv_##fragName##fragspv_size)), true);
#define LoadComputeShaderEmbedded(name, compName) shaderLibrary->AddAsset(name, SharedPtr<Graphics::Shader>(Graphics::Shader::CreateCompFromEmbeddedArray(spirv_##compName##compspv.data(), spirv_##compName##compspv_size)), true);
#define LoadShaderFromFile(name, path) shaderLibrary->AddAsset(name, SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + path)), true);

        Renderer* (*Renderer::CreateFunc)() = nullptr;

        Renderer* Renderer::s_Instance = nullptr;

        void Renderer::Init(bool loadEmbeddedShaders, const std::string& engineShaderPath)
        {
            ASSERT(CreateFunc, "No Renderer Create Function");
            LUMOS_PROFILE_FUNCTION();
            s_Instance = CreateFunc();
            s_Instance->InitInternal();
            s_Instance->LoadEngineShaders(loadEmbeddedShaders, engineShaderPath);
        }

        void Renderer::Release()
        {
            delete s_Instance;

            s_Instance = nullptr;
        }

        void Renderer::LoadEngineShaders(bool loadEmbeddedShaders, const std::string& engineShaderPath)
        {
            auto shaderLibrary = Application::Get().GetAssetManager();
            if(loadEmbeddedShaders)
            {
                LINFO("Loading shaders - embedded");
                LoadShaderEmbedded("Skybox", Skybox, Skybox);
                LoadShaderEmbedded("ForwardPBR", ForwardPBR, ForwardPBR);
                LoadShaderEmbedded("ForwardPBRAnim", ForwardPBRAnim, ForwardPBR);
                LoadShaderEmbedded("Shadow", Shadow, Shadow);
                LoadShaderEmbedded("ShadowAlpha", Shadow, ShadowAlpha);
                LoadShaderEmbedded("ShadowAnim", ShadowAnim, Shadow);
                LoadShaderEmbedded("ShadowAnimAlpha", ShadowAnim, ShadowAlpha);
                LoadShaderEmbedded("Batch2DPoint", Batch2DPoint, Batch2DPoint);
                LoadShaderEmbedded("Batch2DLine", Batch2DLine, Batch2DLine);
                LoadShaderEmbedded("Batch2D", Batch2D, Batch2D);
                LoadShaderEmbedded("FinalPass", ScreenPass, ScreenPass);
                LoadShaderEmbedded("Grid", Grid, Grid);
                LoadShaderEmbedded("CreateEnvironmentMap", ScreenPass, CreateEnvironmentMap);
                LoadShaderEmbedded("EnvironmentIrradiance", ScreenPass, EnvironmentIrradiance);
                LoadShaderEmbedded("EnvironmentMipFilter", ScreenPass, EnvironmentMipFilter);
                LoadShaderEmbedded("FXAA", ScreenPass, FXAA);
                LoadShaderEmbedded("FilmicGrain", ScreenPass, FilmicGrain);
                LoadShaderEmbedded("ChromaticAberation", ScreenPass, ChromaticAberation);
                LoadShaderEmbedded("DepthPrePass", ForwardPBR, DepthPrePass);
                LoadShaderEmbedded("DepthPrePassAlpha", ForwardPBR, DepthPrePassAlpha);
                LoadShaderEmbedded("DepthPrePassAnim", ForwardPBRAnim, DepthPrePassAlpha);
                LoadShaderEmbedded("DepthPrePassAlphaAnim", ForwardPBRAnim, DepthPrePassAlpha);
                LoadShaderEmbedded("ToneMapping", ScreenPass, ToneMapping);
                LoadShaderEmbedded("Bloom", ScreenPass, Bloom);
                LoadShaderEmbedded("BRDFLUT", ScreenPass, BRDFLUT);
                LoadShaderEmbedded("Debanding", ScreenPass, Debanding);
                LoadShaderEmbedded("Text", Text, Text);
                LoadShaderEmbedded("DepthOfField", ScreenPass, DepthOfField);
                LoadShaderEmbedded("Sharpen", ScreenPass, Sharpen);
                LoadShaderEmbedded("SSAO", ScreenPass, SSAO);
                LoadShaderEmbedded("SSAOBlur", ScreenPass, SSAOBlur);
                LoadShaderEmbedded("Particle", Particle, Particle);

                if(Renderer::GetCapabilities().SupportCompute)
                {
                    LoadComputeShaderEmbedded("BloomComp", Bloom);
                    LoadComputeShaderEmbedded("FXAAComp", FXAACompute);
                }
            }
            else
            {
                LINFO("Loading shaders - files");
                LoadShaderFromFile("Skybox", "Shaders/Skybox.shader");
                LoadShaderFromFile("Shadow", "Shaders/Shadow.shader");
                LoadShaderFromFile("ShadowAnim", "Shaders/ShadowAnim.shader");
                LoadShaderFromFile("ShadowAlpha", "Shaders/ShadowAlpha.shader");
                LoadShaderFromFile("ShadowAnimAlpha", "Shaders/ShadowAnimAlpha.shader");
                LoadShaderFromFile("Batch2DPoint", "Shaders/Batch2DPoint.shader");
                LoadShaderFromFile("Batch2DLine", "Shaders/Batch2DLine.shader");
                LoadShaderFromFile("Batch2D", "Shaders/Batch2D.shader");
                LoadShaderFromFile("FinalPass", "Shaders/ScreenPass.shader");
                LoadShaderFromFile("Grid", "Shaders/Grid.shader");
                LoadShaderFromFile("CreateEnvironmentMap", "Shaders/CreateEnvironmentMap.shader");
                LoadShaderFromFile("EnvironmentIrradiance", "Shaders/EnvironmentIrradiance.shader");
                LoadShaderFromFile("EnvironmentMipFilter", "Shaders/EnvironmentMipFilter.shader");
                LoadShaderFromFile("FXAA", "Shaders/FXAA.shader");
                LoadShaderFromFile("Debanding", "Shaders/Debanding.shader");
                LoadShaderFromFile("FilmicGrain", "Shaders/FilmicGrain.shader");
                LoadShaderFromFile("ChromaticAberation", "Shaders/ChromaticAberation.shader");
                LoadShaderFromFile("DepthPrePass", "Shaders/DepthPrePass.shader");
                LoadShaderFromFile("DepthPrePassAlpha", "Shaders/DepthPrePassAlpha.shader");
                LoadShaderFromFile("ToneMapping", "Shaders/ToneMapping.shader");
                LoadShaderFromFile("Bloom", "Shaders/Bloom.shader");
                LoadShaderFromFile("DepthOfField", "Shaders/DepthOfField.shader");
                LoadShaderFromFile("BRDFLUT", "Shaders/BRDFLUT.shader");
                LoadShaderFromFile("Text", "Shaders/Text.shader");
                LoadShaderFromFile("SSAO", "Shaders/SSAO.shader");
                LoadShaderFromFile("SSAOBlur", "Shaders/SSAOBlur.shader");
                LoadShaderFromFile("Sharpen", "Shaders/Sharpen.shader");
                LoadShaderFromFile("ForwardPBR", "Shaders/ForwardPBR.shader");
                LoadShaderFromFile("ForwardPBRAnim", "Shaders/ForwardPBRAnim.shader");
                LoadShaderFromFile("Particle", "Shaders/Particle.shader");

                LoadShaderFromFile("DepthPrePassAnim", "Shaders/DepthPrePassAnim.shader");
                LoadShaderFromFile("DepthPrePassAlphaAnim", "Shaders/DepthPrePassAlphaAnim.shader")

                    if(Renderer::GetCapabilities().SupportCompute)
                {
                    LoadShaderFromFile("FXAAComp", "Shaders/FXAACompute.shader");
                    LoadShaderFromFile("BloomComp", "Shaders/BloomComp.shader");
                }
            }
        }

        GraphicsContext* Renderer::GetGraphicsContext()
        {
            return Application::Get().GetWindow()->GetGraphicsContext();
        }
        SwapChain* Renderer::GetMainSwapChain()
        {
            return Application::Get().GetWindow()->GetSwapChain();
        }

        void Renderer::DrawMesh(CommandBuffer* commandBuffer, Graphics::Pipeline* pipeline, Graphics::Mesh* mesh)
        {
            if(mesh->GetAnimVertexBuffer())
                mesh->GetAnimVertexBuffer()->Bind(commandBuffer, pipeline);
            else
                mesh->GetVertexBuffer()->Bind(commandBuffer, pipeline);
            mesh->GetIndexBuffer()->Bind(commandBuffer);

            Renderer::DrawIndexed(commandBuffer, DrawType::TRIANGLE, mesh->GetIndexBuffer()->GetCount());
            // mesh->GetVertexBuffer()->Unbind();
            // mesh->GetIndexBuffer()->Unbind();
        }
    }
}
