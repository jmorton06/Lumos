#include "lmpch.h"
#include "LineRenderer.h"
#include "Core/OS/Window.h"
#include "Graphics/API/Shader.h"
#include "Graphics/API/Framebuffer.h"
#include "Graphics/API/UniformBuffer.h"
#include "Graphics/API/Renderer.h"
#include "Graphics/API/CommandBuffer.h"
#include "Graphics/API/Swapchain.h"
#include "Graphics/API/RenderPass.h"
#include "Graphics/API/Pipeline.h"
#include "Graphics/API/IndexBuffer.h"
#include "Graphics/API/VertexArray.h"
#include "Graphics/API/Texture.h"
#include "Graphics/GBuffer.h"
#include "Graphics/Sprite.h"
#include "App/Scene.h"
#include "App/Application.h"
#include "Graphics/RenderManager.h"
#include "Platform/OpenGL/GLDescriptorSet.h"
#include "Graphics/Renderable2D.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Renderers/Renderer2D.h"
#include "Maths/Transform.h"
#include "Core/Profiler.h"

namespace Lumos
{
	using namespace Graphics;

    static const uint32_t MaxLines = 10000;
    static const uint32_t MaxLineVertices = MaxLines * 2;
    static const uint32_t MaxLineIndices = MaxLines * 6;
	#define MAX_BATCH_DRAW_CALLS	100
	#define RENDERER_LINE_SIZE	RENDERER2DLINE_VERTEX_SIZE * 4
	#define RENDERER_BUFFER_SIZE	RENDERER_LINE_SIZE * MaxLineVertices

	LineRenderer::LineRenderer(u32 width, u32 height, bool renderToGBuffer, bool clear) : m_RenderTexture(nullptr), m_Buffer(nullptr), m_Clear(clear)
	{
		m_RenderTexture = nullptr;
		m_BatchDrawCallIndex = 0;
        m_RenderToGBufferTexture = renderToGBuffer;
        LineIndexCount = 0;
    
        LineRenderer::SetScreenBufferSize(width, height);

        LineRenderer::Init();

        LineRenderer::SetRenderToGBufferTexture(renderToGBuffer);
	}

    LineRenderer::~LineRenderer()
    {
        delete m_IndexBuffer;
        delete m_Pipeline;
        delete m_RenderPass;
        delete m_Shader;
        delete m_UniformBuffer;

        delete[] m_VSSystemUniformBuffer;

        for (auto frameBuffer : m_Framebuffers)
            delete frameBuffer;

        for (auto& commandBuffer : m_CommandBuffers)
        {
            delete commandBuffer;
        }

        for (int i = 0; i < MAX_BATCH_DRAW_CALLS; i++)
            delete m_VertexArrays[i];

        for (int i = 0; i < MAX_BATCH_DRAW_CALLS; i++)
            delete m_SecondaryCommandBuffers[i];
    }

