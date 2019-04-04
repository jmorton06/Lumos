#include "LM.h"
#include "SkyboxRenderer.h"
#include "Graphics/API/Shader.h"
#include "Graphics/RenderList.h"
#include "Graphics/API/Framebuffer.h"
#include "Graphics/API/Textures/TextureDepth.h"
#include "Graphics/API/Textures/TextureCube.h"
#include "Graphics/Mesh.h"
#include "Graphics/API/Renderer.h"
#include "Graphics/API/CommandBuffer.h"
#include "Graphics/API/Swapchain.h"
#include "Graphics/API/RenderPass.h"
#include "Graphics/API/Pipeline.h"
#include "Graphics/GBuffer.h"
#include "Graphics/MeshFactory.h"
#include "Graphics/RenderManager.h"
#include "App/Scene.h"
#include "App/Application.h"
#include "Graphics/Camera/Camera.h"

namespace Lumos
{
	SkyboxRenderer::SkyboxRenderer(uint width, uint height, Texture* cubeMap) : m_UniformBuffer(nullptr), m_CubeMap(nullptr)
	{
		m_Pipeline = nullptr;
        m_CubeMap = cubeMap;
        
		SetScreenBufferSize(width, height);
        Init();
	}

	SkyboxRenderer::~SkyboxRenderer()
	{
		delete m_Shader;
		delete m_UniformBuffer;
		delete m_Skybox;
		delete m_Pipeline;
        delete m_RenderPass;
        delete[] m_VSSystemUniformBuffer;

		for(auto& commandBuffer : m_CommandBuffers)
		{
			delete commandBuffer;
		}

		for(auto& fbo : m_Framebuffers)
		{
			delete fbo;
		}

		m_Framebuffers.clear();
		m_CommandBuffers.clear();
	}

    void SkyboxRenderer::RenderScene(RenderList* renderList, Scene* scene)
	{
		m_CurrentBufferID = 0;
		if (!m_RenderTexture)
			m_CurrentBufferID = Renderer::GetSwapchain()->GetCurrentBufferId();

        Begin();
		BeginScene(scene);
		SetSystemUniforms(m_Shader);
		m_Pipeline->SetActive(m_CommandBuffers[m_CurrentBufferID]);

		Renderer::RenderMesh(m_Skybox, m_Pipeline, m_CommandBuffers[m_CurrentBufferID], 0, nullptr);

        End();
        
		if (!m_RenderTexture)
			Renderer::Present((m_CommandBuffers[Renderer::GetSwapchain()->GetCurrentBufferId()]));
	}

	enum VSSystemUniformIndices : int32
	{
		VSSystemUniformIndex_InverseProjectionViewMatrix = 0,
		VSSystemUniformIndex_Size
	};

	void SkyboxRenderer::Init()
	{
		m_Shader = Shader::CreateFromFile("Skybox", "/CoreShaders/");
		m_Skybox = MeshFactory::CreateQuad();

		// Vertex shader system uniforms
		m_VSSystemUniformBufferSize = sizeof(maths::Matrix4);
		m_VSSystemUniformBuffer = new byte[m_VSSystemUniformBufferSize];
		memset(m_VSSystemUniformBuffer, 0, m_VSSystemUniformBufferSize);
		m_VSSystemUniformBufferOffsets.resize(VSSystemUniformIndex_Size);

		// Per Scene System Uniforms
		m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_InverseProjectionViewMatrix] = 0;

		m_CommandBuffers.resize(2);

		for (auto& commandBuffer : m_CommandBuffers)
		{
			commandBuffer = graphics::api::CommandBuffer::Create();
			commandBuffer->Init(true);
		}
        
        m_RenderPass = graphics::api::RenderPass::Create();
        TextureType textureTypes[2] = { TextureType::COLOUR, TextureType::DEPTH};
        graphics::api::RenderpassInfo renderpassCI{};
        renderpassCI.attachmentCount = 2;
        renderpassCI.textureType = textureTypes;
        renderpassCI.clear = false;
        
        m_RenderPass->Init(renderpassCI);

