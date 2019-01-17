#include "LM.h"
#include "SkyboxRenderer.h"
#include "Graphics/API/Shader.h"
#include "Graphics/RenderList.h"
#include "Graphics/API/Framebuffer.h"
#include "Graphics/ParticleManager.h"
#include "Graphics/Light.h"
#include "Graphics/API/Textures/TextureCube.h"
#include "Graphics/Model/Model.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"
#include "Graphics/LightSetUp.h"
#include "Graphics/API/Renderer.h"
#include "Graphics/API/CommandBuffer.h"
#include "Graphics/API/Swapchain.h"
#include "Graphics/API/RenderPass.h"
#include "Graphics/API/Pipeline.h"
#include "Graphics/API/Shader.h"
#include "Graphics/GBuffer.h"
#include "Graphics/MeshFactory.h"
#include "Renderer/SceneManager.h"
#include "Renderer/Scene.h"
#include "Entity/Entity.h"

namespace Lumos
{
	SkyboxRenderer::SkyboxRenderer(uint width, uint height) : m_UniformBuffer(nullptr), m_CubeMap(nullptr)
	{
		m_Pipeline = nullptr;
		SetScreenBufferSize(width, height);
	}

	SkyboxRenderer::~SkyboxRenderer()
	{
		delete m_Shader;
		delete m_UniformBuffer;
		delete m_Skybox;
		delete m_Pipeline;
        delete[] m_VSSystemUniformBuffer;

		for(auto& commandBuffer : m_CommandBuffers)
		{
			delete commandBuffer;
		}

		m_CommandBuffers.clear();
	}

	void SkyboxRenderer::Render(graphics::api::CommandBuffer* commandBuffer, Scene* scene, int framebufferId)
	{
		BeginScene(scene);
		SetSystemUniforms(m_Shader);

		graphics::api::CommandBuffer* currentCMDBuffer = m_CommandBuffers[framebufferId];

		currentCMDBuffer->BeginRecordingSecondary(m_RenderPass, Renderer::GetSwapchain()->GetFramebuffer(framebufferId));

		currentCMDBuffer->UpdateViewport(m_ScreenBufferWidth, m_ScreenBufferHeight);

		m_Pipeline->SetActive(currentCMDBuffer);

		Renderer::RenderMesh(m_Skybox, m_Pipeline, currentCMDBuffer, 0, nullptr);

		currentCMDBuffer->EndRecording();
		currentCMDBuffer->ExecuteSecondary(commandBuffer);
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
			commandBuffer->Init(false);
		}

        CreateGraphicsPipeline();
		UpdateUniformBuffer();
	}

	void SkyboxRenderer::BeginScene(Scene* scene)
	{
		auto camera = scene->GetCamera();
		auto proj = camera->GetProjectionMatrix();

#ifdef LUMOS_RENDER_API_VULKAN
		if (graphics::Context::GetRenderAPI() == RenderAPI::VULKAN)
			proj[5] *= -1;
#endif
		auto invViewProj = maths::Matrix4::Inverse(proj * camera->GetViewMatrix());
		memcpy(m_VSSystemUniformBuffer + m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_InverseProjectionViewMatrix], &invViewProj, sizeof(maths::Matrix4));
	}

	void SkyboxRenderer::SetSystemUniforms(Shader* shader) const
	{
		m_UniformBuffer->SetData(sizeof(UniformBufferObject), *&m_VSSystemUniformBuffer);
	}

	void SkyboxRenderer::OnResize(uint width, uint height)
	{
		delete m_Pipeline;

		SetScreenBufferSize(width, height);

		CreateGraphicsPipeline();
		UpdateUniformBuffer();
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

		graphics::api::ImageInfo imageInfo = {};
		imageInfo.texture = m_CubeMap;
		imageInfo.name = "u_CubeMap";
		imageInfo.binding = 1;
		imageInfo.type = TextureType::CUBE;

		imageInfos.push_back(imageInfo);

		if(m_Pipeline != nullptr)
			m_Pipeline->GetDescriptorSet()->Update(imageInfos, bufferInfos);
	}

	void SkyboxRenderer::SetCubeMap(Texture* cubeMap)
	{
		m_CubeMap = cubeMap;
		UpdateUniformBuffer();
	}
}
