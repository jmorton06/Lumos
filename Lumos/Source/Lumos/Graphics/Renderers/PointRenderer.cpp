#include "Precompiled.h"
#include "PointRenderer.h"
#include "Core/OS/Window.h"
#include "Graphics/RHI/Shader.h"
#include "Graphics/RHI/Framebuffer.h"
#include "Graphics/RHI/UniformBuffer.h"
#include "Graphics/RHI/Renderer.h"
#include "Graphics/RHI/CommandBuffer.h"
#include "Graphics/RHI/SwapChain.h"
#include "Graphics/RHI/RenderPass.h"
#include "Graphics/RHI/Pipeline.h"
#include "Graphics/RHI/IndexBuffer.h"
#include "Graphics/RHI/VertexBuffer.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/RHI/GraphicsContext.h"
#include "Graphics/GBuffer.h"
#include "Graphics/Sprite.h"
#include "Graphics/Camera/Camera.h"
#include "Scene/Scene.h"
#include "Core/Application.h"
#include "Platform/OpenGL/GLDescriptorSet.h"
#include "Maths/Transform.h"

#include "CompiledSPV/Headers/Batch2DPointvertspv.hpp"
#include "CompiledSPV/Headers/Batch2DPointfragspv.hpp"

namespace Lumos
{
    using namespace Graphics;

    static const uint32_t MaxPoints = 10000;
    static const uint32_t MaxPointVertices = MaxPoints * 4;
    static const uint32_t MaxPointIndices = MaxPoints * 6;
    static const uint32_t MAX_BATCH_DRAW_CALLS = 100;
    static const uint32_t RENDERER_POINT_SIZE = sizeof(PointVertexData) * 4;
    static const uint32_t RENDERER_BUFFER_SIZE = RENDERER_POINT_SIZE * MaxPointVertices;

    PointRenderer::PointRenderer(uint32_t width, uint32_t height, bool clear)
        : m_IndexCount(0)
        , m_Buffer(nullptr)
        , m_Clear(clear)
    {
        m_RenderTexture = nullptr;
        m_BatchDrawCallIndex = 0;

        PointRenderer::SetScreenBufferSize(width, height);

        PointRenderer::Init();
    }

    PointRenderer::~PointRenderer()
    {
        delete m_IndexBuffer;

        for(int i = 0; i < MAX_BATCH_DRAW_CALLS; i++)
            delete m_VertexBuffers[i];
    }

    void PointRenderer::Init()
    {
        LUMOS_PROFILE_FUNCTION();

        // m_Shader = Application::Get().GetShaderLibrary()->GetResource("//CoreShaders/Batch2DPoint.shader");
        m_Shader = Graphics::Shader::CreateFromEmbeddedArray(spirv_Batch2DPointvertspv.data(), spirv_Batch2DPointvertspv_size, spirv_Batch2DPointfragspv.data(), spirv_Batch2DPointfragspv_size);

        Graphics::DescriptorDesc descriptorDesc {};
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader = m_Shader.get();
        m_DescriptorSet.resize(1);
        m_DescriptorSet[0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_VertexBuffers.resize(MAX_BATCH_DRAW_CALLS);

        for(auto& vertexBuffer : m_VertexBuffers)
        {
            vertexBuffer = Graphics::VertexBuffer::Create(BufferUsage::DYNAMIC);
            vertexBuffer->Resize(RENDERER_BUFFER_SIZE);
        }

        uint32_t* indices = new uint32_t[MaxPointIndices];

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

        m_IndexBuffer = IndexBuffer::Create(indices, MaxPointIndices);

        delete[] indices;

        m_ClearColour = Maths::Vector4(0.2f, 0.7f, 0.2f, 1.0f);
        m_CurrentDescriptorSets.resize(1);
    }

    void PointRenderer::Submit(const Maths::Vector3& p1, float size, const Maths::Vector4& colour)
    {
        m_Points.emplace_back(p1, size, colour);
    }

    void PointRenderer::SubmitInternal(PointInfo& pointInfo)
    {
        if(PointIndexCount >= MaxPointIndices)
            FlushAndResetPoints();

        Maths::Vector3 right = pointInfo.size * m_CameraTransform->GetRightDirection();
        Maths::Vector3 up = pointInfo.size * m_CameraTransform->GetUpDirection();

        m_Buffer->vertex = pointInfo.p1 - right - up; // + Maths::Vector3(-pointInfo.size, -pointInfo.size, 0.0f));
        m_Buffer->colour = pointInfo.col;
        m_Buffer->size = { pointInfo.size, 0.0f };
        m_Buffer->uv = { -1.0f, -1.0f };
        m_Buffer++;

        m_Buffer->vertex = pointInfo.p1 + right - up; //(pointInfo.p1 + Maths::Vector3(pointInfo.size, -pointInfo.size, 0.0f));
        m_Buffer->colour = pointInfo.col;
        m_Buffer->size = { pointInfo.size, 0.0f };
        m_Buffer->uv = { 1.0f, -1.0f };
        m_Buffer++;

        m_Buffer->vertex = pointInfo.p1 + right + up; //(pointInfo.p1 + Maths::Vector3(pointInfo.size, pointInfo.size, 0.0f));
        m_Buffer->colour = pointInfo.col;
        m_Buffer->size = { pointInfo.size, 0.0f };
        m_Buffer->uv = { 1.0f, 1.0f };
        m_Buffer++;

        m_Buffer->vertex = pointInfo.p1 - right + up; // (pointInfo.p1 + Maths::Vector3(-pointInfo.size, pointInfo.size, 0.0f));
        m_Buffer->colour = pointInfo.col;
        m_Buffer->size = { pointInfo.size, 0.0f };
        m_Buffer->uv = { -1.0f, 1.0f };
        m_Buffer++;

        PointIndexCount += 6;
    }

    void PointRenderer::Begin()
    {
        m_CurrentBufferID = 0;
        PointIndexCount = 0;
        m_Points.clear();
    }

    void PointRenderer::BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform)
    {
        auto& registry = scene->GetRegistry();

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

        auto projView = m_Camera->GetProjectionMatrix() * m_CameraTransform->GetWorldMatrix().Inverse();
        m_DescriptorSet[0]->SetUniform("UniformBufferObject", "projView", &projView);
        m_DescriptorSet[0]->Update();
    }