        CreateGraphicsPipeline();
		UpdateUniformBuffer();
		CreateFramebuffers();
	}
    
    void SkyboxRenderer::Begin()
    {
        m_CommandBuffers[m_CurrentBufferID]->BeginRecording();
        
        m_RenderPass->BeginRenderpass(m_CommandBuffers[m_CurrentBufferID], maths::Vector4(0.0f), m_Framebuffers[m_CurrentBufferID], graphics::api::INLINE, m_ScreenBufferWidth, m_ScreenBufferHeight);
    }

	void SkyboxRenderer::BeginScene(Scene* scene)
	{
		auto camera = scene->GetCamera();
		auto proj = camera->GetProjectionMatrix();

		auto invViewProj = maths::Matrix4::Inverse(proj * camera->GetViewMatrix());
		memcpy(m_VSSystemUniformBuffer + m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_InverseProjectionViewMatrix], &invViewProj, sizeof(maths::Matrix4));
	}
    
    void SkyboxRenderer::End()
    {
        m_RenderPass->EndRenderpass(m_CommandBuffers[m_CurrentBufferID]);
        m_CommandBuffers[m_CurrentBufferID]->EndRecording();


		if (m_RenderTexture)
			m_CommandBuffers[m_CurrentBufferID]->Execute(true);
    }

	void SkyboxRenderer::SetSystemUniforms(Shader* shader) const
	{
		m_UniformBuffer->SetData(sizeof(UniformBufferObject), *&m_VSSystemUniformBuffer);
	}

	void SkyboxRenderer::SetRenderToGBufferTexture(bool set)
	{
		m_RenderToGBufferTexture = true;
		m_RenderTexture = Application::Instance()->GetRenderManager()->GetGBuffer()->m_ScreenTex[SCREENTEX_OFFSCREEN0];

		for (auto fbo : m_Framebuffers)
			delete fbo;
		m_Framebuffers.clear();

		CreateFramebuffers();
	}

	void SkyboxRenderer::OnResize(uint width, uint height)
	{
		delete m_Pipeline;

		for(auto fbo : m_Framebuffers)
			delete fbo;
		m_Framebuffers.clear();

		if (m_RenderToGBufferTexture)
			m_RenderTexture = Application::Instance()->GetRenderManager()->GetGBuffer()->m_ScreenTex[SCREENTEX_OFFSCREEN0];

		SetScreenBufferSize(width, height);

		CreateGraphicsPipeline();
		UpdateUniformBuffer();
		CreateFramebuffers();
	}

    void SkyboxRenderer::CreateGraphicsPipeline()
    {
        std::vector<graphics::api::DescriptorPoolInfo> poolInfo =
        {
            { graphics::api::DescriptorType::UNIFORM_BUFFER, 1 },
            { graphics::api::DescriptorType::IMAGE_SAMPLER, 1 }
        };

        std::vector<graphics::api::DescriptorLayoutInfo> layoutInfo =
        {
            { graphics::api::DescriptorType::UNIFORM_BUFFER, graphics::api::ShaderStage::VERTEX, 0 },
			{ graphics::api::DescriptorType::IMAGE_SAMPLER,graphics::api::ShaderStage::FRAGMENT , 1 }
        };

        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        std::vector<graphics::api::DescriptorLayout> descriptorLayouts;

        graphics::api::DescriptorLayout sceneDescriptorLayout{};
        sceneDescriptorLayout.count = static_cast<uint>(layoutInfo.size());
        sceneDescriptorLayout.layoutInfo = layoutInfo.data();

        descriptorLayouts.push_back(sceneDescriptorLayout);

        graphics:: api::PipelineInfo pipelineCI{};
        pipelineCI.pipelineName = "SkyRenderer";
        pipelineCI.shader = m_Shader;
        pipelineCI.vulkanRenderpass = m_RenderPass;
        pipelineCI.numVertexLayout = static_cast<uint>(attributeDescriptions.size());
        pipelineCI.descriptorLayouts = descriptorLayouts;
        pipelineCI.vertexLayout = attributeDescriptions.data();
        pipelineCI.numLayoutBindings = static_cast<uint>(poolInfo.size());
        pipelineCI.typeCounts = poolInfo.data();
        pipelineCI.strideSize = sizeof(Vertex);
        pipelineCI.numColorAttachments = 1;
        pipelineCI.wireframeEnabled = false;
        pipelineCI.cullMode = graphics::api::CullMode::NONE;
        pipelineCI.transparencyEnabled = false;
        pipelineCI.depthBiasEnabled = false;
        pipelineCI.width = m_ScreenBufferWidth;
        pipelineCI.height = m_ScreenBufferHeight;
        pipelineCI.maxObjects = 1;

        m_Pipeline = graphics::api::Pipeline::Create(pipelineCI);
    }

	void SkyboxRenderer::UpdateUniformBuffer()
	{
		if(m_UniformBuffer == nullptr)
		{
			m_UniformBuffer = graphics::api::UniformBuffer::Create();
			uint32_t bufferSize = static_cast<uint32_t>(sizeof(UniformBufferObject));
			m_UniformBuffer->Init(bufferSize, nullptr);
		}

		std::vector<graphics::api::BufferInfo> bufferInfos;

		graphics::api::BufferInfo bufferInfo = {};
		bufferInfo.buffer = m_UniformBuffer;
		bufferInfo.offset = 0;
		bufferInfo.size = sizeof(UniformBufferObject);
		bufferInfo.type = graphics::api::DescriptorType::UNIFORM_BUFFER;
		bufferInfo.binding = 0;
		bufferInfo.shaderType = ShaderType::VERTEX;
		bufferInfo.systemUniforms = true;

		bufferInfos.push_back(bufferInfo);

		std::vector<graphics::api::ImageInfo> imageInfos;


		if(m_CubeMap)
		{
			graphics::api::ImageInfo imageInfo = {};
			imageInfo.texture = { m_CubeMap };
			imageInfo.name = "u_CubeMap";
			imageInfo.binding = 1;
			imageInfo.type = TextureType::CUBE;

			imageInfos.push_back(imageInfo);
		}
		
		if(m_Pipeline != nullptr)
			m_Pipeline->GetDescriptorSet()->Update(imageInfos, bufferInfos);
	}

	void SkyboxRenderer::SetCubeMap(Texture* cubeMap)
	{
		m_CubeMap = cubeMap;
		UpdateUniformBuffer();
	}

	void SkyboxRenderer::SetRenderTarget(Texture* texture)
	{
		m_RenderTexture = texture;

		for (auto fbo : m_Framebuffers)
			delete fbo;
		m_Framebuffers.clear();

		CreateFramebuffers();
	}

	void SkyboxRenderer::CreateFramebuffers()
	{
		TextureType attachmentTypes[2];
		attachmentTypes[0] = TextureType::COLOUR;
        attachmentTypes[1] = TextureType::DEPTH;

		Texture* attachments[2];
		FramebufferInfo bufferInfo{};
		bufferInfo.width = m_ScreenBufferWidth;
		bufferInfo.height = m_ScreenBufferHeight;
		bufferInfo.attachmentCount = 2;
		bufferInfo.renderPass = m_RenderPass;
		bufferInfo.attachmentTypes = attachmentTypes;

        attachments[1] = dynamic_cast<Texture*>(Application::Instance()->GetRenderManager()->GetGBuffer()->m_DepthTexture);
        
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
}