    void LineRenderer::Init()
    {
		LUMOS_PROFILE_FUNC;

		m_Shader = Shader::CreateFromFile("Batch2DLine", "/CoreShaders/");
		
		m_VSSystemUniformBufferSize = sizeof(Maths::Matrix4);
		m_VSSystemUniformBuffer = lmnew u8[m_VSSystemUniformBufferSize];
		
		m_RenderPass = Graphics::RenderPass::Create();
		m_UniformBuffer = Graphics::UniformBuffer::Create();
		
		AttachmentInfo textureTypes[2] =
			{
			{ TextureType::COLOUR, TextureFormat::RGBA8 }
			};
		
		Graphics::RenderpassInfo renderpassCI;
		renderpassCI.attachmentCount = 1;
		renderpassCI.textureType = textureTypes;
		renderpassCI.clear = false;
		
		m_RenderPass->Init(renderpassCI);
		
		CreateFramebuffers();
		
		m_CommandBuffers.resize(Renderer::GetSwapchain()->GetSwapchainBufferCount());
		
		for (auto& commandBuffer : m_CommandBuffers)
			{
			commandBuffer = Graphics::CommandBuffer::Create();
			commandBuffer->Init(true);
			}
		
		m_SecondaryCommandBuffers.resize(MAX_BATCH_DRAW_CALLS);
		
		for (auto& cmdBuffer : m_SecondaryCommandBuffers)
			{
			cmdBuffer = Graphics::CommandBuffer::Create();
			cmdBuffer->Init(false);
			}
		
		CreateGraphicsPipeline();
		
		uint32_t bufferSize = static_cast<uint32_t>(sizeof(UniformBufferObject));
		m_UniformBuffer->Init(bufferSize, nullptr);
		
		std::vector<Graphics::BufferInfo> bufferInfos;
		
		Graphics::BufferInfo bufferInfo;
		bufferInfo.buffer = m_UniformBuffer;
		bufferInfo.offset = 0;
		bufferInfo.size = sizeof(UniformBufferObject);
		bufferInfo.type = Graphics::DescriptorType::UNIFORM_BUFFER;
		bufferInfo.shaderType = Graphics::ShaderType::VERTEX;
		bufferInfo.systemUniforms = false;
		bufferInfo.name = "UniformBufferObject";
		bufferInfo.binding = 0;
		
		bufferInfos.push_back(bufferInfo);
		
		m_Pipeline->GetDescriptorSet()->Update(bufferInfos);
		
		Graphics::BufferLayout layout;
		layout.Push<Maths::Vector3>("POSITION"); // Position
		layout.Push<Maths::Vector4>("COLOUR"); // UV
		
		m_VertexArrays.resize(MAX_BATCH_DRAW_CALLS);
		
		for (auto& vertexArray : m_VertexArrays)
			{
			VertexBuffer* buffer = VertexBuffer::Create(BufferUsage::DYNAMIC);
			buffer->Resize(RENDERER_BUFFER_SIZE);
			buffer->SetLayout(layout);
			vertexArray = Graphics::VertexArray::Create();
			vertexArray->PushBuffer(buffer);
			}
		
		u32* indices = lmnew u32[MaxLineIndices];
		
		i32 offset = 0;
		for (i32 i = 0; i < MaxLineIndices; i++)
		{
			indices[i] = i;
		}
		
		m_IndexBuffer = IndexBuffer::Create(indices, MaxLineIndices);
		
		delete[] indices;
		
		m_ClearColour = Maths::Vector4(0.2f, 0.7f, 0.2f, 1.0f);
    }
	
	void LineRenderer::Submit(const Maths::Vector3& p1, const Maths::Vector3& p2, const Maths::Vector4& colour)
	{
        m_Lines.emplace_back(p1, p2, colour);
	}
    
    void LineRenderer::SubmitInternal(const LineInfo& info)
    {
        if (LineIndexCount >= MaxLineIndices)
            FlushAndResetLines();

        m_Buffer->vertex = info.p1;
        m_Buffer->color = info.col;
        m_Buffer++;

        m_Buffer->vertex = info.p2;
        m_Buffer->color = info.col;
        m_Buffer++;

        LineIndexCount += 2;
    }

	struct PointVertex
	{
		Maths::Vector4 pos;
		Maths::Vector4 col;
	};

	struct LineVertex
	{
		PointVertex p0;
		PointVertex p1;
	};

	struct TriVertex
	{
		PointVertex p0;
		PointVertex p1;
		PointVertex p2;
	};

	void LineRenderer::Begin()
	{
		m_CurrentBufferID = 0;
		LineIndexCount = 0;
    
        m_Lines.clear();
    }

	void LineRenderer::SetSystemUniforms(Shader* shader) const
	{
		shader->SetSystemUniformBuffer(ShaderType::VERTEX, m_VSSystemUniformBuffer, m_VSSystemUniformBufferSize, 0);

		m_UniformBuffer->SetData(sizeof(UniformBufferObject), *&m_VSSystemUniformBuffer);
	}

	void LineRenderer::BeginScene(Scene* scene)
	{
		auto camera = scene->GetCamera();
		auto projView = camera->GetProjectionMatrix() * camera->GetViewMatrix();

		memcpy(m_VSSystemUniformBuffer, &projView, sizeof(Maths::Matrix4));
	}