    void PointRenderer::Present()
    {
        UpdateDesciptorSet();

        Graphics::CommandBuffer* currentCMDBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();

        CreateGraphicsPipeline();

        m_Pipeline->Bind(currentCMDBuffer);

        m_VertexBuffers[m_BatchDrawCallIndex]->ReleasePointer();
        m_VertexBuffers[m_BatchDrawCallIndex]->Unbind();

        m_IndexBuffer->SetCount(PointIndexCount);

        m_CurrentDescriptorSets[0] = m_DescriptorSet[0].get();

        m_VertexBuffers[m_BatchDrawCallIndex]->Bind(currentCMDBuffer, m_Pipeline.get());
        m_IndexBuffer->Bind(currentCMDBuffer);

        Renderer::BindDescriptorSets(m_Pipeline.get(), currentCMDBuffer, 0, m_CurrentDescriptorSets);
        Renderer::DrawIndexed(currentCMDBuffer, DrawType::TRIANGLE, PointIndexCount);

        m_VertexBuffers[m_BatchDrawCallIndex]->Unbind();
        m_IndexBuffer->Unbind();

        m_Pipeline->End(currentCMDBuffer);

        PointIndexCount = 0;

        m_BatchDrawCallIndex++;
    }

    void PointRenderer::End()
    {
        m_BatchDrawCallIndex = 0;
    }

    void PointRenderer::RenderInternal()
    {
        LUMOS_PROFILE_FUNCTION();
        if(m_Points.empty())
            return;

        if(!m_RenderTexture)
            m_CurrentBufferID = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

        m_VertexBuffers[m_BatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), m_Pipeline.get());
        m_Buffer = m_VertexBuffers[m_BatchDrawCallIndex]->GetPointer<PointVertexData>();

        for(auto& point : m_Points)
            SubmitInternal(point);

        Present();

        End();
    }

    void PointRenderer::OnResize(uint32_t width, uint32_t height)
    {
        m_Framebuffers.clear();

        SetScreenBufferSize(width, height);

        CreateFramebuffers();
    }

    void PointRenderer::PresentToScreen()
    {
        //Renderer::Present((m_CommandBuffers[Renderer::GetMainSwapChain()->GetCurrentBufferIndex()].get()));
    }

    void PointRenderer::SetScreenBufferSize(uint32_t width, uint32_t height)
    {
        if(width == 0)
        {
            width = 1;
            LUMOS_LOG_CRITICAL("Width 0");
        }
        if(height == 0)
        {
            height = 1;
            LUMOS_LOG_CRITICAL("Height 0");
        }
        m_ScreenBufferWidth = width;
        m_ScreenBufferHeight = height;
    }

    void PointRenderer::CreateGraphicsPipeline()
    {
        Graphics::PipelineDesc pipelineDesc;
        pipelineDesc.shader = m_Shader;
        pipelineDesc.polygonMode = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode = Graphics::CullMode::NONE;
        pipelineDesc.transparencyEnabled = true;
        pipelineDesc.drawType = DrawType::TRIANGLE;
        pipelineDesc.blendMode = BlendMode::SrcAlphaOneMinusSrcAlpha;

        if(m_RenderTexture)
            pipelineDesc.colourTargets[0] = m_RenderTexture;
        else
            pipelineDesc.swapchainTarget = true;

        m_Pipeline = Graphics::Pipeline::Get(pipelineDesc);
    }

    void PointRenderer::CreateFramebuffers()
    {
    }

    void PointRenderer::UpdateDesciptorSet() const
    {
    }

    void PointRenderer::SetRenderTarget(Graphics::Texture* texture, bool rebuildFramebuffer)
    {
        m_RenderTexture = texture;
        if(rebuildFramebuffer)
        {
            m_Framebuffers.clear();
        }
    }

    void PointRenderer::FlushAndResetPoints()
    {
        Present();

        m_Points.clear();

        m_VertexBuffers[m_BatchDrawCallIndex]->Bind(nullptr, nullptr);
        m_Buffer = m_VertexBuffers[m_BatchDrawCallIndex]->GetPointer<PointVertexData>();
    }
}
