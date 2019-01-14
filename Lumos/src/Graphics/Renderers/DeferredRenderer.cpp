#include "LM.h"
#include "DeferredRenderer.h"
#include "Graphics/API/Shader.h"
#include "Graphics/RenderList.h"
#include "Graphics/API/FrameBuffer.h"
#include "Graphics/ParticleManager.h"
#include "Graphics/Light.h"
#include "Graphics/API/Textures/TextureCube.h"
#include "Graphics/API/Textures/TextureDepth.h"
#include "Graphics/API/Textures/TextureDepthArray.h"
#include "Graphics/Model/Model.h"
#include "Graphics/Mesh.h"
#include "Graphics/MeshFactory.h"
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
#include "Renderer/GraphicsPipeline.h"
#include "Entity/Entity.h"
#include "SkyboxRenderer.h"
#include "Maths/Maths.h"
#include "Maths/MathsUtilities.h"
#include "Maths/BoundingSphere.h"
#include "ShadowRenderer.h"
#include "App/Application.h"

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/GraphicsAPI/Vulkan/VKRenderer.h"
#endif

#define MAX_LIGHTS 10

namespace Lumos
{

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

	DeferredRenderer::DeferredRenderer(uint width, uint height)
	{
		DeferredRenderer::SetScreenBufferSize(width, height);
		DeferredRenderer::Init();
	}

	DeferredRenderer::~DeferredRenderer()
	{
		delete m_DeferredShader;
		delete m_OffScreenShader;
		delete m_FBO;
		delete m_DefaultTexture;
		delete m_UniformBuffer;
		delete m_ModelUniformBuffer;
		delete m_LightUniformBuffer;
		delete m_DeferredRenderpass;
		delete m_DeferredPipeline;
		delete m_OffScreenRenderpass;
		delete m_OffScreenPipeline;
		delete m_DepthTexture;
		delete m_ScreenQuad;
		delete m_ShadowRenderer;
        delete m_DefaultDescriptorSet;
		SAFE_DELETE(m_SkyboxRenderer);

        delete[] m_VSSystemUniformBuffer;
        delete[] m_PSSystemUniformBuffer;
		for(auto& commandBuffer : m_CommandBuffers)
		{
			delete commandBuffer;
		}

		m_CommandBuffers.clear();
	}

	void DeferredRenderer::Init()
	{
		m_OffScreenShader = Shader::CreateFromFile("DeferredColour", "/CoreShaders/");
		m_DeferredShader = Shader::CreateFromFile("DeferredLight", "/CoreShaders/");

		m_GBuffer = std::make_unique<GBuffer>(m_ScreenBufferWidth, m_ScreenBufferHeight);
        m_DefaultTexture = Texture2D::CreateFromFile("Test", "/CoreTextures/checkerboard.tga");

        m_DeferredDescriptorSet = nullptr;
        m_DefaultDescriptorSet = nullptr;

		TextureParameters param;
		param.filter = TextureFilter::LINEAR;
		param.format = TextureFormat::RGBA;
		param.wrap = TextureWrap::CLAMP_TO_EDGE;
		m_PreintegratedFG = std::unique_ptr<Texture2D>(Texture2D::CreateFromFile("PreintegratedFG", "/CoreTextures/PreintegratedFG.png", param));

		m_LightUniformBuffer = nullptr;
		m_UniformBuffer = nullptr;
		m_ModelUniformBuffer = nullptr;
		m_DefaultMaterialDataUniformBuffer = nullptr;

		m_ScreenQuad = Lumos::MeshFactory::CreateQuad();

		m_CommandQueue.reserve(1000);

		//
		// Vertex shader system uniforms
		//
		m_VSSystemUniformBufferSize = sizeof(maths::Matrix4);// +sizeof(Matrix4) + sizeof(Matrix4); //+ sizeof(Vector3) ;
		m_VSSystemUniformBuffer = new byte[m_VSSystemUniformBufferSize];
		memset(m_VSSystemUniformBuffer, 0, m_VSSystemUniformBufferSize);
		m_VSSystemUniformBufferOffsets.resize(VSSystemUniformIndex_Size);

        // Per Scene System Uniforms
        m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionMatrix] = 0;

