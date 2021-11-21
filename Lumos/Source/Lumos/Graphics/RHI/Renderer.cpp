#include "Precompiled.h"
#include "Renderer.h"

#include "Graphics/Mesh.h"
#include "Core/Application.h"

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

namespace Lumos
{
    namespace Graphics
    {
        Renderer* (*Renderer::CreateFunc)() = nullptr;

        Renderer* Renderer::s_Instance = nullptr;

        void Renderer::Init()
        {
            LUMOS_ASSERT(CreateFunc, "No Renderer Create Function");
            LUMOS_PROFILE_FUNCTION();
            s_Instance = CreateFunc();
            s_Instance->InitInternal();
            s_Instance->LoadEngineShaders();
        }

        void Renderer::Release()
        {
            delete s_Instance;

            s_Instance = nullptr;
        }

        void Renderer::LoadEngineShaders()
        {
            const bool LoadEmbeddedShaders = false;
            auto shaderLibrary = Application::Get().GetShaderLibrary();
            if(LoadEmbeddedShaders)
            {
                shaderLibrary->AddResource("Skybox", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_Skyboxvertspv.data(), spirv_Skyboxvertspv_size, spirv_Skyboxfragspv.data(), spirv_Skyboxfragspv_size)));
                shaderLibrary->AddResource("ForwardPBR", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_ForwardPBRvertspv.data(), spirv_ForwardPBRvertspv_size, spirv_ForwardPBRfragspv.data(), spirv_ForwardPBRfragspv_size)));
                shaderLibrary->AddResource("Shadow", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_Shadowvertspv.data(), spirv_Shadowvertspv_size, spirv_Shadowfragspv.data(), spirv_Shadowfragspv_size)));
                shaderLibrary->AddResource("Batch2DPoint", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_Batch2DPointvertspv.data(), spirv_Batch2DPointvertspv_size, spirv_Batch2DPointfragspv.data(), spirv_Batch2DPointfragspv_size)));
                shaderLibrary->AddResource("Batch2DLine", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_Batch2DLinevertspv.data(), spirv_Batch2DLinevertspv_size, spirv_Batch2DLinefragspv.data(), spirv_Batch2DLinefragspv_size)));
                shaderLibrary->AddResource("Batch2D", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_Batch2Dvertspv.data(), spirv_Batch2Dvertspv_size, spirv_Batch2Dfragspv.data(), spirv_Batch2Dfragspv_size)));
                shaderLibrary->AddResource("FinalPass", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_ScreenPassvertspv.data(), spirv_ScreenPassvertspv_size, spirv_ScreenPassfragspv.data(), spirv_ScreenPassfragspv_size)));
                shaderLibrary->AddResource("Grid", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromEmbeddedArray(spirv_Gridvertspv.data(), spirv_Gridvertspv_size, spirv_Gridfragspv.data(), spirv_Gridfragspv_size)));
            }
            else
            {
                shaderLibrary->AddResource("Skybox", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile("//CoreShaders/Skybox.shader")));
                shaderLibrary->AddResource("ForwardPBR", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile("//CoreShaders/ForwardPBR.shader")));
                shaderLibrary->AddResource("Shadow", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile("//CoreShaders/Shadow.shader")));
                shaderLibrary->AddResource("Batch2DPoint", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile("//CoreShaders/Batch2DPoint.shader")));
                shaderLibrary->AddResource("Batch2DLine", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile("//CoreShaders/Batch2DLine.shader")));
                shaderLibrary->AddResource("Batch2D", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile("//CoreShaders/Batch2D.shader")));
                shaderLibrary->AddResource("FinalPass", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile("//CoreShaders/ScreenPass.shader")));
                shaderLibrary->AddResource("Grid", SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile("//CoreShaders/Grid.shader")));
            }
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
