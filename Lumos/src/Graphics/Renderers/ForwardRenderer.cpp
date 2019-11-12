#include "lmpch.h"
#include "ForwardRenderer.h"
#include "Graphics/API/Shader.h"
#include "Graphics/API/Framebuffer.h"
#include "Graphics/Light.h"
#include "Graphics/API/Texture.h"
#include "Graphics/API/UniformBuffer.h"
#include "Graphics/ModelLoader/ModelLoader.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"
#include "Graphics/API/Renderer.h"
#include "Graphics/API/CommandBuffer.h"
#include "Graphics/API/Swapchain.h"
#include "Graphics/API/RenderPass.h"
#include "Graphics/API/Pipeline.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/GBuffer.h"
#include "App/Scene.h"
#include "ECS/EntityManager.h"
#include "ECS/Component/MaterialComponent.h"
#include "ECS/Component/MeshComponent.h"
#include "ECS/Component/TextureMatrixComponent.h"

#include "App/Application.h"
#include "Graphics/RenderManager.h"
#include "Graphics/Camera/Camera.h"

namespace Lumos
{
	namespace Graphics
	{
		ForwardRenderer::ForwardRenderer(u32 width, u32 height, bool renderToGBuffer)
		{
			SetScreenBufferSize(width, height);
			ForwardRenderer::Init();
            SetRenderToGBufferTexture(renderToGBuffer);
		}

		ForwardRenderer::~ForwardRenderer()
		{
			delete m_Shader;
			delete m_FBO;
			delete m_DefaultTexture;
			delete m_UniformBuffer;

            Memory::AlignedFree(m_UBODataDynamic.model);

			delete m_ModelUniformBuffer;
			delete m_RenderPass;
			delete m_Pipeline;
			delete m_DescriptorSet;

			delete[] m_VSSystemUniformBuffer;
			delete[] m_PSSystemUniformBuffer;

			for (auto& framebuffer : m_Framebuffers)
			{
				delete framebuffer;
			}

			for (auto& commandBuffer : m_CommandBuffers)
			{
				delete commandBuffer;
			}
		}

		void ForwardRenderer::RenderScene(Scene* scene)
		{
			//for (i = 0; i < commandBuffers.size(); i++)
			{
				m_CurrentBufferID = 0;
				if (!m_RenderTexture)
					m_CurrentBufferID = Renderer::GetSwapchain()->GetCurrentBufferId();

				Begin();

                auto& registry = scene->GetRegistry();
                
                auto group = registry.group<MeshComponent>(entt::get<Maths::Transform>);

                for(auto entity : group)
                {
                    const auto &[mesh, trans] = group.get<MeshComponent, Maths::Transform>(entity);

                    if (mesh.GetMesh() && mesh.GetMesh()->GetActive())
                    {
                        auto& worldTransform = trans.GetWorldMatrix();

                        float maxScaling = 0.0f;
                        auto scale = worldTransform.GetScaling();
                        maxScaling = Maths::Max(scale.GetX(), maxScaling);
                        maxScaling = Maths::Max(scale.GetY(), maxScaling);
                        maxScaling = Maths::Max(scale.GetZ(), maxScaling);

                        bool inside = m_Frustum.InsideFrustum(worldTransform.GetPositionVector(), maxScaling * mesh.GetMesh()->GetBoundingBox()->SphereRadius());

                        if (!inside)
                            continue;

                        auto meshPtr = mesh.GetMesh();
                        auto materialComponent = registry.try_get<MaterialComponent>(entity);
                        Material* material = nullptr;
                        if (materialComponent && /* materialComponent->GetActive() &&*/ materialComponent->GetMaterial())
                        {
                            material = materialComponent->GetMaterial().get();

                            if (material->GetDescriptorSet() == nullptr || material->GetPipeline() != m_Pipeline)
                                material->CreateDescriptorSet(m_Pipeline, 1);
                        }

                        auto textureMatrixTransform = registry.try_get<TextureMatrixComponent>(entity);
                        Maths::Matrix4 textureMatrix;
                        if (textureMatrixTransform)
                            textureMatrix = textureMatrixTransform->GetMatrix();
                        else
                            textureMatrix = Maths::Matrix4();

                        SubmitMesh(meshPtr, material, worldTransform, textureMatrix);
                    }
                }

				SetSystemUniforms(m_Shader);

				Present();

				EndScene();
				End();
			}

			if (!m_RenderTexture)
				Renderer::Present((m_CommandBuffers[Renderer::GetSwapchain()->GetCurrentBufferId()]));
		}

