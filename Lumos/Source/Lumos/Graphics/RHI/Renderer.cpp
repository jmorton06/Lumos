#include "Precompiled.h"
#include "Renderer.h"

#include "Graphics/Mesh.h"
#include "Core/Application.h"
#include "Utilities/AssetManager.h"
#include "Core/OS/Window.h"

#include "CompiledSPV/Headers/Shadowvertspv.hpp"
#include "CompiledSPV/Headers/Shadowfragspv.hpp"
#include "CompiledSPV/Headers/ShadowAnimvertspv.hpp"

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
        Renderer* (*Renderer::CreateFunc)() = nullptr;

        Renderer* Renderer::s_Instance = nullptr;

        void Renderer::Init(bool loadEmbeddedShaders, const std::string& engineShaderPath)
        {
            LUMOS_ASSERT(CreateFunc, "No Renderer Create Function");
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
            auto shaderLibrary = Application::Get().GetShaderLibrary();
            if(loadEmbeddedShaders)
            {
                LUMOS_LOG_INFO("Loading shaders - embedded");
                shaderLibrary->AddResource("Skybox", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_Skyboxvertspv.data(), spirv_Skyboxvertspv_size, spirv_Skyboxfragspv.data(), spirv_Skyboxfragspv_size)));
                shaderLibrary->AddResource("ForwardPBR", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_ForwardPBRvertspv.data(), spirv_ForwardPBRvertspv_size, spirv_ForwardPBRfragspv.data(), spirv_ForwardPBRfragspv_size)));
                shaderLibrary->AddResource("Shadow", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_Shadowvertspv.data(), spirv_Shadowvertspv_size, spirv_Shadowfragspv.data(), spirv_Shadowfragspv_size)));
                shaderLibrary->AddResource("Batch2DPoint", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_Batch2DPointvertspv.data(), spirv_Batch2DPointvertspv_size, spirv_Batch2DPointfragspv.data(), spirv_Batch2DPointfragspv_size)));
                shaderLibrary->AddResource("Batch2DLine", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_Batch2DLinevertspv.data(), spirv_Batch2DLinevertspv_size, spirv_Batch2DLinefragspv.data(), spirv_Batch2DLinefragspv_size)));
                shaderLibrary->AddResource("Batch2D", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_Batch2Dvertspv.data(), spirv_Batch2Dvertspv_size, spirv_Batch2Dfragspv.data(), spirv_Batch2Dfragspv_size)));
                shaderLibrary->AddResource("FinalPass", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_ScreenPassfragspv.data(), spirv_ScreenPassfragspv_size)));
                shaderLibrary->AddResource("Grid", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_Gridvertspv.data(), spirv_Gridvertspv_size, spirv_Gridfragspv.data(), spirv_Gridfragspv_size)));
                shaderLibrary->AddResource("CreateEnvironmentMap", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_CreateEnvironmentMapfragspv.data(), spirv_CreateEnvironmentMapfragspv_size)));
                shaderLibrary->AddResource("EnvironmentIrradiance", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_EnvironmentIrradiancefragspv.data(), spirv_EnvironmentIrradiancefragspv_size)));
                shaderLibrary->AddResource("EnvironmentMipFilter", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_EnvironmentMipFilterfragspv.data(), spirv_EnvironmentMipFilterfragspv_size)));
                shaderLibrary->AddResource("FXAA", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_FXAAfragspv.data(), spirv_FXAAfragspv_size)));

                if(Renderer::GetCapabilities().SupportCompute)
                    shaderLibrary->AddResource("FXAAComp", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateCompFromEmbeddedArray(spirv_FXAAComputecompspv.data(), spirv_FXAAComputecompspv_size)));
                shaderLibrary->AddResource("FilmicGrain", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_FilmicGrainfragspv.data(), spirv_FilmicGrainfragspv_size)));
                //                shaderLibrary->AddResource("Outline", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_Outlinefragspv.data(), spirv_Outlinefragspv_size)));
                shaderLibrary->AddResource("Debanding", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_Debandingfragspv.data(), spirv_Debandingfragspv_size)));
                shaderLibrary->AddResource("ChromaticAberation", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_ChromaticAberationfragspv.data(), spirv_ChromaticAberationfragspv_size)));
                shaderLibrary->AddResource("DepthPrePass", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_ForwardPBRvertspv.data(), spirv_ForwardPBRvertspv_size, spirv_DepthPrePassfragspv.data(), spirv_DepthPrePassfragspv_size)));
                shaderLibrary->AddResource("ToneMapping", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_ToneMappingfragspv.data(), spirv_ToneMappingfragspv_size)));
                shaderLibrary->AddResource("Bloom", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_Bloomfragspv.data(), spirv_Bloomfragspv_size)));
                if(Renderer::GetCapabilities().SupportCompute)
                    shaderLibrary->AddResource("BloomComp", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateCompFromEmbeddedArray(spirv_Bloomcompspv.data(), spirv_Bloomcompspv_size)));
                shaderLibrary->AddResource("BRDFLUT", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_BRDFLUTfragspv.data(), spirv_BRDFLUTfragspv_size)));
                shaderLibrary->AddResource("Text", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_Textvertspv.data(), spirv_Textvertspv_size, spirv_Textfragspv.data(), spirv_Textfragspv_size)));
                shaderLibrary->AddResource("DepthOfField", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_DepthOfFieldfragspv.data(), spirv_DepthOfFieldfragspv_size)));
                shaderLibrary->AddResource("Sharpen", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_Sharpenfragspv.data(), spirv_Sharpenfragspv_size)));
                shaderLibrary->AddResource("SSAO", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_SSAOfragspv.data(), spirv_SSAOfragspv_size)));
                shaderLibrary->AddResource("SSAOBlur", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_SSAOBlurfragspv.data(), spirv_SSAOBlurfragspv_size)));
            }
            else
            {
                LUMOS_LOG_INFO("Loading shaders - files");
                shaderLibrary->AddResource("Skybox", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/Skybox.shader")));
                shaderLibrary->AddResource("ForwardPBR", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/ForwardPBR.shader")));
                shaderLibrary->AddResource("Shadow", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/Shadow.shader")));
                shaderLibrary->AddResource("Batch2DPoint", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/Batch2DPoint.shader")));
                shaderLibrary->AddResource("Batch2DLine", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/Batch2DLine.shader")));
                shaderLibrary->AddResource("Batch2D", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/Batch2D.shader")));
                shaderLibrary->AddResource("FinalPass", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/ScreenPass.shader")));
                shaderLibrary->AddResource("Grid", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/Grid.shader")));
                shaderLibrary->AddResource("CreateEnvironmentMap", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/CreateEnvironmentMap.shader")));
                shaderLibrary->AddResource("EnvironmentIrradiance", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/EnvironmentIrradiance.shader")));
                shaderLibrary->AddResource("EnvironmentMipFilter", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/EnvironmentMipFilter.shader")));

                shaderLibrary->AddResource("FXAA", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/FXAA.shader")));

                if(Renderer::GetCapabilities().SupportCompute)
                    shaderLibrary->AddResource("FXAAComp", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/FXAACompute.shader")));

                shaderLibrary->AddResource("Debanding", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/Debanding.shader")));
                shaderLibrary->AddResource("FilmicGrain", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/FilmicGrain.shader")));
                // shaderLibrary->AddResource("Outline", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/Outline.shader")));
                shaderLibrary->AddResource("ChromaticAberation", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/ChromaticAberation.shader")));
                shaderLibrary->AddResource("DepthPrePass", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/DepthPrePass.shader")));
                shaderLibrary->AddResource("ToneMapping", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/ToneMapping.shader")));
                shaderLibrary->AddResource("Bloom", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/Bloom.shader")));
                if(Renderer::GetCapabilities().SupportCompute)
                    shaderLibrary->AddResource("BloomComp", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/BloomComp.shader")));
                shaderLibrary->AddResource("DepthOfField", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/DepthOfField.shader")));

                shaderLibrary->AddResource("BRDFLUT", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/BRDFLUT.shader")));
                shaderLibrary->AddResource("Text", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/Text.shader")));
                shaderLibrary->AddResource("SSAO", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/SSAO.shader")));
                shaderLibrary->AddResource("SSAOBlur", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/SSAOBlur.shader")));
                shaderLibrary->AddResource("Sharpen", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(engineShaderPath + "Shaders/Sharpen.shader")));
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
            mesh->GetVertexBuffer()->Bind(commandBuffer, pipeline);
            mesh->GetIndexBuffer()->Bind(commandBuffer);
            Renderer::DrawIndexed(commandBuffer, DrawType::TRIANGLE, mesh->GetIndexBuffer()->GetCount());
            mesh->GetVertexBuffer()->Unbind();
            mesh->GetIndexBuffer()->Unbind();
        }
    }
}
