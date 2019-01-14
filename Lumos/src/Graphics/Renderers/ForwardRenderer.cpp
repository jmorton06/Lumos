#include "JM.h"
#include "ForwardRenderer.h"
#include "Graphics/API/Shader.h"
#include "Graphics/RenderList.h"
#include "Graphics/API/FrameBuffer.h"
#include "Graphics/ParticleManager.h"
#include "Graphics/Light.h"
#include "Graphics/API/Textures/TextureCube.h"
#include "Graphics/API/Textures/TextureDepth.h"
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
#include "Renderer/SceneManager.h"
#include "Renderer/Scene.h"
#include "Entity/Entity.h"
#include "App/Application.h"

#ifdef JM_RENDER_API_VULKAN
#include "Platform/GraphicsAPI/Vulkan/VKRenderer.h"
#endif

int i = 0;

namespace jm
{
	ForwardRenderer::ForwardRenderer(uint width, uint height)
	{
		SetScreenBufferSize(width, height);
		ForwardRenderer::Init();
	}

	ForwardRenderer::~ForwardRenderer()
	{
		delete m_Shader;
		delete m_FBO;
		delete m_DefaultTexture;
		delete m_UniformBuffer;
		delete m_ModelUniformBuffer;
		delete m_RenderPass;
		delete m_DepthTexture;
	}

	void ForwardRenderer::RenderScene(RenderList* renderList, Scene* scene)
	{
		for (i = 0; i < commandBuffers.size(); i++)
		{
			Begin();
			BeginScene(scene->GetCamera());

			renderList->RenderOpaqueObjects([&](Entity* obj)
			{
				if (obj != nullptr)
				{
					auto* model = obj->GetComponent<ModelComponent>();
					if (model && model->m_Model)
					{
						for (auto& mesh : model->m_Model->GetMeshs())
						{
							TextureMatrixComponent* textureMatrixTransform = obj->GetComponent<TextureMatrixComponent>();
							maths::Matrix4 textureMatrix;
							if (textureMatrixTransform)
								textureMatrix = textureMatrixTransform->m_TextureMatrix;
							else
								textureMatrix = maths::Matrix4();
							SubmitMesh(mesh.get(), obj->GetComponent<TransformComponent>()->m_WorldSpaceTransform, textureMatrix);
						}
					}
				}
			});

			SetSystemUniforms(m_Shader);

			Present();

			EndScene();
			End();
		}
#ifdef JM_RENDER_API_VULKAN
		if (graphics::Context::GetRenderAPI() == RenderAPI::VULKAN)
			graphics::VKRenderer::GetRenderer()->Present(static_cast<graphics::VKCommandBuffer*>(commandBuffers[graphics::VKRenderer::GetRenderer()->GetSwapchain()->GetCurrentBufferId()]));
#endif
	}

	enum VSSystemUniformIndices : int32
	{
		VSSystemUniformIndex_ProjectionMatrix = 0,
		VSSystemUniformIndex_ViewMatrix = 1,
		VSSystemUniformIndex_ModelMatrix = 2,
		VSSystemUniformIndex_TextureMatrix = 3,
		VSSystemUniformIndex_Size
	};

	enum PSSystemUniformIndices : int32
	{
		PSSystemUniformIndex_Lights = 0,
		PSSystemUniformIndex_Size
	};

