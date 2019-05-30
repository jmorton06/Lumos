#include "LM.h"
#include "DeferredOffScreenRenderer.h"
#include "App/Scene.h"
#include "App/Application.h"
#include "Entity/Entity.h"
#include "Maths/Maths.h"

#include "Graphics/RenderManager.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/RenderList.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"
#include "Graphics/GBuffer.h"

#include "Graphics/API/Shader.h"
#include "Graphics/API/Framebuffer.h"
#include "Graphics/API/Textures/Texture2D.h"
#include "Graphics/API/Textures/TextureDepth.h"
#include "Graphics/API/UniformBuffer.h"
#include "Graphics/API/Renderer.h"
#include "Graphics/API/CommandBuffer.h"
#include "Graphics/API/Swapchain.h"
#include "Graphics/API/RenderPass.h"
#include "Graphics/API/Pipeline.h"
#include "Graphics/API/GraphicsContext.h"

#define MAX_LIGHTS 32
#define MAX_SHADOWMAPS 16

namespace lumos
{
	namespace graphics
	{
		enum VSSystemUniformIndices : int32
		{
			VSSystemUniformIndex_ProjectionViewMatrix = 0,
			VSSystemUniformIndex_Size
		};

		enum PSSystemUniformIndices : int32
		{
			PSSystemUniformIndex_Lights = 0,
			PSSystemUniformIndex_Size
		};

		DeferredOffScreenRenderer::DeferredOffScreenRenderer(uint width, uint height)
		{
			DeferredOffScreenRenderer::SetScreenBufferSize(width, height);
			DeferredOffScreenRenderer::Init();
		}

		DeferredOffScreenRenderer::~DeferredOffScreenRenderer()
		{
			delete m_Shader;
			delete m_FBO;
			delete m_DefaultTexture;
			delete m_UniformBuffer;

			delete m_ModelUniformBuffer;
			delete m_RenderPass;
			delete m_Pipeline;
			delete m_DefaultDescriptorSet;
			delete m_DefaultMaterialDataUniformBuffer;
			delete m_DeferredCommandBuffers;

			delete[] m_VSSystemUniformBuffer;
			delete[] m_PSSystemUniformBuffer;

			for (auto& commandBuffer : m_CommandBuffers)
			{
				delete commandBuffer;
			}

			m_CommandBuffers.clear();

			AlignedFree(uboDataDynamic.model);
		}

		void DeferredOffScreenRenderer::Init()
		{
			m_Shader = Shader::CreateFromFile("DeferredColour", "/CoreShaders/");

			m_DefaultTexture = Texture2D::CreateFromFile("Test", "/CoreTextures/checkerboard.tga");

			m_DefaultDescriptorSet = nullptr;

			m_UniformBuffer = nullptr;
			m_ModelUniformBuffer = nullptr;

			m_DefaultMaterialDataUniformBuffer = graphics::UniformBuffer::Create();

			MaterialProperties properties;
			properties.roughnessColour = 1.0f;
			properties.specularColour = maths::Vector4(0.0f, 1.0f, 0.0f, 1.0f);
			properties.usingAlbedoMap = 1.0f;
			properties.usingRoughnessMap = 0.0f;
			properties.usingNormalMap = 0.0f;
			properties.usingSpecularMap = 0.0f;

			uint32_t bufferSize = static_cast<uint32_t>(sizeof(MaterialProperties));
			m_DefaultMaterialDataUniformBuffer->Init(bufferSize, nullptr);
			m_DefaultMaterialDataUniformBuffer->SetData(bufferSize, &properties);

			m_CommandQueue.reserve(1000);

			//
			// Vertex shader system uniforms
			//
			m_VSSystemUniformBufferSize = sizeof(maths::Matrix4);
			m_VSSystemUniformBuffer = new byte[m_VSSystemUniformBufferSize];
			memset(m_VSSystemUniformBuffer, 0, m_VSSystemUniformBufferSize);
			m_VSSystemUniformBufferOffsets.resize(VSSystemUniformIndex_Size);

			// Per Scene System Uniforms
			m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionViewMatrix] = 0;


			m_RenderPass = graphics::RenderPass::Create();

