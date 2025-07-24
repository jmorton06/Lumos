
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
#define LoadShaderFromFile(name, path) shaderLibrary->AddAsset(name, SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile((engineShaderPath + path).c_str())), true);

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
                LoadShaderEmbedded(Str8Lit("Skybox"), Skybox, Skybox);
                LoadShaderEmbedded(Str8Lit("ForwardPBR"), ForwardPBR, ForwardPBR);
                LoadShaderEmbedded(Str8Lit("ForwardPBRAnim"), ForwardPBRAnim, ForwardPBR);
                LoadShaderEmbedded(Str8Lit("Shadow"), Shadow, Shadow);
                LoadShaderEmbedded(Str8Lit("ShadowAlpha"), Shadow, ShadowAlpha);
                LoadShaderEmbedded(Str8Lit("ShadowAnim"), ShadowAnim, Shadow);
                LoadShaderEmbedded(Str8Lit("ShadowAnimAlpha"), ShadowAnim, ShadowAlpha);
                LoadShaderEmbedded(Str8Lit("Batch2DPoint"), Batch2DPoint, Batch2DPoint);
                LoadShaderEmbedded(Str8Lit("Batch2DLine"), Batch2DLine, Batch2DLine);
                LoadShaderEmbedded(Str8Lit("Batch2D"), Batch2D, Batch2D);
                LoadShaderEmbedded(Str8Lit("FinalPass"), ScreenPass, ScreenPass);
                LoadShaderEmbedded(Str8Lit("Grid"), Grid, Grid);
                LoadShaderEmbedded(Str8Lit("CreateEnvironmentMap"), ScreenPass, CreateEnvironmentMap);
                LoadShaderEmbedded(Str8Lit("EnvironmentIrradiance"), ScreenPass, EnvironmentIrradiance);
                LoadShaderEmbedded(Str8Lit("EnvironmentMipFilter"), ScreenPass, EnvironmentMipFilter);
                LoadShaderEmbedded(Str8Lit("FXAA"), ScreenPass, FXAA);
                LoadShaderEmbedded(Str8Lit("FilmicGrain"), ScreenPass, FilmicGrain);
                LoadShaderEmbedded(Str8Lit("ChromaticAberation"), ScreenPass, ChromaticAberation);
                LoadShaderEmbedded(Str8Lit("DepthPrePass"), ForwardPBR, DepthPrePass);
                LoadShaderEmbedded(Str8Lit("DepthPrePassAlpha"), ForwardPBR, DepthPrePassAlpha);
                LoadShaderEmbedded(Str8Lit("DepthPrePassAnim"), ForwardPBRAnim, DepthPrePassAlpha);
                LoadShaderEmbedded(Str8Lit("DepthPrePassAlphaAnim"), ForwardPBRAnim, DepthPrePassAlpha);
                LoadShaderEmbedded(Str8Lit("ToneMapping"), ScreenPass, ToneMapping);
                LoadShaderEmbedded(Str8Lit("Bloom"), ScreenPass, Bloom);
                LoadShaderEmbedded(Str8Lit("BRDFLUT"), ScreenPass, BRDFLUT);
                LoadShaderEmbedded(Str8Lit("Debanding"), ScreenPass, Debanding);
                LoadShaderEmbedded(Str8Lit("Text"), Text, Text);
                LoadShaderEmbedded(Str8Lit("DepthOfField"), ScreenPass, DepthOfField);
                LoadShaderEmbedded(Str8Lit("Sharpen"), ScreenPass, Sharpen);
                LoadShaderEmbedded(Str8Lit("SSAO"), ScreenPass, SSAO);
                LoadShaderEmbedded(Str8Lit("SSAOBlur"), ScreenPass, SSAOBlur);
                LoadShaderEmbedded(Str8Lit("Particle"), Particle, Particle);

                if(Renderer::GetCapabilities().SupportCompute)
                {
                    LoadComputeShaderEmbedded(Str8Lit("BloomComp"), Bloom);
                    LoadComputeShaderEmbedded(Str8Lit("FXAAComp"), FXAACompute);
                }
            }
            else
            {
                LINFO("Loading shaders - files");
                LoadShaderFromFile(Str8Lit("Skybox"), "Shaders/Skybox.shader");
                LoadShaderFromFile(Str8Lit("Shadow"), "Shaders/Shadow.shader");
                LoadShaderFromFile(Str8Lit("ShadowAnim"), "Shaders/ShadowAnim.shader");
                LoadShaderFromFile(Str8Lit("ShadowAlpha"), "Shaders/ShadowAlpha.shader");
                LoadShaderFromFile(Str8Lit("ShadowAnimAlpha"), "Shaders/ShadowAnimAlpha.shader");
                LoadShaderFromFile(Str8Lit("Batch2DPoint"), "Shaders/Batch2DPoint.shader");
                LoadShaderFromFile(Str8Lit("Batch2DLine"), "Shaders/Batch2DLine.shader");
                LoadShaderFromFile(Str8Lit("Batch2D"), "Shaders/Batch2D.shader");
                LoadShaderFromFile(Str8Lit("FinalPass"), "Shaders/ScreenPass.shader");
                LoadShaderFromFile(Str8Lit("Grid"), "Shaders/Grid.shader");
                LoadShaderFromFile(Str8Lit("CreateEnvironmentMap"), "Shaders/CreateEnvironmentMap.shader");
                LoadShaderFromFile(Str8Lit("EnvironmentIrradiance"), "Shaders/EnvironmentIrradiance.shader");
                LoadShaderFromFile(Str8Lit("EnvironmentMipFilter"), "Shaders/EnvironmentMipFilter.shader");
                LoadShaderFromFile(Str8Lit("FXAA"), "Shaders/FXAA.shader");
                LoadShaderFromFile(Str8Lit("Debanding"), "Shaders/Debanding.shader");
                LoadShaderFromFile(Str8Lit("FilmicGrain"), "Shaders/FilmicGrain.shader");
                LoadShaderFromFile(Str8Lit("ChromaticAberation"), "Shaders/ChromaticAberation.shader");
                LoadShaderFromFile(Str8Lit("DepthPrePass"), "Shaders/DepthPrePass.shader");
                LoadShaderFromFile(Str8Lit("DepthPrePassAlpha"), "Shaders/DepthPrePassAlpha.shader");
                LoadShaderFromFile(Str8Lit("ToneMapping"), "Shaders/ToneMapping.shader");
                LoadShaderFromFile(Str8Lit("Bloom"), "Shaders/Bloom.shader");
                LoadShaderFromFile(Str8Lit("DepthOfField"), "Shaders/DepthOfField.shader");
                LoadShaderFromFile(Str8Lit("BRDFLUT"), "Shaders/BRDFLUT.shader");
                LoadShaderFromFile(Str8Lit("Text"), "Shaders/Text.shader");
                LoadShaderFromFile(Str8Lit("SSAO"), "Shaders/SSAO.shader");
                LoadShaderFromFile(Str8Lit("SSAOBlur"), "Shaders/SSAOBlur.shader");
                LoadShaderFromFile(Str8Lit("Sharpen"), "Shaders/Sharpen.shader");
                LoadShaderFromFile(Str8Lit("ForwardPBR"), "Shaders/ForwardPBR.shader");
                LoadShaderFromFile(Str8Lit("ForwardPBRAnim"), "Shaders/ForwardPBRAnim.shader");
                LoadShaderFromFile(Str8Lit("Particle"), "Shaders/Particle.shader");
                LoadShaderFromFile(Str8Lit("DepthPrePassAnim"), "Shaders/DepthPrePassAnim.shader");
                LoadShaderFromFile(Str8Lit("DepthPrePassAlphaAnim"), "Shaders/DepthPrePassAlphaAnim.shader")

                if(Renderer::GetCapabilities().SupportCompute)
                {
                    LoadShaderFromFile(Str8Lit("FXAAComp"), "Shaders/FXAACompute.shader");
                    LoadShaderFromFile(Str8Lit("BloomComp"), "Shaders/BloomComp.shader");
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
