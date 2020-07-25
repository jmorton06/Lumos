#include "lmpch.h"
#include "PointRenderer.h"
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
#include "Graphics/Camera/Camera.h"
#include "Scene/Scene.h"
#include "Core/Application.h"
#include "Graphics/RenderManager.h"
#include "Platform/OpenGL/GLDescriptorSet.h"
#include "Maths/Transform.h"
#include "Core/Profiler.h"

namespace Lumos
{
	using namespace Graphics;

	static const uint32_t MaxPoints = 10000;
	static const uint32_t MaxPointVertices = MaxPoints * 4;
	static const uint32_t MaxPointIndices = MaxPoints * 6;
#define MAX_BATCH_DRAW_CALLS 100
#define RENDERER_POINT_SIZE RENDERER2DPOINT_VERTEX_SIZE * 4
#define RENDERER_BUFFER_SIZE RENDERER_POINT_SIZE* MaxPointVertices

	PointRenderer::PointRenderer(u32 width, u32 height, bool clear)
		: m_IndexCount(0)
		, m_RenderTexture(nullptr)
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
		delete m_Pipeline;
		delete m_RenderPass;
		delete m_Shader;
		delete m_UniformBuffer;

		delete[] m_VSSystemUniformBuffer;

		for(auto frameBuffer : m_Framebuffers)
			delete frameBuffer;

		for(auto& commandBuffer : m_CommandBuffers)
		{
			delete commandBuffer;
		}

		for(int i = 0; i < MAX_BATCH_DRAW_CALLS; i++)
			delete m_VertexArrays[i];