			AttachmentInfo textureTypesOffScreen[5] = 
			{
				{ TextureType::COLOUR, Application::Instance()->GetRenderManager()->GetGBuffer()->GetTextureFormat(SCREENTEX_COLOUR) },
				{ TextureType::COLOUR, Application::Instance()->GetRenderManager()->GetGBuffer()->GetTextureFormat(SCREENTEX_POSITION) },
				{ TextureType::COLOUR, Application::Instance()->GetRenderManager()->GetGBuffer()->GetTextureFormat(SCREENTEX_NORMALS) },
				{ TextureType::COLOUR, Application::Instance()->GetRenderManager()->GetGBuffer()->GetTextureFormat(SCREENTEX_PBR) },
				{ TextureType::DEPTH, TextureFormat::DEPTH }
			};

			graphics::RenderpassInfo renderpassCIOffScreen{};
			renderpassCIOffScreen.attachmentCount = 5;
			renderpassCIOffScreen.textureType = textureTypesOffScreen;

			m_RenderPass->Init(renderpassCIOffScreen);

			m_CommandBuffers.resize(Renderer::GetSwapchain()->GetSwapchainBufferCount());

			for (auto& commandBuffer : m_CommandBuffers)
			{
				commandBuffer = graphics::CommandBuffer::Create();
				commandBuffer->Init(true);
			}

			m_DeferredCommandBuffers = graphics::CommandBuffer::Create();
			m_DeferredCommandBuffers->Init(true);

			CreatePipeline();
			CreateBuffer();
			CreateFBO();
			CreateDefaultDescriptorSet();

			m_ClearColour = maths::Vector4(0.1f, 0.1f, 0.1f, 1.0f);
		}

		void DeferredOffScreenRenderer::RenderScene(RenderList* renderList, Scene* scene)
		{
			BeginScene(scene);

			Begin();

			renderList->RenderOpaqueObjects([&](Entity* obj)
			{
				if (obj != nullptr)
				{
					auto* model = obj->GetComponent<MeshComponent>();
					if (model && model->m_Model)
					{
						auto mesh = model->m_Model;
						if (mesh->GetMaterial())
						{
							if (mesh->GetMaterial()->GetDescriptorSet() == nullptr || mesh->GetMaterial()->GetPipeline() != m_Pipeline)
								mesh->GetMaterial()->CreateDescriptorSet(m_Pipeline, 1);
						}

						TextureMatrixComponent* textureMatrixTransform = obj->GetComponent<TextureMatrixComponent>();
						maths::Matrix4 textureMatrix;
						if (textureMatrixTransform)
							textureMatrix = textureMatrixTransform->m_TextureMatrix;
						else
							textureMatrix = maths::Matrix4();

						auto transform = obj->GetComponent<TransformComponent>()->m_Transform.GetWorldMatrix();

#if 0
						bool inside = true;

						float maxScaling = 0.0f;
						maxScaling = maths::Max(transform.GetScaling().GetX(), maxScaling);
						maxScaling = maths::Max(transform.GetScaling().GetY(), maxScaling);
						maxScaling = maths::Max(transform.GetScaling().GetZ(), maxScaling);

						inside = GraphicsPipeline::Instance()->GetFrustum().InsideFrustum(transform * mesh->GetBoundingSphere()->Centre(), maxScaling * mesh->GetBoundingSphere()->SphereRadius());

						if (inside)
#endif
							SubmitMesh(mesh.get(), transform, textureMatrix);
					}
				}
			});

			SetSystemUniforms(m_Shader);

			Present();

			End();
		}

		void DeferredOffScreenRenderer::PresentToScreen()
		{
			Renderer::Present((m_CommandBuffers[Renderer::GetSwapchain()->GetCurrentBufferId()]));
		}

		void DeferredOffScreenRenderer::Begin()
		{
			m_CommandQueue.clear();
			m_SystemUniforms.clear();

			m_DeferredCommandBuffers->BeginRecording();

			m_RenderPass->BeginRenderpass(m_DeferredCommandBuffers, maths::Vector4(0.0f), m_FBO, graphics::SECONDARY, m_ScreenBufferWidth, m_ScreenBufferHeight);
		}

