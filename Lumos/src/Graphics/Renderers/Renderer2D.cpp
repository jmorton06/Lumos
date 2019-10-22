#include "lmpch.h"
#include "Renderer2D.h"
#include "Graphics/API/Shader.h"
#include "Graphics/RenderList.h"
#include "Graphics/API/Framebuffer.h"
#include "Graphics/ModelLoader/ModelLoader.h"
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
#include "ECS/EntityManager.h"
#include "ECS/Component/MaterialComponent.h"
#include "App/Application.h"
#include "Graphics/RenderManager.h"
#include "Platform/OpenGL/GLDescriptorSet.h"
#include "Graphics/Renderable2D.h"
#include "Graphics/Camera/Camera.h"

#define RENDERER_MAX_SPRITES	10000
#define RENDERER_SPRITE_SIZE	RENDERER2D_VERTEX_SIZE * 4
#define RENDERER_BUFFER_SIZE	RENDERER_SPRITE_SIZE * RENDERER_MAX_SPRITES
#define RENDERER_INDICES_SIZE	RENDERER_MAX_SPRITES * 6
#define RENDERER_MAX_TEXTURES	16 - 1
#define MAX_BATCH_DRAW_CALLS	100

namespace Lumos
{
	namespace Graphics
	{
		Renderer2D::Renderer2D(u32 width, u32 height, bool renderToGBuffer) : m_IndexCount(0), m_RenderTexture(nullptr), m_Buffer(nullptr)
		{
			SetScreenBufferSize(width, height);

			Init();
            
            SetRenderToGBufferTexture(renderToGBuffer);
		}

		Renderer2D::~Renderer2D()
		{
			delete m_IndexBuffer;
			delete m_Pipeline;
			delete m_DescriptorSet;
			delete m_RenderPass;
			delete m_Shader;
			delete m_UniformBuffer;

			delete[] m_VSSystemUniformBuffer;

			for (auto frameBuffer : m_Framebuffers)
				delete frameBuffer;

			for (int i = 0; i < MAX_BATCH_DRAW_CALLS; i++)
				delete m_VertexArrays[i];

			for (int i = 0; i < MAX_BATCH_DRAW_CALLS; i++)
				delete m_SecondaryCommandBuffers[i];

			for (auto& commandBuffer : m_CommandBuffers)
			{
				delete commandBuffer;
			}
		}

		void Renderer2D::Init()
		{
			m_Shader = Shader::CreateFromFile("Batch2D", "/CoreShaders/");

			m_TransformationStack.emplace_back(Maths::Matrix4());
			m_TransformationBack = &m_TransformationStack.back();

			m_VSSystemUniformBufferSize = sizeof(Maths::Matrix4);
			m_VSSystemUniformBuffer = lmnew u8[m_VSSystemUniformBufferSize];

			m_RenderPass = Graphics::RenderPass::Create();
			m_UniformBuffer = Graphics::UniformBuffer::Create();

			AttachmentInfo textureTypes[2] =
			{
				{ TextureType::COLOUR, TextureFormat::RGBA8 }
			};

			Graphics::RenderpassInfo renderpassCI{};
			renderpassCI.attachmentCount = 1;
			renderpassCI.textureType = textureTypes;
			renderpassCI.clear = true;

			m_RenderPass->Init(renderpassCI);

			CreateFramebuffers();

			m_CommandBuffers.resize(2);

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

			Graphics::BufferInfo bufferInfo = {};
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

			Graphics::DescriptorInfo info{};
			info.pipeline = m_Pipeline;
			info.layoutIndex = 1; //?
			info.shader = m_Shader;
			m_DescriptorSet = Graphics::DescriptorSet::Create(info);

			Graphics::BufferLayout layout;
			layout.Push<Maths::Vector3>("POSITION"); // Position
			layout.Push<Maths::Vector2>("TEXCOORD"); // UV
			layout.Push<Maths::Vector2>("ID"); // Texture Index
			layout.Push<Maths::Vector4>("COLOUR"); // Colour

			m_VertexArrays.resize(MAX_BATCH_DRAW_CALLS);

			for (auto& vertexArray : m_VertexArrays)
			{
				VertexBuffer* buffer = VertexBuffer::Create(BufferUsage::DYNAMIC);
				buffer->Resize(RENDERER_BUFFER_SIZE);
				buffer->SetLayout(layout);
				vertexArray = Graphics::VertexArray::Create();
				vertexArray->PushBuffer(buffer);
			}

			u32* indices = lmnew u32[RENDERER_INDICES_SIZE];

			i32 offset = 0;
			for (i32 i = 0; i < RENDERER_INDICES_SIZE; i += 6)
			{
				indices[i] = offset + 0;
				indices[i + 1] = offset + 1;
				indices[i + 2] = offset + 2;

				indices[i + 3] = offset + 2;
				indices[i + 4] = offset + 3;
				indices[i + 5] = offset + 0;

				offset += 4;
			}

			m_IndexBuffer = IndexBuffer::Create(indices, RENDERER_INDICES_SIZE);

			delete[] indices;

			m_ClearColour = Maths::Vector4(0.2f, 0.2f, 0.2f, 1.0f);
		}