	void LineRenderer::Present()
	{
		Graphics::CommandBuffer* currentCMDBuffer = m_SecondaryCommandBuffers[m_BatchDrawCallIndex];

		currentCMDBuffer->BeginRecordingSecondary(m_RenderPass, m_Framebuffers[m_CurrentBufferID]);
		currentCMDBuffer->UpdateViewport(m_ScreenBufferWidth, m_ScreenBufferHeight);
		m_Pipeline->SetActive(currentCMDBuffer);

		m_VertexArrays[m_BatchDrawCallIndex]->GetBuffer()->ReleasePointer();
		m_VertexArrays[m_BatchDrawCallIndex]->Unbind();

		m_IndexBuffer->SetCount(LineIndexCount);

		std::vector<Graphics::DescriptorSet*> descriptors = { m_Pipeline->GetDescriptorSet() };

		m_VertexArrays[m_BatchDrawCallIndex]->Bind(currentCMDBuffer);
		m_IndexBuffer->Bind(currentCMDBuffer);

		Renderer::BindDescriptorSets(m_Pipeline, currentCMDBuffer, 0, descriptors);
		Renderer::DrawIndexed(currentCMDBuffer, DrawType::LINES, LineIndexCount);

		m_VertexArrays[m_BatchDrawCallIndex]->Unbind();
		m_IndexBuffer->Unbind();

		LineIndexCount = 0;

		currentCMDBuffer->EndRecording();
		currentCMDBuffer->ExecuteSecondary(m_CommandBuffers[m_CurrentBufferID]);

		m_BatchDrawCallIndex++;
	}

	void LineRenderer::End()
	{
		m_RenderPass->EndRenderpass(m_CommandBuffers[m_CurrentBufferID]);
		m_CommandBuffers[m_CurrentBufferID]->EndRecording();

		if (m_RenderTexture)
			m_CommandBuffers[m_CurrentBufferID]->Execute(true);

		if (!m_RenderTexture)
			PresentToScreen();

		m_BatchDrawCallIndex = 0;
	}

	void LineRenderer::RenderInternal(Scene* scene)
	{
        LUMOS_PROFILE_FUNC;

		BeginScene(scene);
    
        if (!m_RenderTexture)
            m_CurrentBufferID = Renderer::GetSwapchain()->GetCurrentBufferId();

        m_CommandBuffers[m_CurrentBufferID]->BeginRecording();

        m_RenderPass->BeginRenderpass(m_CommandBuffers[m_CurrentBufferID], m_ClearColour, m_Framebuffers[m_CurrentBufferID], Graphics::SECONDARY, m_ScreenBufferWidth, m_ScreenBufferHeight);

        m_VertexArrays[m_BatchDrawCallIndex]->Bind(m_CommandBuffers[m_CurrentBufferID]);
        m_Buffer = m_VertexArrays[m_BatchDrawCallIndex]->GetBuffer()->GetPointer<LineVertexData>();

		SetSystemUniforms(m_Shader);
        
        for(auto& line : m_Lines)
        {
            SubmitInternal(line);
        }
		
		Present();

		End();
	}

	void LineRenderer::OnResize(u32 width, u32 height)
	{
		delete m_Pipeline;

		for (auto fbo : m_Framebuffers)
			delete fbo;
		m_Framebuffers.clear();

		if (m_RenderToGBufferTexture)
			m_RenderTexture = Application::Instance()->GetRenderManager()->GetGBuffer()->GetTexture(SCREENTEX_OFFSCREEN0);
    
		SetScreenBufferSize(width, height);

		CreateGraphicsPipeline();

		if (m_UniformBuffer == nullptr)
		{
			m_UniformBuffer = Graphics::UniformBuffer::Create();
			uint32_t bufferSize = static_cast<uint32_t>(sizeof(UniformBufferObject));
			m_UniformBuffer->Init(bufferSize, nullptr);
		}

		std::vector<Graphics::BufferInfo> bufferInfos;

		Graphics::BufferInfo bufferInfo;
		bufferInfo.buffer = m_UniformBuffer;
		bufferInfo.offset = 0;
		bufferInfo.size = sizeof(UniformBufferObject);
		bufferInfo.type = Graphics::DescriptorType::UNIFORM_BUFFER;
		bufferInfo.shaderType = ShaderType::VERTEX;
		bufferInfo.systemUniforms = false;
		bufferInfo.name = "UniformBufferObject";
		bufferInfo.binding = 0;

		bufferInfos.push_back(bufferInfo);

		m_Pipeline->GetDescriptorSet()->Update(bufferInfos);

		CreateFramebuffers();
	}

	void LineRenderer::PresentToScreen()
	{
		Renderer::Present((m_CommandBuffers[Renderer::GetSwapchain()->GetCurrentBufferId()]));
	}

	void LineRenderer::SetScreenBufferSize(u32 width, u32 height)
	{
		if (width == 0)
		{
			width = 1;
			LUMOS_LOG_CRITICAL("Width 0");
		}
		if (height == 0)
		{
			height = 1;
			LUMOS_LOG_CRITICAL("Height 0");
		}
		m_ScreenBufferWidth = width;
		m_ScreenBufferHeight = height;
	}