		for(int i = 0; i < MAX_BATCH_DRAW_CALLS; i++)
			delete m_SecondaryCommandBuffers[i];
	}

	void PointRenderer::Init()
	{
		LUMOS_PROFILE_FUNC;

		m_Shader = Shader::CreateFromFile("Batch2DPoint", "/CoreShaders/");

		m_VSSystemUniformBufferSize = sizeof(Maths::Matrix4);
		m_VSSystemUniformBuffer = lmnew u8[m_VSSystemUniformBufferSize];

		m_RenderPass = Graphics::RenderPass::Create();
		m_UniformBuffer = Graphics::UniformBuffer::Create();

		AttachmentInfo textureTypes[2] =
			{
				{TextureType::COLOUR, TextureFormat::RGBA8}};

		Graphics::RenderpassInfo renderpassCI;
		renderpassCI.attachmentCount = 1;
		renderpassCI.textureType = textureTypes;
		renderpassCI.clear = false;

		m_RenderPass->Init(renderpassCI);

		CreateFramebuffers();

		m_CommandBuffers.resize(Renderer::GetSwapchain()->GetSwapchainBufferCount());

		for(auto& commandBuffer : m_CommandBuffers)
		{
			commandBuffer = Graphics::CommandBuffer::Create();
			commandBuffer->Init(true);
		}

		m_SecondaryCommandBuffers.resize(MAX_BATCH_DRAW_CALLS);

		for(auto& cmdBuffer : m_SecondaryCommandBuffers)
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
		layout.Push<Maths::Vector3>("POSITION");
		layout.Push<Maths::Vector4>("COLOUR");
		layout.Push<Maths::Vector2>("SIZE");
		layout.Push<Maths::Vector2>("UV");

		m_VertexArrays.resize(MAX_BATCH_DRAW_CALLS);

		for(auto& vertexArray : m_VertexArrays)
		{
			vertexArray = Graphics::VertexArray::Create();

			VertexBuffer* buffer = VertexBuffer::Create(BufferUsage::DYNAMIC);
			buffer->Resize(RENDERER_BUFFER_SIZE);
			buffer->SetLayout(layout);
			vertexArray->PushBuffer(buffer);
		}

		u32* indices = lmnew u32[MaxPointIndices];

		i32 offset = 0;
		for(i32 i = 0; i < MaxPointIndices; i += 6)
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
	}

	void PointRenderer::Submit(const Maths::Vector3& p1, float size, const Maths::Vector4& colour)
	{
		m_Points.emplace_back(p1, size, colour);
	}

	void PointRenderer::SubmitInternal(PointInfo& pointInfo)
	{
		if(PointIndexCount >= MaxPointIndices)
			FlushAndResetPoints();

		Maths::Vector3 right = pointInfo.size * m_Camera->GetRightDirection();
		Maths::Vector3 up = pointInfo.size * m_Camera->GetUpDirection();

		m_Buffer->vertex = pointInfo.p1 - right - up; // + Maths::Vector3(-pointInfo.size, -pointInfo.size, 0.0f));
		m_Buffer->color = pointInfo.col;
		m_Buffer->size = {pointInfo.size, 0.0f};
		m_Buffer->uv = {-1.0f, -1.0f};
		m_Buffer++;

		m_Buffer->vertex = pointInfo.p1 + right - up; //(pointInfo.p1 + Maths::Vector3(pointInfo.size, -pointInfo.size, 0.0f));
		m_Buffer->color = pointInfo.col;
		m_Buffer->size = {pointInfo.size, 0.0f};
		m_Buffer->uv = {1.0f, -1.0f};
		m_Buffer++;

		m_Buffer->vertex = pointInfo.p1 + right + up; //(pointInfo.p1 + Maths::Vector3(pointInfo.size, pointInfo.size, 0.0f));
		m_Buffer->color = pointInfo.col;
		m_Buffer->size = {pointInfo.size, 0.0f};
		m_Buffer->uv = {1.0f, 1.0f};
		m_Buffer++;

		m_Buffer->vertex = pointInfo.p1 - right + up; // (pointInfo.p1 + Maths::Vector3(-pointInfo.size, pointInfo.size, 0.0f));
		m_Buffer->color = pointInfo.col;
		m_Buffer->size = {pointInfo.size, 0.0f};
		m_Buffer->uv = {-1.0f, 1.0f};
		m_Buffer++;

		PointIndexCount += 6;
	}

	void PointRenderer::Begin()
	{
		m_CurrentBufferID = 0;
		PointIndexCount = 0;
		m_Points.clear();
	}

	void PointRenderer::SetSystemUniforms(Shader* shader) const
	{
		shader->SetSystemUniformBuffer(ShaderType::VERTEX, m_VSSystemUniformBuffer, m_VSSystemUniformBufferSize, 0);

		m_UniformBuffer->SetData(sizeof(UniformBufferObject), *&m_VSSystemUniformBuffer);
	}

	void PointRenderer::BeginScene(Scene* scene, Camera* overrideCamera)
	{
		auto& registry = scene->GetRegistry();

		if(overrideCamera)
			m_Camera = overrideCamera;
		else
		{
			auto cameraView = registry.view<Camera>();
			if(!cameraView.empty())
			{
				m_Camera = &registry.get<Camera>(cameraView.front());
			}
		}

		if(!m_Camera)
			return;

		auto projView = m_Camera->GetProjectionMatrix() * m_Camera->GetViewMatrix();
		m_ProjectionMatrix = m_Camera->GetProjectionMatrix();

		memcpy(m_VSSystemUniformBuffer, &projView, sizeof(Maths::Matrix4));
	}

	void PointRenderer::Present()
	{
		UpdateDesciptorSet();

		Graphics::CommandBuffer* currentCMDBuffer = m_SecondaryCommandBuffers[m_BatchDrawCallIndex];

		currentCMDBuffer->BeginRecordingSecondary(m_RenderPass, m_Framebuffers[m_CurrentBufferID]);
		currentCMDBuffer->UpdateViewport(m_ScreenBufferWidth, m_ScreenBufferHeight);
		m_Pipeline->SetActive(currentCMDBuffer);

		m_VertexArrays[m_BatchDrawCallIndex]->GetBuffer()->ReleasePointer();
		m_VertexArrays[m_BatchDrawCallIndex]->Unbind();

		m_IndexBuffer->SetCount(PointIndexCount);

		std::vector<Graphics::DescriptorSet*> descriptors = {m_Pipeline->GetDescriptorSet()};

		m_VertexArrays[m_BatchDrawCallIndex]->Bind(currentCMDBuffer);
		m_IndexBuffer->Bind(currentCMDBuffer);

		Renderer::BindDescriptorSets(m_Pipeline, currentCMDBuffer, 0, descriptors);
		Renderer::DrawIndexed(currentCMDBuffer, DrawType::TRIANGLE, PointIndexCount);

		m_VertexArrays[m_BatchDrawCallIndex]->Unbind();
		m_IndexBuffer->Unbind();

		PointIndexCount = 0;

		currentCMDBuffer->EndRecording();
		currentCMDBuffer->ExecuteSecondary(m_CommandBuffers[m_CurrentBufferID]);

		m_BatchDrawCallIndex++;
	}

	void PointRenderer::End()
	{
		m_RenderPass->EndRenderpass(m_CommandBuffers[m_CurrentBufferID]);
		m_CommandBuffers[m_CurrentBufferID]->EndRecording();

		if(m_RenderTexture)
			m_CommandBuffers[m_CurrentBufferID]->Execute(true);

		if(!m_RenderTexture)
			PresentToScreen();

		m_BatchDrawCallIndex = 0;
	}

	void PointRenderer::RenderInternal(Scene* scene, Camera* overrideCamera)
	{
		LUMOS_PROFILE_FUNC;

		BeginScene(scene, overrideCamera);

		if(!m_RenderTexture)
			m_CurrentBufferID = Renderer::GetSwapchain()->GetCurrentBufferId();

		m_CommandBuffers[m_CurrentBufferID]->BeginRecording();

		m_RenderPass->BeginRenderpass(m_CommandBuffers[m_CurrentBufferID], m_ClearColour, m_Framebuffers[m_CurrentBufferID], Graphics::SECONDARY, m_ScreenBufferWidth, m_ScreenBufferHeight);

		m_VertexArrays[m_BatchDrawCallIndex]->Bind(m_CommandBuffers[m_CurrentBufferID]);
		m_Buffer = m_VertexArrays[m_BatchDrawCallIndex]->GetBuffer()->GetPointer<PointVertexData>();

		SetSystemUniforms(m_Shader);

		for(auto& point : m_Points)
			SubmitInternal(point);

		Present();

		End();
	}

	void PointRenderer::OnResize(u32 width, u32 height)
	{
		for(auto fbo : m_Framebuffers)
			delete fbo;
		m_Framebuffers.clear();

		SetScreenBufferSize(width, height);

		CreateFramebuffers();
	}

	void PointRenderer::PresentToScreen()
	{
		Renderer::Present((m_CommandBuffers[Renderer::GetSwapchain()->GetCurrentBufferId()]));
	}

	void PointRenderer::SetScreenBufferSize(u32 width, u32 height)
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
		std::vector<Graphics::DescriptorPoolInfo> poolInfo =
			{
				{Graphics::DescriptorType::UNIFORM_BUFFER, MAX_BATCH_DRAW_CALLS},
			};

		std::vector<Graphics::DescriptorLayoutInfo> layoutInfo =
			{
				{Graphics::DescriptorType::UNIFORM_BUFFER, Graphics::ShaderType::VERTEX, 0}};

		auto attributeDescriptions = PointVertexData::getAttributeDescriptions();

		std::vector<Graphics::DescriptorLayout> descriptorLayouts;

		Graphics::DescriptorLayout sceneDescriptorLayout;
		sceneDescriptorLayout.count = static_cast<u32>(layoutInfo.size());
		sceneDescriptorLayout.layoutInfo = layoutInfo.data();

		descriptorLayouts.push_back(sceneDescriptorLayout);

		Graphics::PipelineInfo PipelineCI;
		PipelineCI.pipelineName = "BatchPointRenderer";
		PipelineCI.shader = m_Shader;
		PipelineCI.renderpass = m_RenderPass;
		PipelineCI.numVertexLayout = static_cast<u32>(attributeDescriptions.size());
		PipelineCI.descriptorLayouts = descriptorLayouts;
		PipelineCI.vertexLayout = attributeDescriptions.data();
		PipelineCI.numLayoutBindings = static_cast<u32>(poolInfo.size());
		PipelineCI.typeCounts = poolInfo.data();
		PipelineCI.strideSize = sizeof(PointVertexData);
		PipelineCI.numColorAttachments = 1;
		PipelineCI.polygonMode = Graphics::PolygonMode::Fill;
		PipelineCI.cullMode = Graphics::CullMode::NONE;
		PipelineCI.transparencyEnabled = true;
		PipelineCI.depthBiasEnabled = true;
		PipelineCI.maxObjects = MAX_BATCH_DRAW_CALLS;
		PipelineCI.drawType = DrawType::TRIANGLE;
		PipelineCI.lineWidth = 1.0f;

		m_Pipeline = Graphics::Pipeline::Create(PipelineCI);
	}

	void PointRenderer::CreateFramebuffers()
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

		if(m_RenderTexture)
		{
			attachments[0] = m_RenderTexture;
			bufferInfo.attachments = attachments;
			bufferInfo.screenFBO = false;
			m_Framebuffers.emplace_back(Framebuffer::Create(bufferInfo));
		}
		else
		{
			for(uint32_t i = 0; i < Renderer::GetSwapchain()->GetSwapchainBufferCount(); i++)
			{
				bufferInfo.screenFBO = true;
				attachments[0] = Renderer::GetSwapchain()->GetImage(i);
				bufferInfo.attachments = attachments;

				m_Framebuffers.emplace_back(Framebuffer::Create(bufferInfo));
			}
		}
	}

	void PointRenderer::UpdateDesciptorSet() const
	{
	}

	void PointRenderer::SetRenderTarget(Graphics::Texture* texture, bool rebuildFramebuffer)
	{
		m_RenderTexture = texture;

		for(auto fbo : m_Framebuffers)
			delete fbo;
		m_Framebuffers.clear();

		CreateFramebuffers();
	}

	void PointRenderer::FlushAndResetPoints()
	{
		Present();

		m_Points.clear();

		m_VertexArrays[m_BatchDrawCallIndex]->Bind();
		m_Buffer = m_VertexArrays[m_BatchDrawCallIndex]->GetBuffer()->GetPointer<PointVertexData>();
	}
}
