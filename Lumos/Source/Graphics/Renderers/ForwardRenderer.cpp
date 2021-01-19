#include "Precompiled.h"
#include "ForwardRenderer.h"
#include "Graphics/API/Shader.h"
#include "Graphics/API/Framebuffer.h"
#include "Graphics/Light.h"
#include "Graphics/API/Texture.h"
#include "Graphics/API/UniformBuffer.h"
#include "Graphics/Mesh.h"
#include "Graphics/Model.h"
#include "Graphics/Material.h"
#include "Graphics/API/Renderer.h"
#include "Graphics/API/CommandBuffer.h"
#include "Graphics/API/Swapchain.h"
#include "Graphics/API/RenderPass.h"
#include "Graphics/API/Pipeline.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/GBuffer.h"
#include "Scene/Scene.h"
#include "Scene/Component/TextureMatrixComponent.h"
#include "Maths/Maths.h"
#include "Maths/Transform.h"

#include "Embedded/CheckerBoardTextureArray.inl"

#include "Core/Application.h"
#include "RenderGraph.h"
#include "Graphics/Camera/Camera.h"

namespace Lumos
{
	namespace Graphics
	{
		ForwardRenderer::ForwardRenderer(u32 width, u32 height, bool depthTest)
		{
			m_DepthTest = depthTest;
			SetScreenBufferSize(width, height);
			ForwardRenderer::Init();
		}

		ForwardRenderer::~ForwardRenderer()
		{
			delete m_DefaultTexture;
			delete m_UniformBuffer;

			Memory::AlignedFree(m_UBODataDynamic.model);

			delete m_ModelUniformBuffer;

			delete[] m_VSSystemUniformBuffer;
			delete[] m_PSSystemUniformBuffer;
            
			for(auto& commandBuffer : m_CommandBuffers)
			{
				delete commandBuffer;
			}
		}