		void Renderer2D::Submit(Renderable2D* renderable, const Maths::Matrix4& transform)
		{
			const Maths::Vector2 min = renderable->GetPosition();
			const Maths::Vector2 max = renderable->GetPosition() + renderable->GetScale();

			const Maths::Vector4 colour = renderable->GetColour();
			const std::vector<Maths::Vector2>& uv = renderable->GetUVs();
			const Texture* texture = renderable->GetTexture();

			float textureSlot = 0.0f;
			if (texture)
				textureSlot = SubmitTexture(renderable->GetTexture());

			Maths::Vector3 vertex = transform * Maths::Vector3(min.x, min.y, 0.0f);
			m_Buffer->vertex = vertex;
			m_Buffer->uv = uv[0];
			m_Buffer->tid = textureSlot;
			m_Buffer->color = colour;
			m_Buffer++;

			vertex = transform * Maths::Vector3(max.x, min.y, 0.0f);
			m_Buffer->vertex = vertex;
			m_Buffer->uv = uv[1];
			m_Buffer->tid = textureSlot;
			m_Buffer->color = colour;
			m_Buffer++;

			vertex = transform * Maths::Vector3(max.x, max.y, 0.0f);
			m_Buffer->vertex = vertex;
			m_Buffer->uv = uv[2];
			m_Buffer->tid = textureSlot;
			m_Buffer->color = colour;
			m_Buffer++;

			vertex = transform * Maths::Vector3(min.x, max.y, 0.0f);
			m_Buffer->vertex = vertex;
			m_Buffer->uv = uv[3];
			m_Buffer->tid = textureSlot;
			m_Buffer->color = colour;
			m_Buffer++;

			m_IndexCount += 6;
		}

		void Renderer2D::Begin()
		{
			m_CurrentBufferID = 0;
			if (!m_RenderTexture)
				m_CurrentBufferID = Renderer::GetSwapchain()->GetCurrentBufferId();

			m_CommandBuffers[m_CurrentBufferID]->BeginRecording();

			m_RenderPass->BeginRenderpass(m_CommandBuffers[m_CurrentBufferID], m_ClearColour, m_Framebuffers[m_CurrentBufferID], Graphics::SECONDARY, m_ScreenBufferWidth, m_ScreenBufferHeight);

			m_Textures.clear();
			m_Sprites.clear();

			m_VertexArrays[m_BatchDrawCallIndex]->Bind(m_CommandBuffers[m_CurrentBufferID]);
			m_Buffer = m_VertexArrays[m_BatchDrawCallIndex]->GetBuffer()->GetPointer<VertexData>();
		}

		void Renderer2D::SetSystemUniforms(Shader* shader) const
		{
			shader->SetSystemUniformBuffer(ShaderType::VERTEX, m_VSSystemUniformBuffer, m_VSSystemUniformBufferSize, 0);

			m_UniformBuffer->SetData(sizeof(UniformBufferObject), *&m_VSSystemUniformBuffer);
		}

		void Renderer2D::BeginScene(Scene* scene)
		{
			auto camera = scene->GetCamera();
			auto projView = camera->GetProjectionMatrix() * camera->GetViewMatrix();

			memcpy(m_VSSystemUniformBuffer, &projView, sizeof(Maths::Matrix4));
		}