	void ForwardRenderer::Init()
	{
		m_CommandQueue.reserve(1000);

		//
		// Vertex shader system uniforms
		//
		m_VSSystemUniformBufferSize = sizeof(maths::Matrix4) + sizeof(maths::Matrix4) + sizeof(maths::Matrix4) + sizeof(maths::Matrix4);
		m_VSSystemUniformBuffer = new byte[m_VSSystemUniformBufferSize];
		memset(m_VSSystemUniformBuffer, 0, m_VSSystemUniformBufferSize);
		m_VSSystemUniformBufferOffsets.resize(VSSystemUniformIndex_Size);

		// Per Scene System Uniforms
		m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionMatrix] = 0;
		m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ViewMatrix] = m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionMatrix] + sizeof(maths::Matrix4);
		m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ModelMatrix] = m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ViewMatrix] + sizeof(maths::Matrix4);
		m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_TextureMatrix] = m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ModelMatrix] + sizeof(maths::Matrix4);

		// Pixel/fragment shader system uniforms
		m_PSSystemUniformBufferSize = sizeof(Light);
		m_PSSystemUniformBuffer = new byte[m_PSSystemUniformBufferSize];
		memset(m_PSSystemUniformBuffer, 0, m_PSSystemUniformBufferSize);
		m_PSSystemUniformBufferOffsets.resize(PSSystemUniformIndex_Size);

		// Per Scene System Uniforms
		m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_Lights] = 0;

		m_RenderPass = graphics::api::RenderPass::Create();
		m_DepthTexture = TextureDepth::Create(m_ScreenBufferWidth, m_ScreenBufferHeight);
		m_UniformBuffer = graphics::api::UniformBuffer::Create();
		m_ModelUniformBuffer = graphics::api::UniformBuffer::Create();

		TextureType textureTypes[2] = { TextureType::COLOUR , TextureType::DEPTH };
		graphics::api::RenderpassInfo renderpassCI{};
		renderpassCI.attachmentCount = 2;
		renderpassCI.textureType = textureTypes;

		m_RenderPass->Init(renderpassCI);

		//Renderer::GetSwapchain()->Init(m_DepthTexture, m_Renderpass);

		m_FBO = nullptr;

		commandBuffers.resize(2);

		for (auto& commandBuffer : commandBuffers)
		{
			commandBuffer = graphics::api::CommandBuffer::Create();
			commandBuffer->Init(true);
		}

        CreateGraphicsPipeline();

		uint32_t bufferSize = static_cast<uint32_t>(sizeof(UniformBufferObject));
		m_UniformBuffer->Init(bufferSize, nullptr);

		const size_t minUboAlignment = graphics::Context::GetContext()->GetMinUniformBufferOffsetAlignment();

		dynamicAlignment = sizeof(jm::maths::Matrix4);
		if (minUboAlignment > 0)
		{
			dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
		}

		uint32_t bufferSize2 = static_cast<uint32_t>(MAX_OBJECTS * dynamicAlignment);

		uboDataDynamic.model = static_cast<maths::Matrix4*>(AlignedAlloc(bufferSize2, dynamicAlignment));

		m_ModelUniformBuffer->Init(bufferSize2, nullptr);

		std::vector<graphics::api::BufferInfo> bufferInfos;

		graphics::api::BufferInfo bufferInfo = {};
		bufferInfo.buffer = m_UniformBuffer;
		bufferInfo.offset = 0;
		bufferInfo.size = sizeof(UniformBufferObject);
		bufferInfo.type = graphics::api::DescriptorType::UNIFORM_BUFFER;
		bufferInfo.binding = 0;

		graphics::api::BufferInfo bufferInfo2 = {};
		bufferInfo2.buffer = m_ModelUniformBuffer;
		bufferInfo2.offset = 0;
		bufferInfo2.size = sizeof(UniformBufferModel);
		bufferInfo2.type = graphics::api::DescriptorType::UNIFORM_BUFFER_DYNAMIC;
		bufferInfo2.binding = 1;

		bufferInfos.push_back(bufferInfo);
		bufferInfos.push_back(bufferInfo2);

		m_GraphicsPipeline->GetDescriptorSet()->Update(bufferInfos);

		m_ClearColour = maths::Vector4(0.8f, 0.8f, 0.8f, 1.0f);
		m_SkyboxRenderer = nullptr;
	}

	void ForwardRenderer::Begin()
	{
		m_CommandQueue.clear();
		m_SystemUniforms.clear();

        commandBuffers[i]->BeginRecording();

		m_RenderPass->BeginRenderpass(commandBuffers[i], m_ClearColour, Renderer::GetSwapchain()->GetFramebuffer(i), graphics::api::SECONDARY, m_ScreenBufferWidth, m_ScreenBufferHeight);
		m_Shader->Bind();

    }

	void ForwardRenderer::BeginScene(Camera* camera)
	{
		auto proj = camera->GetProjectionMatrix();

#ifdef JM_RENDER_API_VULKAN
		if (graphics::Context::GetRenderAPI() == RenderAPI::VULKAN)
		proj[5] *= -1;
#endif

		memcpy(m_VSSystemUniformBuffer + m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionMatrix], &proj, sizeof(maths::Matrix4));
		memcpy(m_VSSystemUniformBuffer + m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ViewMatrix], &camera->GetViewMatrix(), sizeof(maths::Matrix4));
	}

	void ForwardRenderer::Submit(const RenderCommand& command)
	{
		m_CommandQueue.push_back(command);
	}

	void ForwardRenderer::SubmitMesh(Mesh* mesh, const maths::Matrix4& transform, const maths::Matrix4& textureMatrix)
	{
		RenderCommand command;
		command.mesh = mesh;
		command.transform = transform;
		command.textureMatrix = textureMatrix;
		Submit(command);
	}

	void ForwardRenderer::EndScene()
	{
	}

	void ForwardRenderer::End()
	{
		m_RenderPass->EndRenderpass(commandBuffers[i]);
        commandBuffers[i]->EndRecording();
	}

	void ForwardRenderer::SetCubeMap(Texture* cubeMap)
	{
		m_SkyboxRenderer = new SkyboxRenderer(m_ScreenBufferWidth, m_ScreenBufferHeight);
		m_SkyboxRenderer->SetCubeMap(cubeMap);
		m_SkyboxRenderer->SetRenderInfo(m_RenderPass);
		m_SkyboxRenderer->Init();
	}

	void ForwardRenderer::SetSystemUniforms(Shader* shader) const
	{
		shader->SetSystemUniformBuffer(ShaderType::VERTEX, m_VSSystemUniformBuffer, m_VSSystemUniformBufferSize, 0);

		m_UniformBuffer->SetData(sizeof(UniformBufferObject), *&m_VSSystemUniformBuffer);

		int index = 0;

		for (auto& command : m_CommandQueue)
		{
			maths::Matrix4* modelMat = reinterpret_cast<maths::Matrix4*>((reinterpret_cast<uint64_t>(uboDataDynamic.model) + (index * dynamicAlignment)));
			*modelMat = command.transform;
			index++;
		}

		shader->SetSystemUniformBuffer(ShaderType::FRAGMENT, m_PSSystemUniformBuffer, m_PSSystemUniformBufferSize, 0);

		m_ModelUniformBuffer->SetDynamicData(static_cast<uint32_t>(MAX_OBJECTS * dynamicAlignment), sizeof(maths::Matrix4), &*uboDataDynamic.model);
	}

	void ForwardRenderer::InitScene(Scene* scene)
	{
		m_DefaultTexture = Texture2D::CreateFromFile("Test", "/CoreTextures/checkerboard.tga");

		for (auto& entity : scene->GetEntities())
		{
			Model* model = entity->GetComponent<ModelComponent>()->m_Model.get();
			auto& meshes = model->GetMeshs();

			for (auto& mesh : meshes)
			{
				if (mesh->GetMaterial())
					mesh->GetMaterial()->CreateDescriptorSet(m_GraphicsPipeline, 1);
			}
		}

		graphics::api::DescriptorInfo info{};
		info.pipeline = m_GraphicsPipeline;
		info.layoutIndex = 1;
		info.shader = m_Shader;
		m_DefaultDescriptorSet = graphics::api::DescriptorSet::Create(info);

		std::vector<graphics::api::ImageInfo> bufferInfos;

		graphics::api::ImageInfo imageInfo = {};
		imageInfo.texture = m_DefaultTexture;
		imageInfo.binding = 0;
		imageInfo.name = "texSampler";

		bufferInfos.push_back(imageInfo);

		m_DefaultDescriptorSet->Update(bufferInfos);

		if(scene->GetEnvironmentMap())
			SetCubeMap((Texture*)scene->GetEnvironmentMap());
	}

	void ForwardRenderer::Present()
	{
        int index = 0;

		for (auto& command : m_CommandQueue)
		{
            Mesh* mesh = command.mesh;

            graphics::api::CommandBuffer* currentCMDBuffer = (mesh->GetCommandBuffer(static_cast<int>(i)));

			currentCMDBuffer->BeginRecordingSecondary(m_RenderPass, Renderer::GetSwapchain()->GetFramebuffer(i));

            currentCMDBuffer->UpdateViewport(m_ScreenBufferWidth,m_ScreenBufferHeight);

            m_GraphicsPipeline->SetActive(currentCMDBuffer);

            uint32_t dynamicOffset = index * static_cast<uint32_t>(dynamicAlignment);

			m_Shader->SetUserUniformBuffer(ShaderType::VERTEX, reinterpret_cast<byte*>(m_ModelUniformBuffer->GetBuffer()) + dynamicOffset, sizeof(maths::Matrix4));

			Renderer::RenderMesh(mesh, m_GraphicsPipeline, currentCMDBuffer, dynamicOffset, m_DefaultDescriptorSet);

            currentCMDBuffer->EndRecording();
            currentCMDBuffer->ExecuteSecondary(commandBuffers[i]);

            index++;
		}

		if(m_SkyboxRenderer)
			m_SkyboxRenderer->Render(commandBuffers[i], Application::Instance()->GetSceneManager()->GetCurrentScene()->GetCamera(), i);
	}

	void ForwardRenderer::OnResize(uint width, uint height)
	{
		m_ScreenBufferWidth  = static_cast<uint>(width);
		m_ScreenBufferHeight = static_cast<uint>(height);

		delete m_GraphicsPipeline;
		delete m_DepthTexture;
		delete m_RenderPass;
		delete m_Shader;

		m_RenderPass = graphics::api::RenderPass::Create();
		m_DepthTexture = TextureDepth::Create(width, height);

		TextureType textureTypes[2] = { TextureType::COLOUR , TextureType::DEPTH };
		graphics::api::RenderpassInfo renderpassCI{};
		renderpassCI.attachmentCount = 2;
		renderpassCI.textureType = textureTypes;
		m_RenderPass->Init(renderpassCI);

        CreateGraphicsPipeline();

		Renderer::GetSwapchain()->Init(m_DepthTexture, m_RenderPass);

		std::vector<graphics::api::BufferInfo> bufferInfos;

		graphics::api::BufferInfo bufferInfo = {};
		bufferInfo.buffer = m_UniformBuffer;
		bufferInfo.offset = 0;
		bufferInfo.size = sizeof(UniformBufferObject);
		bufferInfo.type = graphics::api::DescriptorType::UNIFORM_BUFFER;
		bufferInfo.binding = 0;

		graphics::api::BufferInfo bufferInfo2 = {};
		bufferInfo2.buffer = m_ModelUniformBuffer;
		bufferInfo2.offset = 0;
		bufferInfo2.size = sizeof(UniformBufferModel);
		bufferInfo2.type = graphics::api::DescriptorType::UNIFORM_BUFFER_DYNAMIC;
		bufferInfo2.binding = 1;

		bufferInfos.push_back(bufferInfo);
		bufferInfos.push_back(bufferInfo2);

		m_GraphicsPipeline->GetDescriptorSet()->Update(bufferInfos);

	}

    void ForwardRenderer::CreateGraphicsPipeline()
    {
        m_Shader = Shader::CreateFromFile("Simple","/CoreShaders/");

        std::vector<graphics::api::DescriptorPoolInfo> poolInfo =
        {
            { graphics::api::DescriptorType::UNIFORM_BUFFER, MAX_OBJECTS },
            { graphics::api::DescriptorType::UNIFORM_BUFFER_DYNAMIC, MAX_OBJECTS },
            { graphics::api::DescriptorType::IMAGE_SAMPLER, MAX_OBJECTS }
        };

        std::vector<graphics::api::DescriptorLayoutInfo> layoutInfo =
        {
            { graphics::api::DescriptorType::UNIFORM_BUFFER, graphics::api::ShaderStage::VERTEX, 0 },
            { graphics::api::DescriptorType::UNIFORM_BUFFER_DYNAMIC,graphics::api::ShaderStage::VERTEX , 1 },
        };

        std::vector<graphics::api::DescriptorLayoutInfo> layoutInfoMesh =
        {
             { graphics::api::DescriptorType::IMAGE_SAMPLER,graphics::api::ShaderStage::FRAGMENT , 0 }
        };

        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        std::vector<graphics::api::DescriptorLayout> descriptorLayouts;

        graphics::api::DescriptorLayout sceneDescriptorLayout{};
        sceneDescriptorLayout.count = static_cast<uint>(layoutInfo.size());
        sceneDescriptorLayout.layoutInfo = layoutInfo.data();

        descriptorLayouts.push_back(sceneDescriptorLayout);

        graphics::api::DescriptorLayout meshDescriptorLayout{};
        meshDescriptorLayout.count = static_cast<uint>(layoutInfoMesh.size());
        meshDescriptorLayout.layoutInfo = layoutInfoMesh.data();

        descriptorLayouts.push_back(meshDescriptorLayout);

        graphics:: api::PipelineInfo pipelineCI{};
        pipelineCI.pipelineName = "ForwardRenderer";
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
        pipelineCI.cullMode = graphics::api::CullMode::BACK;
        pipelineCI.transparencyEnabled = false;
        pipelineCI.depthBiasEnabled = false;
        pipelineCI.width = m_ScreenBufferWidth;
        pipelineCI.height = m_ScreenBufferHeight;
        pipelineCI.maxObjects = MAX_OBJECTS;

        m_GraphicsPipeline = graphics::api::Pipeline::Create(pipelineCI);
    }
}