		void ForwardRenderer::RenderScene()
		{
			//for (i = 0; i < commandBuffers.size(); i++)
			{
				Begin();

				SetSystemUniforms(m_Shader.get());

				Present();

				EndScene();
				End();
			}

			if(!m_RenderTexture)
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
			m_VSSystemUniformBuffer = new u8[m_VSSystemUniformBufferSize];
			memset(m_VSSystemUniformBuffer, 0, m_VSSystemUniformBufferSize);
			m_VSSystemUniformBufferOffsets.resize(VSSystemUniformIndex_Size);

			// Per Scene System Uniforms
			m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionMatrix] = 0;
			m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ViewMatrix] = m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionMatrix] + sizeof(Maths::Matrix4);
			m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ModelMatrix] = m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ViewMatrix] + sizeof(Maths::Matrix4);
			m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_TextureMatrix] = m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ModelMatrix] + sizeof(Maths::Matrix4);

			// Pixel/fragment shader System uniforms
			m_PSSystemUniformBufferSize = sizeof(Graphics::Light);
			m_PSSystemUniformBuffer = new u8[m_PSSystemUniformBufferSize];
			memset(m_PSSystemUniformBuffer, 0, m_PSSystemUniformBufferSize);
			m_PSSystemUniformBufferOffsets.resize(PSSystemUniformIndex_Size);

			// Per Scene System Uniforms
			m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_Lights] = 0;

			m_UniformBuffer = Graphics::UniformBuffer::Create();
			m_ModelUniformBuffer = Graphics::UniformBuffer::Create();

			Graphics::RenderPassInfo renderpassCI{};

			if(m_DepthTest)
			{
				AttachmentInfo textureTypes[2] =
					{
						{TextureType::COLOUR, TextureFormat::RGBA8},
						{TextureType::DEPTH, TextureFormat::DEPTH}};

				renderpassCI.attachmentCount = 2;
				renderpassCI.textureType = textureTypes;
			}
			else
			{
				AttachmentInfo textureTypes[1] =
					{
						{TextureType::COLOUR, TextureFormat::RGBA8},
					};

				renderpassCI.attachmentCount = 1;
				renderpassCI.textureType = textureTypes;
			}

            m_RenderPass = Graphics::RenderPass::Get(renderpassCI);

			CreateFramebuffers();

			m_CommandBuffers.resize(Renderer::GetSwapchain()->GetSwapchainBufferCount());

			for(auto& commandBuffer : m_CommandBuffers)
			{
				commandBuffer = Graphics::CommandBuffer::Create();
				commandBuffer->Init(true);
			}

			CreateGraphicsPipeline();

			uint32_t bufferSize = static_cast<uint32_t>(sizeof(UniformBufferObject));
			m_UniformBuffer->Init(bufferSize, nullptr);

			const size_t minUboAlignment = Graphics::GraphicsContext::GetContext()->GetMinUniformBufferOffsetAlignment();

			m_DynamicAlignment = sizeof(Lumos::Maths::Matrix4);
			if(minUboAlignment > 0)
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

			m_ClearColour = Maths::Vector4(0.4f, 0.4f, 0.4f, 1.0f);

			m_DefaultTexture = Texture2D::CreateFromSource(CheckerboardTextureArrayWidth, CheckerboardTextureArrayHeight, (void*)(u8*)CheckerboardTextureArray);

			Graphics::DescriptorInfo info{};
			info.pipeline = m_Pipeline.get();
			info.layoutIndex = 1;
			info.shader = m_Shader.get();
			m_DescriptorSet = Graphics::DescriptorSet::Create(info);

			std::vector<Graphics::ImageInfo> bufferInfosDefault;

			Graphics::ImageInfo imageInfo = {};
			imageInfo.texture = {m_DefaultTexture};
			imageInfo.binding = 0;
			imageInfo.name = "texSampler";

			bufferInfosDefault.push_back(imageInfo);

			m_DescriptorSet->Update(bufferInfosDefault);
            
            m_CurrentDescriptorSets.resize(2);
		}

		void ForwardRenderer::Begin()
		{
			m_CurrentBufferID = 0;
			if(!m_RenderTexture)
				m_CurrentBufferID = Renderer::GetSwapchain()->GetCurrentBufferId();

			m_RenderPass->BeginRenderpass(m_CommandBuffers[m_CurrentBufferID], m_ClearColour, m_Framebuffers[m_CurrentBufferID].get(), Graphics::INLINE, m_ScreenBufferWidth, m_ScreenBufferHeight);
		}

		void ForwardRenderer::BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform)
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

			auto proj = m_Camera->GetProjectionMatrix();

			memcpy(m_VSSystemUniformBuffer + m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionMatrix], &proj, sizeof(Maths::Matrix4));

            m_CommandQueue.clear();

            auto group = registry.group<Model>(entt::get<Maths::Transform>);

            for(auto entity : group)
            {
                const auto& [model, trans] = group.get<Model, Maths::Transform>(entity);

                const auto& meshes = model.GetMeshes();
                
                for(auto mesh : meshes)
                {
                    if(mesh->GetActive())
                    {
                        auto& worldTransform = trans.GetWorldMatrix();

                        auto bb = mesh->GetBoundingBox();
                        auto bbCopy = bb->Transformed(worldTransform);
                        auto inside = m_Frustum.IsInsideFast(bbCopy);

                        if(inside == Maths::Intersection::OUTSIDE)
                            continue;

                        auto meshPtr = mesh;
                        auto material = meshPtr->GetMaterial();
                        if(material)
                        {
                            if(material->GetDescriptorSet() == nullptr || material->GetPipeline() != m_Pipeline.get() || material->GetTexturesUpdated())
                            {
                                material->CreateDescriptorSet(m_Pipeline.get(), 1);
                                material->SetTexturesUpdated(false);
                            }
                        }

                        auto textureMatrixTransform = registry.try_get<TextureMatrixComponent>(entity);
                        Maths::Matrix4 textureMatrix;
                        if(textureMatrixTransform)
                            textureMatrix = textureMatrixTransform->GetMatrix();
                        else
                            textureMatrix = Maths::Matrix4();

                        SubmitMesh(meshPtr.get(), material.get(), worldTransform, textureMatrix);
                    }
                }
            }
		}

		void ForwardRenderer::BeginScene(const Maths::Matrix4& proj, const Maths::Matrix4& view)
		{
			memcpy(m_VSSystemUniformBuffer + m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionMatrix], &proj, sizeof(Maths::Matrix4));
			memcpy(m_VSSystemUniformBuffer + m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ViewMatrix], &view, sizeof(Maths::Matrix4));
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

			if(m_RenderTexture)
				m_CommandBuffers[m_CurrentBufferID]->Execute(true);
		}

		void ForwardRenderer::SetSystemUniforms(Shader* shader) const
		{
			m_UniformBuffer->SetData(sizeof(UniformBufferObject), *&m_VSSystemUniformBuffer);

			int index = 0;

			for(auto& command : m_CommandQueue)
			{
				Maths::Matrix4* modelMat = reinterpret_cast<Maths::Matrix4*>((reinterpret_cast<uint64_t>(m_UBODataDynamic.model) + (index * m_DynamicAlignment)));
				*modelMat = command.transform;
				index++;
			}

			m_ModelUniformBuffer->SetDynamicData(static_cast<uint32_t>(index * m_DynamicAlignment), sizeof(Maths::Matrix4), &*m_UBODataDynamic.model);
		}

		void ForwardRenderer::Present()
		{
			int index = 0;

			for(auto& command : m_CommandQueue)
			{
				Mesh* mesh = command.mesh;

				Graphics::CommandBuffer* currentCMDBuffer = m_CommandBuffers[m_CurrentBufferID];

				m_Pipeline->Bind(currentCMDBuffer);

				uint32_t dynamicOffset = index * static_cast<uint32_t>(m_DynamicAlignment);

                m_CurrentDescriptorSets[0] = m_Pipeline->GetDescriptorSet();
                m_CurrentDescriptorSets[1] = m_DescriptorSet.get();
				
				mesh->GetVertexBuffer()->Bind(currentCMDBuffer, m_Pipeline.get());
				mesh->GetIndexBuffer()->Bind(currentCMDBuffer);

				Renderer::BindDescriptorSets(m_Pipeline.get(), currentCMDBuffer, dynamicOffset, m_CurrentDescriptorSets);
				Renderer::DrawIndexed(currentCMDBuffer, DrawType::TRIANGLE, mesh->GetIndexBuffer()->GetCount());

				mesh->GetVertexBuffer()->Unbind();
				mesh->GetIndexBuffer()->Unbind();

				index++;
			}
		}

		void ForwardRenderer::OnResize(u32 width, u32 height)
		{
			SetScreenBufferSize(width, height);
			m_Framebuffers.clear();

			CreateFramebuffers();
		}

		void ForwardRenderer::CreateGraphicsPipeline()
		{
			m_Shader = Application::Get().GetShaderLibrary()->GetResource("/CoreShaders/Simple.shader");

            Graphics::BufferLayout vertexBufferLayout;
            vertexBufferLayout.Push<Maths::Vector3>("position");
            vertexBufferLayout.Push<Maths::Vector4>("colour");
            vertexBufferLayout.Push<Maths::Vector2>("uv");
            vertexBufferLayout.Push<Maths::Vector3>("normal");
            vertexBufferLayout.Push<Maths::Vector3>("tangent");
            
			Graphics::PipelineInfo pipelineCreateInfo{};
            pipelineCreateInfo.vertexBufferLayout = vertexBufferLayout;
			pipelineCreateInfo.shader = m_Shader;
			pipelineCreateInfo.renderpass = m_RenderPass;
			pipelineCreateInfo.polygonMode = Graphics::PolygonMode::FILL;
			pipelineCreateInfo.cullMode = Graphics::CullMode::BACK;
			pipelineCreateInfo.transparencyEnabled = false;
			pipelineCreateInfo.depthBiasEnabled = false;

			m_Pipeline = Graphics::Pipeline::Get(pipelineCreateInfo);
		}

		void ForwardRenderer::SetRenderTarget(Texture* texture, bool rebuildFramebuffer)
		{
			m_RenderTexture = texture;

			if(!rebuildFramebuffer)
				return;

			m_Framebuffers.clear();

			CreateFramebuffers();
		}

		void ForwardRenderer::CreateFramebuffers()
		{
			TextureType attachmentTypes[2];
			Texture* attachments[2];

			attachmentTypes[0] = TextureType::COLOUR;
			if(m_DepthTest)
			{
				attachmentTypes[1] = TextureType::DEPTH;
				attachments[1] = reinterpret_cast<Texture*>(Application::Get().GetRenderGraph()->GetGBuffer()->GetDepthTexture());
			}

			FramebufferInfo bufferInfo{};
			bufferInfo.width = m_ScreenBufferWidth;
			bufferInfo.height = m_ScreenBufferHeight;
			bufferInfo.attachmentCount = m_DepthTest ? 2 : 1;
			bufferInfo.renderPass = m_RenderPass.get();
			bufferInfo.attachmentTypes = attachmentTypes;

			if(m_RenderTexture)
			{
				attachments[0] = m_RenderTexture;
				bufferInfo.attachments = attachments;
				bufferInfo.screenFBO = false;
				m_Framebuffers.emplace_back(Framebuffer::Get(bufferInfo));
			}
			else
			{
				for(uint32_t i = 0; i < Renderer::GetSwapchain()->GetSwapchainBufferCount(); i++)
				{
					bufferInfo.screenFBO = true;
					attachments[0] = Renderer::GetSwapchain()->GetImage(i);
					bufferInfo.attachments = attachments;

					m_Framebuffers.emplace_back(Framebuffer::Get(bufferInfo));
				}
			}
		}
	}
}