		void Renderer2D::Present()
		{
			UpdateDesciptorSet();

			Graphics::CommandBuffer* currentCMDBuffer = m_SecondaryCommandBuffers[m_BatchDrawCallIndex];

			currentCMDBuffer->BeginRecordingSecondary(m_RenderPass, m_Framebuffers[m_CurrentBufferID]);
			currentCMDBuffer->UpdateViewport(m_ScreenBufferWidth, m_ScreenBufferHeight);
			m_Pipeline->SetActive(currentCMDBuffer);

			m_VertexArrays[m_BatchDrawCallIndex]->GetBuffer()->ReleasePointer();
			m_VertexArrays[m_BatchDrawCallIndex]->Unbind();

			m_IndexBuffer->SetCount(m_IndexCount);

			std::vector<Graphics::DescriptorSet*> descriptors = { m_Pipeline->GetDescriptorSet(), m_DescriptorSet };

			m_VertexArrays[m_BatchDrawCallIndex]->Bind(currentCMDBuffer);
			m_IndexBuffer->Bind(currentCMDBuffer);

			Renderer::BindDescriptorSets(m_Pipeline, currentCMDBuffer, 0, descriptors);
			Renderer::DrawIndexed(currentCMDBuffer, DrawType::TRIANGLE, m_IndexCount);

			m_VertexArrays[m_BatchDrawCallIndex]->Unbind();
			m_IndexBuffer->Unbind();

			m_IndexCount = 0;

			currentCMDBuffer->EndRecording();
			currentCMDBuffer->ExecuteSecondary(m_CommandBuffers[m_CurrentBufferID]);

			m_BatchDrawCallIndex++;
		}

		void Renderer2D::End()
		{
			m_RenderPass->EndRenderpass(m_CommandBuffers[m_CurrentBufferID]);
			m_CommandBuffers[m_CurrentBufferID]->EndRecording();

			if (m_RenderTexture)
				m_CommandBuffers[m_CurrentBufferID]->Execute(true);

			if (!m_RenderTexture)
				PresentToScreen();

			m_BatchDrawCallIndex = 0;
		}

		void Renderer2D::Render(Scene* scene)
		{
			Begin();

			SetSystemUniforms(m_Shader);

            auto entities = EntityManager::Instance()->GetEntitiesWithType<Graphics::Sprite>();
			for(auto& obj : entities)
			{
				if (obj != nullptr)
				{
					auto* sprite = obj->GetComponent<Graphics::Sprite>();
					if (sprite)
					{
						Submit(reinterpret_cast<Renderable2D*>(sprite), obj->GetTransformComponent()->GetWorldMatrix());
					}
				}
			};

			Present();

			End();
		}

		float Renderer2D::SubmitTexture(Texture* texture)
		{
			float result = 0.0f;
			bool found = false;
			for (u32 i = 0; i < m_Textures.size(); i++)
			{
				if (m_Textures[i] == texture) //Temp
				{
					result = static_cast<float>(i + 1);
					found = true;
					break;
				}
			}

			if (!found)
			{
				if (m_Textures.size() >= RENDERER_MAX_TEXTURES)
				{
					Present();

					m_Textures.clear();
					m_Sprites.clear();

					m_VertexArrays[m_BatchDrawCallIndex]->Bind();
					m_Buffer = m_VertexArrays[m_BatchDrawCallIndex]->GetBuffer()->GetPointer<VertexData>();
				}
				m_Textures.push_back(texture);
				result = static_cast<float>(m_Textures.size());
			}
			return result;
		}

		void Renderer2D::OnResize(u32 width, u32 height)
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

			Graphics::BufferInfo bufferInfo = {};
			bufferInfo.buffer = m_UniformBuffer;
			bufferInfo.offset = 0;
			bufferInfo.size = sizeof(UniformBufferObject);
			bufferInfo.type = Graphics::DescriptorType::UNIFORM_BUFFER;
			bufferInfo.shaderType = ShaderType::VERTEX;
			bufferInfo.systemUniforms = false;
			bufferInfo.name = "UniformBufferObject";
			bufferInfo.binding = 0;

			bufferInfos.push_back(bufferInfo);

			delete m_DescriptorSet; Graphics::DescriptorInfo info{};
			info.pipeline = m_Pipeline;
			info.layoutIndex = 1; //?
			info.shader = m_Shader;
			m_DescriptorSet = Graphics::DescriptorSet::Create(info);