		void DeferredOffScreenRenderer::BeginScene(Scene* scene)
		{
			auto camera = scene->GetCamera();
			auto proj = camera->GetProjectionMatrix();

			auto projView = proj * camera->GetViewMatrix();
			memcpy(m_VSSystemUniformBuffer + m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionViewMatrix], &projView, sizeof(maths::Matrix4));
		}

		void DeferredOffScreenRenderer::Submit(const RenderCommand& command)
		{
			m_CommandQueue.push_back(command);
		}

		void DeferredOffScreenRenderer::SubmitMesh(Mesh* mesh, const maths::Matrix4& transform, const maths::Matrix4& textureMatrix)
		{
			RenderCommand command;
			command.mesh = mesh;
			command.transform = transform;
			command.textureMatrix = textureMatrix;
			Submit(command);
		}

		void DeferredOffScreenRenderer::EndScene()
		{
		}

		void DeferredOffScreenRenderer::End()
		{
			m_RenderPass->EndRenderpass(m_DeferredCommandBuffers);
			m_DeferredCommandBuffers->EndRecording();
			m_DeferredCommandBuffers->Execute(true);
		}

		void DeferredOffScreenRenderer::SetSystemUniforms(Shader* shader) const
		{
			m_UniformBuffer->SetData(m_VSSystemUniformBufferSize, *&m_VSSystemUniformBuffer);

			int index = 0;

			for (auto& command : m_CommandQueue)
			{
				maths::Matrix4* modelMat = reinterpret_cast<maths::Matrix4*>((reinterpret_cast<uint64_t>(uboDataDynamic.model) + (index * dynamicAlignment)));
				*modelMat = command.transform;
				index++;
			}
			m_ModelUniformBuffer->SetDynamicData(static_cast<uint32_t>(MAX_OBJECTS * dynamicAlignment), sizeof(maths::Matrix4), &*uboDataDynamic.model);
		}

		void DeferredOffScreenRenderer::Present()
		{
			int index = 0;

			for (auto& command : m_CommandQueue)
			{
				Mesh* mesh = command.mesh;

				graphics::CommandBuffer* currentCMDBuffer = mesh->GetCommandBuffer(0);

				currentCMDBuffer->BeginRecordingSecondary(m_RenderPass, m_FBO);
				currentCMDBuffer->UpdateViewport(m_ScreenBufferWidth, m_ScreenBufferHeight);

				m_Pipeline->SetActive(currentCMDBuffer);

				uint32_t dynamicOffset = index * static_cast<uint32_t>(dynamicAlignment);

				Renderer::RenderMesh(mesh, m_Pipeline, currentCMDBuffer, dynamicOffset, m_DefaultDescriptorSet);

				currentCMDBuffer->EndRecording();
				currentCMDBuffer->ExecuteSecondary(m_DeferredCommandBuffers);

				index++;
			}
		}

		void DeferredOffScreenRenderer::CreatePipeline()
		{
			std::vector<graphics::DescriptorPoolInfo> poolInfo =
			{
				{ graphics::DescriptorType::UNIFORM_BUFFER, MAX_OBJECTS },
				{ graphics::DescriptorType::UNIFORM_BUFFER, MAX_OBJECTS },
				{ graphics::DescriptorType::UNIFORM_BUFFER_DYNAMIC, MAX_OBJECTS },
				{ graphics::DescriptorType::IMAGE_SAMPLER, MAX_OBJECTS },
				{ graphics::DescriptorType::IMAGE_SAMPLER, MAX_OBJECTS },
				{ graphics::DescriptorType::IMAGE_SAMPLER, MAX_OBJECTS },
				{ graphics::DescriptorType::IMAGE_SAMPLER, MAX_OBJECTS },
				{ graphics::DescriptorType::IMAGE_SAMPLER, MAX_OBJECTS },
				{ graphics::DescriptorType::IMAGE_SAMPLER, MAX_OBJECTS }
			};

			std::vector<graphics::DescriptorLayoutInfo> layoutInfo =
			{
				{ graphics::DescriptorType::UNIFORM_BUFFER, graphics::ShaderStage::VERTEX, 0 },
				{ graphics::DescriptorType::UNIFORM_BUFFER_DYNAMIC,graphics::ShaderStage::VERTEX , 1 },
			};

			std::vector<graphics::DescriptorLayoutInfo> layoutInfoMesh =
			{
				{ graphics::DescriptorType::IMAGE_SAMPLER,graphics::ShaderStage::FRAGMENT , 0 },
				{ graphics::DescriptorType::IMAGE_SAMPLER,graphics::ShaderStage::FRAGMENT , 1 },
				{ graphics::DescriptorType::IMAGE_SAMPLER,graphics::ShaderStage::FRAGMENT , 2 },
				{ graphics::DescriptorType::IMAGE_SAMPLER,graphics::ShaderStage::FRAGMENT , 3 },
				{ graphics::DescriptorType::IMAGE_SAMPLER,graphics::ShaderStage::FRAGMENT , 4 },
				{ graphics::DescriptorType::IMAGE_SAMPLER,graphics::ShaderStage::FRAGMENT , 5 },
				{ graphics::DescriptorType::UNIFORM_BUFFER, graphics::ShaderStage::FRAGMENT, 6 },
			};

			auto attributeDescriptions = Vertex::getAttributeDescriptions();

			std::vector<graphics::DescriptorLayout> descriptorLayouts;

			graphics::DescriptorLayout sceneDescriptorLayout{};
			sceneDescriptorLayout.count = static_cast<uint>(layoutInfo.size());
			sceneDescriptorLayout.layoutInfo = layoutInfo.data();

			descriptorLayouts.push_back(sceneDescriptorLayout);

			graphics::DescriptorLayout meshDescriptorLayout{};
			meshDescriptorLayout.count = static_cast<uint>(layoutInfoMesh.size());
			meshDescriptorLayout.layoutInfo = layoutInfoMesh.data();

			descriptorLayouts.push_back(meshDescriptorLayout);

			graphics::PipelineInfo pipelineCI{};
			pipelineCI.pipelineName = "OffScreenRenderer";
			pipelineCI.shader = m_Shader;
			pipelineCI.vulkanRenderpass = m_RenderPass;
			pipelineCI.numVertexLayout = static_cast<uint>(attributeDescriptions.size());
			pipelineCI.descriptorLayouts = descriptorLayouts;
			pipelineCI.vertexLayout = attributeDescriptions.data();
			pipelineCI.numLayoutBindings = static_cast<uint>(poolInfo.size());
			pipelineCI.typeCounts = poolInfo.data();
			pipelineCI.strideSize = sizeof(Vertex);
			pipelineCI.numColorAttachments = 6;
			pipelineCI.wireframeEnabled = false;
			pipelineCI.cullMode = graphics::CullMode::BACK;
			pipelineCI.transparencyEnabled = false;
			pipelineCI.depthBiasEnabled = false;
			pipelineCI.width = m_ScreenBufferWidth;
			pipelineCI.height = m_ScreenBufferHeight;
			pipelineCI.maxObjects = MAX_OBJECTS;

			m_Pipeline = graphics::Pipeline::Create(pipelineCI);
		}

		void DeferredOffScreenRenderer::CreateBuffer()
		{
			if (m_UniformBuffer == nullptr)
			{
				m_UniformBuffer = graphics::UniformBuffer::Create();

				uint32_t bufferSize = m_VSSystemUniformBufferSize;
				m_UniformBuffer->Init(bufferSize, nullptr);
			}

			if (m_ModelUniformBuffer == nullptr)
			{
				m_ModelUniformBuffer = graphics::UniformBuffer::Create();
				const size_t minUboAlignment = graphics::GraphicsContext::GetContext()->GetMinUniformBufferOffsetAlignment();

				dynamicAlignment = sizeof(lumos::maths::Matrix4);
				if (minUboAlignment > 0)
				{
					dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
				}

				uint32_t bufferSize2 = static_cast<uint32_t>(MAX_OBJECTS * dynamicAlignment);

				uboDataDynamic.model = static_cast<maths::Matrix4*>(AlignedAlloc(bufferSize2, dynamicAlignment));

				m_ModelUniformBuffer->Init(bufferSize2, nullptr);
			}

			std::vector<graphics::BufferInfo> bufferInfos;

			graphics::BufferInfo bufferInfo = {};
			bufferInfo.buffer = m_UniformBuffer;
			bufferInfo.offset = 0;
			bufferInfo.size = m_VSSystemUniformBufferSize;
			bufferInfo.type = graphics::DescriptorType::UNIFORM_BUFFER;
			bufferInfo.binding = 0;
			bufferInfo.shaderType = ShaderType::VERTEX;
			bufferInfo.systemUniforms = true;

			graphics::BufferInfo bufferInfo2 = {};
			bufferInfo2.buffer = m_ModelUniformBuffer;
			bufferInfo2.offset = 0;
			bufferInfo2.size = sizeof(UniformBufferModel);
			bufferInfo2.type = graphics::DescriptorType::UNIFORM_BUFFER_DYNAMIC;
			bufferInfo2.binding = 1;
			bufferInfo2.shaderType = ShaderType::VERTEX;
			bufferInfo2.systemUniforms = false;

			bufferInfos.push_back(bufferInfo);
			bufferInfos.push_back(bufferInfo2);

			m_Pipeline->GetDescriptorSet()->Update(bufferInfos);
		}

		void DeferredOffScreenRenderer::CreateFBO()
		{
			const uint attachmentCount = 5;
			TextureType attachmentTypes[attachmentCount];
			attachmentTypes[0] = TextureType::COLOUR;
			attachmentTypes[1] = TextureType::COLOUR;
			attachmentTypes[2] = TextureType::COLOUR;
			attachmentTypes[3] = TextureType::COLOUR;
			attachmentTypes[4] = TextureType::DEPTH;

			FramebufferInfo bufferInfo{};
			bufferInfo.width = m_ScreenBufferWidth;
			bufferInfo.height = m_ScreenBufferHeight;
			bufferInfo.attachmentCount = attachmentCount;
			bufferInfo.renderPass = m_RenderPass;
			bufferInfo.attachmentTypes = attachmentTypes;

			Texture* attachments[attachmentCount];
			attachments[0] = Application::Instance()->GetRenderManager()->GetGBuffer()->GetTexture(SCREENTEX_COLOUR);
			attachments[1] = Application::Instance()->GetRenderManager()->GetGBuffer()->GetTexture(SCREENTEX_POSITION);
			attachments[2] = Application::Instance()->GetRenderManager()->GetGBuffer()->GetTexture(SCREENTEX_NORMALS);
			attachments[3] = Application::Instance()->GetRenderManager()->GetGBuffer()->GetTexture(SCREENTEX_PBR);
			attachments[4] = Application::Instance()->GetRenderManager()->GetGBuffer()->GetDepthTexture();
			bufferInfo.attachments = attachments;

			m_FBO = Framebuffer::Create(bufferInfo);
		}

		void DeferredOffScreenRenderer::OnResize(uint width, uint height)
		{
			delete m_Pipeline;
			delete m_FBO;

			if (m_RenderToGBufferTexture)
				m_RenderTexture = Application::Instance()->GetRenderManager()->GetGBuffer()->GetTexture(SCREENTEX_OFFSCREEN0);

			DeferredOffScreenRenderer::SetScreenBufferSize(width, height);

			CreatePipeline();
			CreateBuffer();
			CreateFBO();
			CreateDefaultDescriptorSet();

			m_ClearColour = maths::Vector4(0.8f, 0.8f, 0.8f, 1.0f);
		}

		void DeferredOffScreenRenderer::CreateDefaultDescriptorSet()
		{
			if (m_DefaultDescriptorSet)
				delete m_DefaultDescriptorSet;

			graphics::DescriptorInfo info{};
			info.pipeline = m_Pipeline;
			info.layoutIndex = 1;
			info.shader = m_Shader;
			m_DefaultDescriptorSet = graphics::DescriptorSet::Create(info);

			std::vector<graphics::ImageInfo> imageInfos;

			graphics::ImageInfo imageInfo = {};
			imageInfo.texture = { m_DefaultTexture };
			imageInfo.binding = 0;
			imageInfo.name = "u_AlbedoMap";

			imageInfos.push_back(imageInfo);

			std::vector<graphics::BufferInfo> bufferInfos;

			graphics::BufferInfo bufferInfo = {};
			bufferInfo.buffer = m_DefaultMaterialDataUniformBuffer;
			bufferInfo.offset = 0;
			bufferInfo.size = sizeof(MaterialProperties);
			bufferInfo.type = graphics::DescriptorType::UNIFORM_BUFFER;
			bufferInfo.binding = 6;
			bufferInfo.shaderType = ShaderType::FRAGMENT;
			bufferInfo.name = "UniformMaterialData";
			bufferInfo.systemUniforms = false;

			bufferInfos.push_back(bufferInfo);

			m_DefaultDescriptorSet->Update(imageInfos, bufferInfos);
		}
	}
}