		enum VSSystemUniformIndices : i32
		{
			VSSystemUniformIndex_ProjectionMatrix = 0,
			VSSystemUniformIndex_ViewMatrix = 1,
			VSSystemUniformIndex_ModelMatrix = 2,
			VSSystemUniformIndex_TextureMatrix = 3,
			VSSystemUniformIndex_Size
		};

		enum PSSystemUniformIndices : i32
		{
			PSSystemUniformIndex_Lights = 0,
			PSSystemUniformIndex_Size
		};

		void ForwardRenderer::Init()
		{
			m_CommandQueue.reserve(1000);

			//
			// Vertex shader System uniforms
			//
			m_VSSystemUniformBufferSize = sizeof(Maths::Matrix4) + sizeof(Maths::Matrix4) + sizeof(Maths::Matrix4) + sizeof(Maths::Matrix4);
			m_VSSystemUniformBuffer = lmnew u8[m_VSSystemUniformBufferSize];
			memset(m_VSSystemUniformBuffer, 0, m_VSSystemUniformBufferSize);
			m_VSSystemUniformBufferOffsets.resize(VSSystemUniformIndex_Size);

			// Per Scene System Uniforms
			m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionMatrix] = 0;
			m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ViewMatrix] = m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionMatrix] + sizeof(Maths::Matrix4);
			m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ModelMatrix] = m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ViewMatrix] + sizeof(Maths::Matrix4);
			m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_TextureMatrix] = m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ModelMatrix] + sizeof(Maths::Matrix4);

			// Pixel/fragment shader System uniforms
			m_PSSystemUniformBufferSize = sizeof(Graphics::Light);
			m_PSSystemUniformBuffer = lmnew u8[m_PSSystemUniformBufferSize];
			memset(m_PSSystemUniformBuffer, 0, m_PSSystemUniformBufferSize);
			m_PSSystemUniformBufferOffsets.resize(PSSystemUniformIndex_Size);

			// Per Scene System Uniforms
			m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_Lights] = 0;

			m_RenderPass = Graphics::RenderPass::Create();
			m_UniformBuffer = Graphics::UniformBuffer::Create();
			m_ModelUniformBuffer = Graphics::UniformBuffer::Create();

			AttachmentInfo textureTypes[2] =
			{
				{ TextureType::COLOUR, TextureFormat::RGBA8 },
				{ TextureType::DEPTH , TextureFormat::DEPTH }
			};

			Graphics::RenderpassInfo renderpassCI{};
			renderpassCI.attachmentCount = 2;
			renderpassCI.textureType = textureTypes;

			m_RenderPass->Init(renderpassCI);

			m_FBO = nullptr;

			CreateFramebuffers();

			m_CommandBuffers.resize(2);

			for (auto& commandBuffer : m_CommandBuffers)
			{
				commandBuffer = Graphics::CommandBuffer::Create();
				commandBuffer->Init(true);
			}

			CreateGraphicsPipeline();

			uint32_t bufferSize = static_cast<uint32_t>(sizeof(UniformBufferObject));
			m_UniformBuffer->Init(bufferSize, nullptr);

			const size_t minUboAlignment = Graphics::GraphicsContext::GetContext()->GetMinUniformBufferOffsetAlignment();

			m_DynamicAlignment = sizeof(Lumos::Maths::Matrix4);
			if (minUboAlignment > 0)
			{
				m_DynamicAlignment = (m_DynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
			}

			uint32_t bufferSize2 = static_cast<uint32_t>(MAX_OBJECTS * m_DynamicAlignment);

            m_UBODataDynamic.model = static_cast<Maths::Matrix4*>(Memory::AlignedAlloc(bufferSize2, m_DynamicAlignment));

			m_ModelUniformBuffer->Init(bufferSize2, nullptr);

			std::vector<Graphics::BufferInfo> bufferInfos;

			Graphics::BufferInfo bufferInfo = {};
			bufferInfo.buffer = m_UniformBuffer;
			bufferInfo.offset = 0;
			bufferInfo.size = sizeof(UniformBufferObject);
			bufferInfo.type = Graphics::DescriptorType::UNIFORM_BUFFER;
			bufferInfo.binding = 0;

			Graphics::BufferInfo bufferInfo2 = {};
			bufferInfo2.buffer = m_ModelUniformBuffer;
			bufferInfo2.offset = 0;
			bufferInfo2.size = sizeof(UniformBufferModel);
			bufferInfo2.type = Graphics::DescriptorType::UNIFORM_BUFFER_DYNAMIC;
			bufferInfo2.binding = 1;

			bufferInfos.push_back(bufferInfo);
			bufferInfos.push_back(bufferInfo2);

			m_Pipeline->GetDescriptorSet()->Update(bufferInfos);

			m_ClearColour = Maths::Vector4(0.8f, 0.8f, 0.8f, 1.0f);

			m_DefaultTexture = Texture2D::CreateFromFile("Test", "/CoreTextures/checkerboard.tga");

			Graphics::DescriptorInfo info{};
			info.pipeline = m_Pipeline;
			info.layoutIndex = 1;
			info.shader = m_Shader;
			m_DescriptorSet = Graphics::DescriptorSet::Create(info);

			std::vector<Graphics::ImageInfo> bufferInfosDefault;

			Graphics::ImageInfo imageInfo = {};
			imageInfo.texture = { m_DefaultTexture };
			imageInfo.binding = 0;
			imageInfo.name = "texSampler";

			bufferInfosDefault.push_back(imageInfo);

			m_DescriptorSet->Update(bufferInfosDefault);
		}

		void ForwardRenderer::Begin()
		{
			m_CommandQueue.clear();
			m_SystemUniforms.clear();

			m_CommandBuffers[m_CurrentBufferID]->BeginRecording();

			m_RenderPass->BeginRenderpass(m_CommandBuffers[m_CurrentBufferID], m_ClearColour, m_Framebuffers[m_CurrentBufferID], Graphics::INLINE, m_ScreenBufferWidth, m_ScreenBufferHeight);
		}

		void ForwardRenderer::BeginScene(Scene* scene)
		{
			auto camera = scene->GetCamera();
			auto proj = camera->GetProjectionMatrix();

			memcpy(m_VSSystemUniformBuffer + m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionMatrix], &proj, sizeof(Maths::Matrix4));
			memcpy(m_VSSystemUniformBuffer + m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ViewMatrix], &camera->GetViewMatrix(), sizeof(Maths::Matrix4));

			m_Frustum.FromMatrix(proj * camera->GetViewMatrix());
		}

		void ForwardRenderer::Submit(const RenderCommand& command)
		{
			m_CommandQueue.push_back(command);
		}

		void ForwardRenderer::SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix)
		{
			RenderCommand command;
			command.mesh = mesh;
			command.transform = transform;
			command.textureMatrix = textureMatrix;
			command.material = material;
			Submit(command);
		}

		void ForwardRenderer::EndScene()
		{
		}

		void ForwardRenderer::End()
		{
			m_RenderPass->EndRenderpass(m_CommandBuffers[m_CurrentBufferID]);
			m_CommandBuffers[m_CurrentBufferID]->EndRecording();

			if (m_RenderTexture)
				m_CommandBuffers[m_CurrentBufferID]->Execute(true);
		}

		void ForwardRenderer::SetSystemUniforms(Shader* shader) const
		{
			shader->SetSystemUniformBuffer(ShaderType::VERTEX, m_VSSystemUniformBuffer, m_VSSystemUniformBufferSize, 0);

			m_UniformBuffer->SetData(sizeof(UniformBufferObject), *&m_VSSystemUniformBuffer);

			int index = 0;

			for (auto& command : m_CommandQueue)
			{
				Maths::Matrix4* modelMat = reinterpret_cast<Maths::Matrix4*>((reinterpret_cast<uint64_t>(m_UBODataDynamic.model) + (index * m_DynamicAlignment)));
				*modelMat = command.transform;
				index++;
			}

			shader->SetSystemUniformBuffer(ShaderType::FRAGMENT, m_PSSystemUniformBuffer, m_PSSystemUniformBufferSize, 0);

			m_ModelUniformBuffer->SetDynamicData(static_cast<uint32_t>(MAX_OBJECTS * m_DynamicAlignment), sizeof(Maths::Matrix4), &*m_UBODataDynamic.model);
		}

		void ForwardRenderer::InitScene(Scene* scene)
		{
		}

		void ForwardRenderer::Present()
		{
			int index = 0;

			for (auto& command : m_CommandQueue)
			{
				Mesh* mesh = command.mesh;

				Graphics::CommandBuffer* currentCMDBuffer = m_CommandBuffers[m_CurrentBufferID];// (mesh->GetCommandBuffer(static_cast<int>(m_CurrentBufferID)));

				//currentCMDBuffer->BeginRecordingSecondary(m_RenderPass, m_Framebuffers[m_CurrentBufferID]);
				//currentCMDBuffer->UpdateViewport(m_ScreenBufferWidth, m_ScreenBufferHeight);

				m_Pipeline->SetActive(currentCMDBuffer);

				uint32_t dynamicOffset = index * static_cast<uint32_t>(m_DynamicAlignment);

				m_Shader->SetUserUniformBuffer(ShaderType::VERTEX, reinterpret_cast<u8*>(m_ModelUniformBuffer->GetBuffer()) + dynamicOffset, sizeof(Maths::Matrix4));

				std::vector<Graphics::DescriptorSet*> descriptorSets;
				descriptorSets.emplace_back(m_Pipeline->GetDescriptorSet());
				descriptorSets.emplace_back(m_DescriptorSet);

				mesh->GetVertexArray()->Bind(currentCMDBuffer);
				mesh->GetIndexBuffer()->Bind(currentCMDBuffer);

				Renderer::BindDescriptorSets(m_Pipeline, currentCMDBuffer, dynamicOffset, descriptorSets);
				Renderer::DrawIndexed(currentCMDBuffer, DrawType::TRIANGLE, mesh->GetIndexBuffer()->GetCount());

				mesh->GetVertexArray()->Unbind();
				mesh->GetIndexBuffer()->Unbind();

				//currentCMDBuffer->EndRecording();
				//currentCMDBuffer->ExecuteSecondary(m_CommandBuffers[m_CurrentBufferID]);

				index++;
			}
		}

		void ForwardRenderer::SetRenderToGBufferTexture(bool set)
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

		void ForwardRenderer::OnResize(u32 width, u32 height)
		{
			m_ScreenBufferWidth = static_cast<u32>(width);
			m_ScreenBufferHeight = static_cast<u32>(height);

			delete m_Pipeline;
			delete m_RenderPass;
			delete m_Shader;

			for (auto fbo : m_Framebuffers)
				delete fbo;

			m_Framebuffers.clear();

			if (m_RenderToGBufferTexture)
				m_RenderTexture = Application::Instance()->GetRenderManager()->GetGBuffer()->GetTexture(SCREENTEX_OFFSCREEN0);

			m_RenderPass = Graphics::RenderPass::Create();

			AttachmentInfo textureTypes[2] =
			{
				{ TextureType::COLOUR, TextureFormat::RGBA8 },
				{ TextureType::DEPTH , TextureFormat::DEPTH }
			};

			Graphics::RenderpassInfo renderpassCI{};
			renderpassCI.attachmentCount = 2;
			renderpassCI.textureType = textureTypes;
			m_RenderPass->Init(renderpassCI);

			CreateGraphicsPipeline();

			CreateFramebuffers();

			std::vector<Graphics::BufferInfo> bufferInfos;

			Graphics::BufferInfo bufferInfo = {};
			bufferInfo.buffer = m_UniformBuffer;
			bufferInfo.offset = 0;
			bufferInfo.size = sizeof(UniformBufferObject);
			bufferInfo.type = Graphics::DescriptorType::UNIFORM_BUFFER;
			bufferInfo.binding = 0;

			Graphics::BufferInfo bufferInfo2 = {};
			bufferInfo2.buffer = m_ModelUniformBuffer;
			bufferInfo2.offset = 0;
			bufferInfo2.size = sizeof(UniformBufferModel);
			bufferInfo2.type = Graphics::DescriptorType::UNIFORM_BUFFER_DYNAMIC;
			bufferInfo2.binding = 1;

			bufferInfos.push_back(bufferInfo);
			bufferInfos.push_back(bufferInfo2);

			m_Pipeline->GetDescriptorSet()->Update(bufferInfos);

		}

		void ForwardRenderer::CreateGraphicsPipeline()
		{
			m_Shader = Shader::CreateFromFile("Simple", "/CoreShaders/");

			std::vector<Graphics::DescriptorPoolInfo> poolInfo =
			{
				{ Graphics::DescriptorType::UNIFORM_BUFFER, MAX_OBJECTS },
				{ Graphics::DescriptorType::UNIFORM_BUFFER_DYNAMIC, MAX_OBJECTS },
				{ Graphics::DescriptorType::IMAGE_SAMPLER, MAX_OBJECTS }
			};

			std::vector<Graphics::DescriptorLayoutInfo> layoutInfo =
			{
				{ Graphics::DescriptorType::UNIFORM_BUFFER, Graphics::ShaderType::VERTEX, 0 },
				{ Graphics::DescriptorType::UNIFORM_BUFFER_DYNAMIC,Graphics::ShaderType::VERTEX , 1 },
			};

			std::vector<Graphics::DescriptorLayoutInfo> layoutInfoMesh =
			{
				 { Graphics::DescriptorType::IMAGE_SAMPLER,Graphics::ShaderType::FRAGMENT , 0 }
			};

			auto attributeDescriptions = Vertex::getAttributeDescriptions();

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
			pipelineCI.pipelineName = "ForwardRenderer";
			pipelineCI.shader = m_Shader;
			pipelineCI.renderpass = m_RenderPass;
			pipelineCI.numVertexLayout = static_cast<u32>(attributeDescriptions.size());
			pipelineCI.descriptorLayouts = descriptorLayouts;
			pipelineCI.vertexLayout = attributeDescriptions.data();
			pipelineCI.numLayoutBindings = static_cast<u32>(poolInfo.size());
			pipelineCI.typeCounts = poolInfo.data();
			pipelineCI.strideSize = sizeof(Vertex);
			pipelineCI.numColorAttachments = 1;
			pipelineCI.wireframeEnabled = false;
			pipelineCI.cullMode = Graphics::CullMode::BACK;
			pipelineCI.transparencyEnabled = false;
			pipelineCI.depthBiasEnabled = false;
			pipelineCI.width = m_ScreenBufferWidth;
			pipelineCI.height = m_ScreenBufferHeight;
			pipelineCI.maxObjects = MAX_OBJECTS;

			m_Pipeline = Graphics::Pipeline::Create(pipelineCI);
		}

		void ForwardRenderer::SetRenderTarget(Texture* texture)
		{
			m_RenderTexture = texture;

			for (auto fbo : m_Framebuffers)
				delete fbo;
			m_Framebuffers.clear();

			CreateFramebuffers();
		}

		void ForwardRenderer::CreateFramebuffers()
		{
			TextureType attachmentTypes[2];
			attachmentTypes[0] = TextureType::COLOUR;
			attachmentTypes[1] = TextureType::DEPTH;

			Texture* attachments[2];
			attachments[1] = reinterpret_cast<Texture*>(Application::Instance()->GetRenderManager()->GetGBuffer()->GetDepthTexture());
			FramebufferInfo bufferInfo{};
			bufferInfo.width = m_ScreenBufferWidth;
			bufferInfo.height = m_ScreenBufferHeight;
			bufferInfo.attachmentCount = 2;
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
	}
}