			m_Pipeline->GetDescriptorSet()->Update(bufferInfos);

			CreateFramebuffers();
		}

		void Renderer2D::PresentToScreen()
		{
			Renderer::Present((m_CommandBuffers[Renderer::GetSwapchain()->GetCurrentBufferId()]));
		}

		void Renderer2D::SetScreenBufferSize(u32 width, u32 height)
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

		void Renderer2D::CreateGraphicsPipeline()
		{
			std::vector<Graphics::DescriptorPoolInfo> poolInfo =
			{
				{ Graphics::DescriptorType::UNIFORM_BUFFER, MAX_BATCH_DRAW_CALLS },
				{ Graphics::DescriptorType::IMAGE_SAMPLER, MAX_BATCH_DRAW_CALLS }
			};

			std::vector<Graphics::DescriptorLayoutInfo> layoutInfo =
			{
				{ Graphics::DescriptorType::UNIFORM_BUFFER, Graphics::ShaderType::VERTEX, 0 }
			};

			std::vector<Graphics::DescriptorLayoutInfo> layoutInfoMesh =
			{
				 { Graphics::DescriptorType::IMAGE_SAMPLER,Graphics::ShaderType::FRAGMENT , 0, RENDERER_MAX_TEXTURES }
			};

			auto attributeDescriptions = VertexData::getAttributeDescriptions();

			std::vector<Graphics::DescriptorLayout> descriptorLayouts;

			Graphics::DescriptorLayout sceneDescriptorLayout{};
			sceneDescriptorLayout.count = static_cast<u32>(layoutInfo.size());
			sceneDescriptorLayout.layoutInfo = layoutInfo.data();

			descriptorLayouts.push_back(sceneDescriptorLayout);

			Graphics::DescriptorLayout meshDescriptorLayout{};
			meshDescriptorLayout.count = static_cast<u32>(layoutInfoMesh.size());
			meshDescriptorLayout.layoutInfo = layoutInfoMesh.data();

			descriptorLayouts.push_back(meshDescriptorLayout);

			Graphics::PipelineInfo pipelineCI{};
			pipelineCI.pipelineName = "Batch2DRenderer";
			pipelineCI.shader = m_Shader;
			pipelineCI.renderpass = m_RenderPass;
			pipelineCI.numVertexLayout = static_cast<u32>(attributeDescriptions.size());
			pipelineCI.descriptorLayouts = descriptorLayouts;
			pipelineCI.vertexLayout = attributeDescriptions.data();
			pipelineCI.numLayoutBindings = static_cast<u32>(poolInfo.size());
			pipelineCI.typeCounts = poolInfo.data();
			pipelineCI.strideSize = sizeof(VertexData);
			pipelineCI.numColorAttachments = 1;
			pipelineCI.wireframeEnabled = false;
			pipelineCI.cullMode = Graphics::CullMode::BACK;
			pipelineCI.transparencyEnabled = false;
			pipelineCI.depthBiasEnabled = false;
			pipelineCI.width = m_ScreenBufferWidth;
			pipelineCI.height = m_ScreenBufferHeight;
			pipelineCI.maxObjects = MAX_BATCH_DRAW_CALLS;

			m_Pipeline = Graphics::Pipeline::Create(pipelineCI);
		}

		void Renderer2D::CreateFramebuffers()
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

		void Renderer2D::UpdateDesciptorSet()
		{
			if (m_Textures.empty())
				return;
			std::vector<Graphics::ImageInfo> imageInfos;

			Graphics::ImageInfo imageInfo = {};

			imageInfo.binding = 0;
			imageInfo.name = "textures";
			imageInfo.count = 0;
			imageInfo.texture = m_Textures;
			imageInfo.count = static_cast<int>(m_Textures.size());

			imageInfos.push_back(imageInfo);

			m_DescriptorSet->Update(imageInfos);
		}

		void Renderer2D::SetRenderTarget(Texture* texture)
		{
			m_RenderTexture = texture;

			for (auto fbo : m_Framebuffers)
				delete fbo;
			m_Framebuffers.clear();

			CreateFramebuffers();
		}

		void Renderer2D::SetRenderToGBufferTexture(bool set)
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
	}
}