	void LineRenderer::CreateGraphicsPipeline()
	{
		std::vector<Graphics::DescriptorPoolInfo> poolInfo =
		{
			{ Graphics::DescriptorType::UNIFORM_BUFFER, MAX_BATCH_DRAW_CALLS },
		};

		std::vector<Graphics::DescriptorLayoutInfo> layoutInfo =
		{
			{ Graphics::DescriptorType::UNIFORM_BUFFER, Graphics::ShaderType::VERTEX, 0 }
		};

		auto attributeDescriptions = LineVertexData::getAttributeDescriptions();

		std::vector<Graphics::DescriptorLayout> descriptorLayouts;

		Graphics::DescriptorLayout sceneDescriptorLayout;
		sceneDescriptorLayout.count = static_cast<u32>(layoutInfo.size());
		sceneDescriptorLayout.layoutInfo = layoutInfo.data();

		descriptorLayouts.push_back(sceneDescriptorLayout);

		Graphics::PipelineInfo pipelineCI;
		pipelineCI.pipelineName = "BatchLineRenderer";
		pipelineCI.shader = m_Shader;
		pipelineCI.renderpass = m_RenderPass;
		pipelineCI.numVertexLayout = static_cast<u32>(attributeDescriptions.size());
		pipelineCI.descriptorLayouts = descriptorLayouts;
		pipelineCI.vertexLayout = attributeDescriptions.data();
		pipelineCI.numLayoutBindings = static_cast<u32>(poolInfo.size());
		pipelineCI.typeCounts = poolInfo.data();
		pipelineCI.strideSize = sizeof(LineVertexData);
		pipelineCI.numColorAttachments = 1;
        pipelineCI.polygonMode = Graphics::PolygonMode::Fill;
		pipelineCI.cullMode = Graphics::CullMode::BACK;
		pipelineCI.transparencyEnabled = false;
		pipelineCI.depthBiasEnabled = true;
		pipelineCI.width = m_ScreenBufferWidth;
		pipelineCI.height = m_ScreenBufferHeight;
		pipelineCI.maxObjects = MAX_BATCH_DRAW_CALLS;
		pipelineCI.drawType = DrawType::LINES;
        pipelineCI.lineWidth = 20.0f;

		m_Pipeline = Graphics::Pipeline::Create(pipelineCI);
	}

	void LineRenderer::CreateFramebuffers()
	{
		TextureType attachmentTypes[1];
		attachmentTypes[0] = TextureType::COLOUR;

		Texture* attachments[1];
		FramebufferInfo bufferInfo{};
		bufferInfo.width = m_ScreenBufferWidth;
		bufferInfo.height = m_ScreenBufferHeight;
		bufferInfo.attachmentCount = 1;
		bufferInfo.renderPass = m_RenderPass;
		bufferInfo.attachmentTypes = attachmentTypes;

		if (m_RenderTexture)
		{
			attachments[0] = m_RenderTexture;
			bufferInfo.attachments = attachments;
			bufferInfo.screenFBO = false;
			m_Framebuffers.emplace_back(Framebuffer::Create(bufferInfo));
		}
		else
		{
			for (uint32_t i = 0; i < Renderer::GetSwapchain()->GetSwapchainBufferCount(); i++)
			{
				bufferInfo.screenFBO = true;
				attachments[0] = Renderer::GetSwapchain()->GetImage(i);
				bufferInfo.attachments = attachments;

				m_Framebuffers.emplace_back(Framebuffer::Create(bufferInfo));
			}
		}
	}
    
	void LineRenderer::SetRenderTarget(Texture* texture)
	{
		m_RenderTexture = texture;

		for (auto fbo : m_Framebuffers)
			delete fbo;
		m_Framebuffers.clear();

		CreateFramebuffers();
	}

	void LineRenderer::SetRenderToGBufferTexture(bool set)
	{
		if(set)
		{
			m_RenderToGBufferTexture = true;
			m_RenderTexture = Application::Instance()->GetRenderManager()->GetGBuffer()->GetTexture(SCREENTEX_OFFSCREEN0);
			
			for (auto fbo : m_Framebuffers)
				delete fbo;
			m_Framebuffers.clear();
			
			CreateFramebuffers();
		}
	}
    
    void LineRenderer::FlushAndResetLines()
    {
        Present();
    
        m_Lines.clear();
        
        m_VertexArrays[m_BatchDrawCallIndex]->Bind();
        m_Buffer = m_VertexArrays[m_BatchDrawCallIndex]->GetBuffer()->GetPointer<LineVertexData>();
    }
}