        // Pixel/fragment shader system uniforms
        m_PSSystemUniformBufferSize = sizeof(UniformBufferLight);// sizeof(maths::Vector3) + sizeof(maths::Vector3) + sizeof(maths::Vector3);//sizeof(Light);
        m_PSSystemUniformBuffer = new byte[m_PSSystemUniformBufferSize];
        memset(m_PSSystemUniformBuffer, 0, m_PSSystemUniformBufferSize);
        m_PSSystemUniformBufferOffsets.resize(PSSystemUniformIndex_Size);

        // Per Scene System Uniforms
        m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_Lights] = 0;

		m_OffScreenRenderpass = graphics::api::RenderPass::Create();
		m_DeferredRenderpass = graphics::api::RenderPass::Create();
		m_DepthTexture = TextureDepth::Create(m_ScreenBufferWidth, m_ScreenBufferHeight);

		TextureType textureTypes[2] = { TextureType::COLOUR, TextureType::DEPTH };
		graphics::api::RenderpassInfo renderpassCI{};
		renderpassCI.attachmentCount = 2;
		renderpassCI.textureType = textureTypes;

		m_DeferredRenderpass->Init(renderpassCI);

		TextureType textureTypesOffScreen[5] = { TextureType::COLOUR , TextureType::COLOUR ,TextureType::COLOUR ,TextureType::COLOUR ,TextureType::DEPTH };
		graphics::api::RenderpassInfo renderpassCIOffScreen{};
		renderpassCIOffScreen.attachmentCount = 5;
		renderpassCIOffScreen.textureType = textureTypesOffScreen;

		m_OffScreenRenderpass->Init(renderpassCIOffScreen);

		Renderer::GetSwapchain()->Init(m_DepthTexture, m_DeferredRenderpass);

		m_CommandBuffers.resize(Renderer::GetSwapchain()->GetSwapchainBufferCount());

		for (auto& commandBuffer : m_CommandBuffers)
		{
			commandBuffer = graphics::api::CommandBuffer::Create();
			commandBuffer->Init(true);
		}

		m_DeferredCommandBuffers = graphics::api::CommandBuffer::Create();
		m_DeferredCommandBuffers->Init(true);

        CreateOffScreenPipeline();
		CreateDeferredPipeline();
		CreateOffScreenBuffer();
		CreateLightBuffer();

		graphics::api::DescriptorInfo info{};
		info.pipeline = m_DeferredPipeline;
		info.layoutIndex = 1; //?
		info.shader = m_DeferredShader;
		if(m_DeferredDescriptorSet)
			delete m_DeferredDescriptorSet;
		m_DeferredDescriptorSet = graphics::api::DescriptorSet::Create(info);

		m_ClearColour = maths::Vector4(0.8f, 0.8f, 0.8f, 1.0f);

		m_ShadowTexture = std::unique_ptr<TextureDepthArray>(TextureDepthArray::Create(2048, 2048, 4));
		m_SkyboxRenderer = nullptr;
        m_ShadowRenderer = new ShadowRenderer(m_ShadowTexture.get(), 2048, 4);
		m_FBO = nullptr;
	}

	void DeferredRenderer::RenderScene(RenderList* renderList, Scene* scene)
	{
		if(m_ShadowRenderer)
			m_ShadowRenderer->RenderScene(nullptr, scene);

		SubmitLightSetup(*scene->GetLightSetup(),scene);

		BeginOffscreen();
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
						if (mesh->GetMaterial() && mesh->GetMaterial()->GetDescriptorSet() == nullptr)
							mesh->GetMaterial()->CreateDescriptorSet(m_OffScreenPipeline, 1);

						TextureMatrixComponent* textureMatrixTransform = obj->GetComponent<TextureMatrixComponent>();
						maths::Matrix4 textureMatrix;
						if (textureMatrixTransform)
							textureMatrix = textureMatrixTransform->m_TextureMatrix;
						else
							textureMatrix = maths::Matrix4();

						auto transform = obj->GetComponent<TransformComponent>()->m_WorldSpaceTransform;

						bool inside = true;

						#if 0
						float maxScaling = 0.0f;
						maxScaling = maths::Max(transform.GetScaling().GetX(), maxScaling);
						maxScaling = maths::Max(transform.GetScaling().GetY(), maxScaling);
						maxScaling = maths::Max(transform.GetScaling().GetZ(), maxScaling);

						inside = GraphicsPipeline::Instance()->GetFrustum().InsideFrustum(transform * mesh->GetBoundingSphere()->Centre(), maxScaling * mesh->GetBoundingSphere()->SphereRadius());
						#endif
                        if (inside)
                        {
                            SubmitMesh(mesh.get(), transform, textureMatrix);
                        }
					}
				}
			}
		});

		SetSystemUniforms(m_OffScreenShader);

		PresentOffScreen();

		EndOffScreen();

	}

	void DeferredRenderer::PresentToScreen()
	{
#ifdef LUMOS_RENDER_API_VULKAN //TODO : remove platform specific code
		if (graphics::Context::GetRenderAPI() == RenderAPI::VULKAN)
			graphics::VKRenderer::GetRenderer()->Present(static_cast<graphics::VKCommandBuffer*>(m_CommandBuffers[graphics::VKRenderer::GetRenderer()->GetSwapchain()->GetCurrentBufferId()]));
#endif
	}

	void DeferredRenderer::InitScene(Scene* scene, bool newScene)
	{
		if(m_ShadowRenderer)
			m_ShadowRenderer->ClearRenderLists();

		for (auto& entity : scene->GetEntities())
		{
			auto modelComponent = entity->GetComponent<ModelComponent>();
			if(modelComponent)
			{
				Model* model = entity->GetComponent<ModelComponent>()->m_Model.get();
				auto& meshes = model->GetMeshs();

				for (auto& mesh : meshes)
				{
					if (mesh->GetMaterial())
						mesh->GetMaterial()->CreateDescriptorSet(m_OffScreenPipeline, 1);
				}
			}
		}

		{
			if(m_DefaultMaterialDataUniformBuffer == nullptr)
			{
				m_DefaultMaterialDataUniformBuffer = graphics::api::UniformBuffer::Create();

				MaterialProperties properties;
				properties.glossColour = 1.0f;
				properties.specularColour = maths::Vector4(0.0f,1.0f,0.0f,1.0f);
				properties.usingAlbedoMap   = 1.0f;
				properties.usingGlossMap    = 0.0f;
				properties.usingNormalMap   = 0.0f;
				properties.usingSpecularMap = 0.0f;

				uint32_t bufferSize = static_cast<uint32_t>(sizeof(MaterialProperties));
				m_DefaultMaterialDataUniformBuffer->Init(bufferSize, nullptr);
				m_DefaultMaterialDataUniformBuffer->SetData(bufferSize, &properties);
			}

            if(m_DefaultDescriptorSet)
                delete m_DefaultDescriptorSet;

			graphics::api::DescriptorInfo info{};
			info.pipeline = m_OffScreenPipeline;
			info.layoutIndex = 1;
			info.shader = m_OffScreenShader;
			m_DefaultDescriptorSet = graphics::api::DescriptorSet::Create(info);

			std::vector<graphics::api::ImageInfo> imageInfos;

			graphics::api::ImageInfo imageInfo = {};
			imageInfo.texture = m_DefaultTexture;
			imageInfo.binding = 0;
			imageInfo.name = "u_AlbedoMap";

			imageInfos.push_back(imageInfo);

			std::vector<graphics::api::BufferInfo> bufferInfos;

			graphics::api::BufferInfo bufferInfo = {};
			bufferInfo.buffer = m_DefaultMaterialDataUniformBuffer;
			bufferInfo.offset = 0;
			bufferInfo.size = sizeof(MaterialProperties);
			bufferInfo.type = graphics::api::DescriptorType::UNIFORM_BUFFER;
			bufferInfo.binding = 4;
			bufferInfo.shaderType = ShaderType::FRAGMENT;
			bufferInfo.name = "UniformMaterialData";
			bufferInfo.systemUniforms = false;

			bufferInfos.push_back(bufferInfo);

			m_DefaultDescriptorSet->Update(imageInfos, bufferInfos);
		}

		{
			std::vector<graphics::api::ImageInfo> bufferInfos;

			graphics::api::ImageInfo imageInfo = {};
			imageInfo.texture = m_GBuffer->m_ScreenTex[SCREENTEX_COLOUR];
			imageInfo.binding = 0;
			imageInfo.name = "uColourSampler";

			graphics::api::ImageInfo imageInfo2 = {};
			imageInfo2.texture = m_GBuffer->m_ScreenTex[SCREENTEX_POSITION];
			imageInfo2.binding = 1;
			imageInfo2.name = "uPositionSampler";

			graphics::api::ImageInfo imageInfo3 = {};
			imageInfo3.texture = m_GBuffer->m_ScreenTex[SCREENTEX_NORMALS];
			imageInfo3.binding = 2;
			imageInfo3.name = "uNormalSampler";

			graphics::api::ImageInfo imageInfo4 = {};
			imageInfo4.texture = m_GBuffer->m_ScreenTex[SCREENTEX_PBR];
			imageInfo4.binding = 3;
			imageInfo4.name = "uPBRSampler";

			graphics::api::ImageInfo imageInfo5 = {};
			imageInfo5.texture = m_PreintegratedFG.get();
			imageInfo5.binding = 4;
			imageInfo5.name = "uPreintegratedFG";

			graphics::api::ImageInfo imageInfo6 = {};
			imageInfo6.texture = scene->GetEnvironmentMap();
			imageInfo6.binding = 5;
			imageInfo6.type = TextureType::CUBE;
			imageInfo6.name = "uEnvironmentMap";

            graphics::api::ImageInfo imageInfo7 = {};
            imageInfo7.texture = (Texture*)m_ShadowTexture.get();
            imageInfo7.binding = 6;
			imageInfo7.type = TextureType::DEPTHARRAY;
            imageInfo7.name = "uShadowMap";

			graphics::api::ImageInfo imageInfo8 = {};
			imageInfo8.texture = m_GBuffer->m_DepthTexture;
			imageInfo8.binding = 7;
			imageInfo8.type = TextureType::DEPTH;
			imageInfo8.name = "uDepthSampler";

			bufferInfos.push_back(imageInfo);
			bufferInfos.push_back(imageInfo2);
			bufferInfos.push_back(imageInfo3);
			bufferInfos.push_back(imageInfo4);
			bufferInfos.push_back(imageInfo5);
			if(scene->GetEnvironmentMap())
				bufferInfos.push_back(imageInfo6);
			bufferInfos.push_back(imageInfo7);

			m_DeferredDescriptorSet->Update(bufferInfos);
		}

		if(m_FBO)
			delete m_FBO;

		CreateOffScreenFBO();
		if(newScene)
		{
			if(scene->GetEnvironmentMap())
				SetCubeMap((Texture*)scene->GetEnvironmentMap());
			else
			{
				SAFE_DELETE(m_SkyboxRenderer);
			}
		}
	}

	void DeferredRenderer::Begin(int commandBufferID)
	{
		m_CommandQueue.clear();
		m_SystemUniforms.clear();

		m_CommandBufferIndex = commandBufferID;

        m_CommandBuffers[m_CommandBufferIndex]->BeginRecording();

		m_DeferredRenderpass->BeginRenderpass(m_CommandBuffers[m_CommandBufferIndex], m_ClearColour, Renderer::GetSwapchain()->GetFramebuffer(m_CommandBufferIndex), graphics::api::SECONDARY, m_ScreenBufferWidth, m_ScreenBufferHeight);
	}

	void DeferredRenderer::BeginOffscreen()
	{
		m_CommandQueue.clear();
		m_SystemUniforms.clear();

        m_DeferredCommandBuffers->BeginRecording();

		m_OffScreenRenderpass->BeginRenderpass(m_DeferredCommandBuffers, maths::Vector4(0.0f), m_FBO, graphics::api::SECONDARY, m_ScreenBufferWidth, m_ScreenBufferHeight);
	}

	void DeferredRenderer::BeginScene(Camera* camera)
	{
		auto proj = camera->GetProjectionMatrix();

#ifdef LUMOS_RENDER_API_VULKAN
		if (graphics::Context::GetRenderAPI() == RenderAPI::VULKAN)
		proj[5] *= -1;
#endif
		auto projView = proj * camera->GetViewMatrix();
		memcpy(m_VSSystemUniformBuffer + m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionMatrix], &projView, sizeof(maths::Matrix4));
	}

	void DeferredRenderer::Submit(const RenderCommand& command)
	{
		m_CommandQueue.push_back(command);
	}

	void DeferredRenderer::SubmitMesh(Mesh* mesh, const maths::Matrix4& transform, const maths::Matrix4& textureMatrix)
	{
		RenderCommand command;
		command.mesh = mesh;
		command.transform = transform;
		command.textureMatrix = textureMatrix;
		Submit(command);
	}

	void DeferredRenderer::SubmitLightSetup(const LightSetup& lightSetup, Scene* scene)
	{
		//One Directional Light
		if (lightSetup.GetDirectionalLight() != nullptr)
		{
			maths::Vector4 lightPos = maths::Vector4(lightSetup.GetDirectionalLight()->GetPosition());
			maths::Vector4 lightDir = maths::Vector4(lightSetup.GetDirectionalLight()->GetDirection());
			maths::Vector4 cameraPos = maths::Vector4(scene->GetCamera()->GetPosition());

			memcpy(m_PSSystemUniformBuffer, &lightPos, sizeof(maths::Vector4));
			memcpy(m_PSSystemUniformBuffer + sizeof(Lumos::maths::Vector4), &lightDir, sizeof(maths::Vector4));
			memcpy(m_PSSystemUniformBuffer + sizeof(Lumos::maths::Vector4) + sizeof(Lumos::maths::Vector4), &cameraPos, sizeof(maths::Vector4));

			if(m_ShadowRenderer)
			{
				maths::Matrix4* shadowTransforms = m_ShadowRenderer->GetShadowProjView();
            	maths::Vector2 shadowPixalSize   = maths::Vector2(1.0f / m_ShadowRenderer->GetShadowMapSize());
				memcpy(m_PSSystemUniformBuffer + sizeof(Lumos::maths::Vector4) + sizeof(Lumos::maths::Vector4) + sizeof(Lumos::maths::Vector4), shadowTransforms, sizeof(maths::Matrix4) * 16);
				memcpy(m_PSSystemUniformBuffer + sizeof(Lumos::maths::Vector4) + sizeof(Lumos::maths::Vector4) + sizeof(Lumos::maths::Vector4) + sizeof(maths::Matrix4) * 16, &shadowPixalSize, sizeof(maths::Vector2));
			}
		}
	}

	void DeferredRenderer::EndScene()
	{
	}

	void DeferredRenderer::End()
	{
		m_DeferredRenderpass->EndRenderpass(m_CommandBuffers[m_CommandBufferIndex]);
        m_CommandBuffers[m_CommandBufferIndex]->EndRecording();
	}

	void DeferredRenderer::EndOffScreen()
	{
		m_OffScreenRenderpass->EndRenderpass(m_DeferredCommandBuffers);
        m_DeferredCommandBuffers->EndRecording();
		m_DeferredCommandBuffers->Execute(true);
	}

	void DeferredRenderer::SetSystemUniforms(Shader* shader) const
	{
        m_UniformBuffer->SetData(sizeof(UniformBufferObject), *&m_VSSystemUniformBuffer);

        int index = 0;

        for (auto& command : m_CommandQueue)
        {
            maths::Matrix4* modelMat = reinterpret_cast<maths::Matrix4*>((reinterpret_cast<uint64_t>(uboDataDynamic.model) + (index * dynamicAlignment)));
            *modelMat = command.transform;
            index++;
        }
        m_ModelUniformBuffer->SetDynamicData(static_cast<uint32_t>(MAX_OBJECTS * dynamicAlignment), sizeof(maths::Matrix4), &*uboDataDynamic.model);

		m_LightUniformBuffer->SetData(sizeof(UniformBufferLight), *&m_PSSystemUniformBuffer);
	}

	void DeferredRenderer::Present()
	{
		graphics::api::CommandBuffer* currentCMDBuffer = (m_ScreenQuad->GetCommandBuffer(static_cast<int>(m_CommandBufferIndex)));

		currentCMDBuffer->BeginRecordingSecondary(m_DeferredRenderpass, Renderer::GetSwapchain()->GetFramebuffer(m_CommandBufferIndex));
		currentCMDBuffer->UpdateViewport(m_ScreenBufferWidth,m_ScreenBufferHeight);

		m_DeferredPipeline->SetActive(currentCMDBuffer);

		Renderer::RenderMesh(m_ScreenQuad, m_DeferredPipeline, currentCMDBuffer, 0, m_DeferredDescriptorSet);

		currentCMDBuffer->EndRecording();
		currentCMDBuffer->ExecuteSecondary(m_CommandBuffers[m_CommandBufferIndex]);

		if(m_SkyboxRenderer)
			m_SkyboxRenderer->Render(m_CommandBuffers[m_CommandBufferIndex], Application::Instance()->GetSceneManager()->GetCurrentScene()->GetCamera(), m_CommandBufferIndex);
	}

	void DeferredRenderer::PresentOffScreen()
	{
		int index = 0;

		for (auto& command : m_CommandQueue)
		{
            Mesh* mesh = command.mesh;

            graphics::api::CommandBuffer* currentCMDBuffer = mesh->GetCommandBuffer(0);

			currentCMDBuffer->BeginRecordingSecondary(m_OffScreenRenderpass, m_FBO);
            currentCMDBuffer->UpdateViewport(m_ScreenBufferWidth,m_ScreenBufferHeight);

            m_OffScreenPipeline->SetActive(currentCMDBuffer);

            uint32_t dynamicOffset = index * static_cast<uint32_t>(dynamicAlignment);

			Renderer::RenderMesh(mesh, m_OffScreenPipeline, currentCMDBuffer, dynamicOffset, m_DefaultDescriptorSet);

            currentCMDBuffer->EndRecording();
            currentCMDBuffer->ExecuteSecondary(m_DeferredCommandBuffers);

            index++;
		}
	}

	void DeferredRenderer::CreateOffScreenPipeline()
    {
        std::vector<graphics::api::DescriptorPoolInfo> poolInfo =
        {
            { graphics::api::DescriptorType::UNIFORM_BUFFER, MAX_OBJECTS },
			{ graphics::api::DescriptorType::UNIFORM_BUFFER, MAX_OBJECTS },
            { graphics::api::DescriptorType::UNIFORM_BUFFER_DYNAMIC, MAX_OBJECTS },
            { graphics::api::DescriptorType::IMAGE_SAMPLER, MAX_OBJECTS },
			{ graphics::api::DescriptorType::IMAGE_SAMPLER, MAX_OBJECTS },
			{ graphics::api::DescriptorType::IMAGE_SAMPLER, MAX_OBJECTS },
			{ graphics::api::DescriptorType::IMAGE_SAMPLER, MAX_OBJECTS }
        };

        std::vector<graphics::api::DescriptorLayoutInfo> layoutInfo =
        {
            { graphics::api::DescriptorType::UNIFORM_BUFFER, graphics::api::ShaderStage::VERTEX, 0 },
            { graphics::api::DescriptorType::UNIFORM_BUFFER_DYNAMIC,graphics::api::ShaderStage::VERTEX , 1 },
        };

        std::vector<graphics::api::DescriptorLayoutInfo> layoutInfoMesh =
        {
            { graphics::api::DescriptorType::IMAGE_SAMPLER,graphics::api::ShaderStage::FRAGMENT , 0 },
			{ graphics::api::DescriptorType::IMAGE_SAMPLER,graphics::api::ShaderStage::FRAGMENT , 1 },
			{ graphics::api::DescriptorType::IMAGE_SAMPLER,graphics::api::ShaderStage::FRAGMENT , 2 },
			{ graphics::api::DescriptorType::IMAGE_SAMPLER,graphics::api::ShaderStage::FRAGMENT , 3 },
			{ graphics::api::DescriptorType::UNIFORM_BUFFER, graphics::api::ShaderStage::FRAGMENT, 4 },
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
        pipelineCI.pipelineName = "OffScreenRenderer";
        pipelineCI.shader = m_OffScreenShader;
        pipelineCI.vulkanRenderpass = m_OffScreenRenderpass;
        pipelineCI.numVertexLayout = static_cast<uint>(attributeDescriptions.size());
        pipelineCI.descriptorLayouts = descriptorLayouts;
        pipelineCI.vertexLayout = attributeDescriptions.data();
        pipelineCI.numLayoutBindings = static_cast<uint>(poolInfo.size());
        pipelineCI.typeCounts = poolInfo.data();
        pipelineCI.strideSize = sizeof(Vertex);
        pipelineCI.numColorAttachments = 5;
        pipelineCI.wireframeEnabled = false;
        pipelineCI.cullMode = graphics::api::CullMode::BACK;
        pipelineCI.transparencyEnabled = false;
        pipelineCI.depthBiasEnabled = false;
        pipelineCI.width = m_ScreenBufferWidth;
        pipelineCI.height = m_ScreenBufferHeight;
        pipelineCI.maxObjects = MAX_OBJECTS;

        m_OffScreenPipeline = graphics::api::Pipeline::Create(pipelineCI);
    }

	void DeferredRenderer::CreateDeferredPipeline()
    {
        std::vector<graphics::api::DescriptorPoolInfo> poolInfo =
        {
            { graphics::api::DescriptorType::IMAGE_SAMPLER , 1 },
			{ graphics::api::DescriptorType::IMAGE_SAMPLER , 1 },
			{ graphics::api::DescriptorType::IMAGE_SAMPLER , 1 },
			{ graphics::api::DescriptorType::IMAGE_SAMPLER , 1 },
			{ graphics::api::DescriptorType::IMAGE_SAMPLER , 1 },
			{ graphics::api::DescriptorType::IMAGE_SAMPLER , 1 },
			{ graphics::api::DescriptorType::IMAGE_SAMPLER , 1 },
            { graphics::api::DescriptorType::IMAGE_SAMPLER , 1 },
			{ graphics::api::DescriptorType::UNIFORM_BUFFER, 1 }
        };

        std::vector<graphics::api::DescriptorLayoutInfo> layoutInfo =
        {
            { graphics::api::DescriptorType::UNIFORM_BUFFER, graphics::api::ShaderStage::FRAGMENT, 0 },

        };

        std::vector<graphics::api::DescriptorLayoutInfo> layoutInfoMesh =
        {
             { graphics::api::DescriptorType::IMAGE_SAMPLER,graphics::api::ShaderStage::FRAGMENT , 0 },
			 { graphics::api::DescriptorType::IMAGE_SAMPLER,graphics::api::ShaderStage::FRAGMENT , 1 },
			 { graphics::api::DescriptorType::IMAGE_SAMPLER,graphics::api::ShaderStage::FRAGMENT , 2 },
			 { graphics::api::DescriptorType::IMAGE_SAMPLER,graphics::api::ShaderStage::FRAGMENT , 3 },
			 { graphics::api::DescriptorType::IMAGE_SAMPLER,graphics::api::ShaderStage::FRAGMENT , 4 },
			 { graphics::api::DescriptorType::IMAGE_SAMPLER,graphics::api::ShaderStage::FRAGMENT , 5 },
			 { graphics::api::DescriptorType::IMAGE_SAMPLER,graphics::api::ShaderStage::FRAGMENT , 6 },
             { graphics::api::DescriptorType::IMAGE_SAMPLER,graphics::api::ShaderStage::FRAGMENT , 7 }
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
        pipelineCI.pipelineName = "Deferred";
        pipelineCI.shader = m_DeferredShader;
        pipelineCI.vulkanRenderpass = m_DeferredRenderpass;
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
        pipelineCI.maxObjects = 10;

        m_DeferredPipeline = graphics::api::Pipeline::Create(pipelineCI);
    }

	void DeferredRenderer::CreateOffScreenBuffer()
	{
		if(m_UniformBuffer == nullptr)
		{
			m_UniformBuffer = graphics::api::UniformBuffer::Create();

			uint32_t bufferSize = static_cast<uint32_t>(sizeof(UniformBufferObject));
			m_UniformBuffer->Init(bufferSize, nullptr);
		}

		if(m_ModelUniformBuffer == nullptr)
		{
			m_ModelUniformBuffer = graphics::api::UniformBuffer::Create();
			const size_t minUboAlignment = graphics::Context::GetContext()->GetMinUniformBufferOffsetAlignment();

			dynamicAlignment = sizeof(Lumos::maths::Matrix4);
			if (minUboAlignment > 0)
			{
				dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
			}

			uint32_t bufferSize2 = static_cast<uint32_t>(MAX_OBJECTS * dynamicAlignment);

			uboDataDynamic.model = static_cast<maths::Matrix4*>(AlignedAlloc(bufferSize2, dynamicAlignment));

			m_ModelUniformBuffer->Init(bufferSize2, nullptr);
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

		graphics::api::BufferInfo bufferInfo2 = {};
		bufferInfo2.buffer = m_ModelUniformBuffer;
		bufferInfo2.offset = 0;
		bufferInfo2.size = sizeof(UniformBufferModel);
		bufferInfo2.type = graphics::api::DescriptorType::UNIFORM_BUFFER_DYNAMIC;
		bufferInfo2.binding = 1;
		bufferInfo2.shaderType = ShaderType::VERTEX;
		bufferInfo2.systemUniforms = false;

		bufferInfos.push_back(bufferInfo);
		bufferInfos.push_back(bufferInfo2);

		m_OffScreenPipeline->GetDescriptorSet()->Update(bufferInfos);
	}

	void DeferredRenderer::CreateOffScreenFBO()
	{
		const uint attachmentCount = 5;
		TextureType attachmentTypes[attachmentCount];
		attachmentTypes[0] = TextureType::COLOUR;
		attachmentTypes[1] = TextureType::COLOUR;
		attachmentTypes[2] = TextureType::COLOUR;
		attachmentTypes[3] = TextureType::COLOUR;
		attachmentTypes[4] = TextureType::DEPTH;

		FrameBufferInfo bufferInfo{};
		bufferInfo.width = m_ScreenBufferWidth;
		bufferInfo.height = m_ScreenBufferHeight;
		bufferInfo.attachmentCount = attachmentCount;
		bufferInfo.renderPass = m_OffScreenRenderpass;
		bufferInfo.attachmentTypes = attachmentTypes;

		Texture* attachments[attachmentCount];
		attachments[0] = m_GBuffer->m_ScreenTex[SCREENTEX_COLOUR];
		attachments[1] = m_GBuffer->m_ScreenTex[SCREENTEX_POSITION];
		attachments[2] = m_GBuffer->m_ScreenTex[SCREENTEX_NORMALS];
		attachments[3] = m_GBuffer->m_ScreenTex[SCREENTEX_PBR];
		attachments[4] = m_GBuffer->m_DepthTexture;
		bufferInfo.attachments = attachments;

		m_FBO = Framebuffer::Create(bufferInfo);
	}

	void DeferredRenderer::CreateLightBuffer()
	{
		if(m_LightUniformBuffer == nullptr)
		{
			m_LightUniformBuffer = graphics::api::UniformBuffer::Create();

			uint32_t bufferSize = static_cast<uint32_t>(sizeof(UniformBufferLight));
			m_LightUniformBuffer->Init(bufferSize, nullptr);
		}

		std::vector<graphics::api::BufferInfo> bufferInfos;

		graphics::api::BufferInfo bufferInfo = {};
		bufferInfo.name = "LightData";
		bufferInfo.buffer = m_LightUniformBuffer;
		bufferInfo.offset = 0;
		bufferInfo.size = sizeof(UniformBufferLight);
		bufferInfo.type = graphics::api::DescriptorType::UNIFORM_BUFFER;
		bufferInfo.binding = 0;
		bufferInfo.shaderType = ShaderType::FRAGMENT;
		bufferInfo.systemUniforms = false;

		bufferInfos.push_back(bufferInfo);

		m_DeferredPipeline->GetDescriptorSet()->Update(bufferInfos);
	}

	void DeferredRenderer::OnResize(uint width, uint height)
	{
		delete m_DeferredPipeline;
		delete m_OffScreenPipeline;
		delete m_DepthTexture;

		DeferredRenderer::SetScreenBufferSize(width, height);

		m_GBuffer->UpdateTextureSize(m_ScreenBufferWidth,m_ScreenBufferHeight);

		m_DepthTexture = TextureDepth::Create(m_ScreenBufferWidth, m_ScreenBufferHeight);

		Renderer::GetSwapchain()->Init(m_DepthTexture, m_DeferredRenderpass);

        CreateOffScreenPipeline();
		CreateDeferredPipeline();
		CreateOffScreenBuffer();
		CreateLightBuffer();

		graphics::api::DescriptorInfo info{};
		info.pipeline = m_DeferredPipeline;
		info.layoutIndex = 1; //?
		info.shader = m_DeferredShader;
		if(m_DeferredDescriptorSet)
			delete m_DeferredDescriptorSet;
		m_DeferredDescriptorSet = graphics::api::DescriptorSet::Create(info);

		m_ClearColour = maths::Vector4(0.8f, 0.8f, 0.8f, 1.0f);

		if(m_SkyboxRenderer != nullptr)
		{
			m_SkyboxRenderer->SetRenderInfo(m_DeferredRenderpass);
			m_SkyboxRenderer->OnResize(m_ScreenBufferWidth, m_ScreenBufferHeight);
		}

	}

	void DeferredRenderer::SetCubeMap(Texture* cubeMap)
	{
		if(m_SkyboxRenderer == nullptr)
		{
			m_SkyboxRenderer = new SkyboxRenderer(m_ScreenBufferWidth, m_ScreenBufferHeight);
			m_SkyboxRenderer->SetCubeMap(cubeMap);
			m_SkyboxRenderer->SetRenderInfo(m_DeferredRenderpass);
			m_SkyboxRenderer->Init();
		}
		else
		{
			m_SkyboxRenderer->SetCubeMap(cubeMap);
		}
	}
}
