#include "Precompiled.h"
#include "DeferredOffScreenRenderer.h"
#include "Scene/Scene.h"
#include "Core/Application.h"
#include "Scene/Component/TextureMatrixComponent.h"

#include "Maths/Maths.h"
#include "Maths/Transform.h"
 

#include "Graphics/RenderManager.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Mesh.h"
#include "Graphics/Model.h"
#include "Graphics/Material.h"
#include "Graphics/GBuffer.h"

#include "Graphics/API/Shader.h"
#include "Graphics/API/Framebuffer.h"
#include "Graphics/API/Texture.h"
#include "Graphics/API/UniformBuffer.h"
#include "Graphics/API/Renderer.h"
#include "Graphics/API/CommandBuffer.h"
#include "Graphics/API/Swapchain.h"
#include "Graphics/API/RenderPass.h"
#include "Graphics/API/Pipeline.h"
#include "Graphics/API/GraphicsContext.h"

#include <imgui/imgui.h>

#define MAX_LIGHTS 32
#define MAX_SHADOWMAPS 16

namespace Lumos
{
	namespace Graphics
	{
		enum VSSystemUniformIndices : i32
		{
			VSSystemUniformIndex_ProjectionViewMatrix = 0,
			VSSystemUniformIndex_Size
		};

		enum PSSystemUniformIndices : i32
		{
			PSSystemUniformIndex_Lights = 0,
			PSSystemUniformIndex_Size
		};

		DeferredOffScreenRenderer::DeferredOffScreenRenderer(u32 width, u32 height)
		{
			DeferredOffScreenRenderer::SetScreenBufferSize(width, height);
			DeferredOffScreenRenderer::Init();
		}

		DeferredOffScreenRenderer::~DeferredOffScreenRenderer()
		{
			delete m_UniformBuffer;
			delete m_ModelUniformBuffer;
			delete m_DeferredCommandBuffers;
			delete m_DefaultMaterial;

			delete[] m_VSSystemUniformBuffer;

			m_Framebuffers.clear();
			m_CommandBuffers.clear();

			Memory::AlignedFree(m_UBODataDynamic.model);
		}

		void DeferredOffScreenRenderer::Init()
		{
			LUMOS_PROFILE_FUNCTION();
			m_Shader = Ref<Graphics::Shader>(Shader::CreateFromFile("DeferredColour", "/CoreShaders/"));
			m_DefaultMaterial = new Material();

			Graphics::MaterialProperties properties;
			properties.albedoColour = Maths::Vector4(1.0f);
			properties.roughnessColour = Maths::Vector4(0.5f);
			properties.metallicColour = Maths::Vector4(0.5f);
			properties.usingAlbedoMap = 0.0f;
			properties.usingRoughnessMap = 0.0f;
			properties.usingNormalMap = 0.0f;
			properties.usingMetallicMap = 0.0f;
			m_DefaultMaterial->SetMaterialProperites(properties);

			const size_t minUboAlignment = size_t(Graphics::Renderer::GetCapabilities().UniformBufferOffsetAlignment);

			m_DynamicAlignment = sizeof(Lumos::Maths::Matrix4);
			if(minUboAlignment > 0)
			{
				m_DynamicAlignment = (m_DynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
			}

			m_UniformBuffer = nullptr;
			m_ModelUniformBuffer = nullptr;

			m_CommandQueue.reserve(1000);

			//
			// Vertex shader System uniforms
			//
			m_VSSystemUniformBufferSize = sizeof(Maths::Matrix4);
			m_VSSystemUniformBuffer = new u8[m_VSSystemUniformBufferSize];
			memset(m_VSSystemUniformBuffer, 0, m_VSSystemUniformBufferSize);
			m_VSSystemUniformBufferOffsets.resize(VSSystemUniformIndex_Size);

			// Per Scene System Uniforms
			m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionViewMatrix] = 0;

			m_RenderPass = Ref<Graphics::RenderPass>(Graphics::RenderPass::Create());

			AttachmentInfo textureTypesOffScreen[5] =
				{
					{TextureType::COLOUR, Application::Get().GetRenderManager()->GetGBuffer()->GetTextureFormat(SCREENTEX_COLOUR)},
					{TextureType::COLOUR, Application::Get().GetRenderManager()->GetGBuffer()->GetTextureFormat(SCREENTEX_POSITION)},
					{TextureType::COLOUR, Application::Get().GetRenderManager()->GetGBuffer()->GetTextureFormat(SCREENTEX_NORMALS)},
					{TextureType::COLOUR, Application::Get().GetRenderManager()->GetGBuffer()->GetTextureFormat(SCREENTEX_PBR)},
					{TextureType::DEPTH, TextureFormat::DEPTH}};

			Graphics::RenderpassInfo renderpassCIOffScreen{};
			renderpassCIOffScreen.attachmentCount = 5;
			renderpassCIOffScreen.textureType = textureTypesOffScreen;

			m_RenderPass->Init(renderpassCIOffScreen);

			m_CommandBuffers.resize(Renderer::GetSwapchain()->GetSwapchainBufferCount());

			for(auto& commandBuffer : m_CommandBuffers)
			{
				commandBuffer = Graphics::CommandBuffer::Create();
				commandBuffer->Init(true);
			}

			m_DeferredCommandBuffers = Graphics::CommandBuffer::Create();
			m_DeferredCommandBuffers->Init(true);

			CreatePipeline();
			CreateBuffer();
			CreateFramebuffer();

			m_DefaultMaterial->CreateDescriptorSet(m_Pipeline.get(), 1);

			m_ClearColour = Maths::Vector4(0.1f, 0.1f, 0.1f, 1.0f);
            m_CurrentDescriptorSets.resize(2);
		}

		void DeferredOffScreenRenderer::RenderScene(Scene* scene)
		{
			LUMOS_PROFILE_FUNCTION();

			Begin();
			SetSystemUniforms(m_Shader.get());
			Present();
			End();
		}

		void DeferredOffScreenRenderer::PresentToScreen()
		{
			LUMOS_PROFILE_FUNCTION();
			Renderer::Present(m_CommandBuffers[Renderer::GetSwapchain()->GetCurrentBufferId()].get());
		}

		void DeferredOffScreenRenderer::Begin()
		{
			LUMOS_PROFILE_FUNCTION();
			m_DeferredCommandBuffers->BeginRecording();
			m_DeferredCommandBuffers->UpdateViewport(m_ScreenBufferWidth, m_ScreenBufferHeight);

			m_RenderPass->BeginRenderpass(m_DeferredCommandBuffers, Maths::Vector4(0.0f), m_Framebuffers.front().get(), Graphics::INLINE, m_ScreenBufferWidth, m_ScreenBufferHeight);
		}

		void DeferredOffScreenRenderer::BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform)
		{
			LUMOS_PROFILE_FUNCTION();
            m_CommandQueue.clear();
            m_SystemUniforms.clear();
            
			m_Camera = overrideCamera;
			m_CameraTransform = overrideCameraTransform;

			auto view = m_CameraTransform->GetWorldMatrix().Inverse();
			
			if(!m_Camera)
			{
				return;
			}

			LUMOS_ASSERT(m_Camera, "No Camera Set for Renderer");
			auto projView = m_Camera->GetProjectionMatrix() * view;
			memcpy(m_VSSystemUniformBuffer + m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_ProjectionViewMatrix], &projView, sizeof(Maths::Matrix4));

			m_Frustum = m_Camera->GetFrustum(view);
			
			
			auto& registry = scene->GetRegistry();
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

		void DeferredOffScreenRenderer::Submit(const RenderCommand& command)
		{
			LUMOS_PROFILE_FUNCTION();
			m_CommandQueue.push_back(command);
		}

		void DeferredOffScreenRenderer::SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix)
		{
			LUMOS_PROFILE_FUNCTION();
			RenderCommand command;
			command.mesh = mesh;
			command.material = material;
			command.transform = transform;
			command.textureMatrix = textureMatrix;
			Submit(command);
		}

		void DeferredOffScreenRenderer::EndScene()
		{
		}

		void DeferredOffScreenRenderer::End()
		{
			LUMOS_PROFILE_FUNCTION();
			m_RenderPass->EndRenderpass(m_DeferredCommandBuffers);
			m_DeferredCommandBuffers->EndRecording();
			m_DeferredCommandBuffers->Execute(true);
		}

		void DeferredOffScreenRenderer::SetSystemUniforms(Shader* shader)
		{
			LUMOS_PROFILE_FUNCTION();
			m_UniformBuffer->SetData(m_VSSystemUniformBufferSize, *&m_VSSystemUniformBuffer);

			int index = 0;

			for(auto& command : m_CommandQueue)
			{
				Maths::Matrix4* modelMat = reinterpret_cast<Maths::Matrix4*>((reinterpret_cast<uint64_t>(m_UBODataDynamic.model) + (index * m_DynamicAlignment)));
				command.transform = command.transform;
				*modelMat = command.transform;
				index++;
			}
			m_ModelUniformBuffer->SetDynamicData(static_cast<uint32_t>(index * m_DynamicAlignment), sizeof(Maths::Matrix4), &*m_UBODataDynamic.model);
		}

		void DeferredOffScreenRenderer::Present()
		{
			LUMOS_PROFILE_FUNCTION();
			m_Pipeline->Bind(m_DeferredCommandBuffers);

			for(u32 i = 0; i < static_cast<u32>(m_CommandQueue.size()); i++)
			{
				auto command = m_CommandQueue[i];
				Mesh* mesh = command.mesh;

				uint32_t dynamicOffset = i * static_cast<uint32_t>(m_DynamicAlignment);

				std::vector<Graphics::DescriptorSet*> descriptorSets = {m_Pipeline->GetDescriptorSet(), command.material ? command.material->GetDescriptorSet() : m_DefaultMaterial->GetDescriptorSet()};
                
                m_CurrentDescriptorSets[0] = m_Pipeline->GetDescriptorSet();
                m_CurrentDescriptorSets[1] = command.material ? command.material->GetDescriptorSet() : m_DefaultMaterial->GetDescriptorSet();

				mesh->GetVertexBuffer()->Bind(m_DeferredCommandBuffers, m_Pipeline.get());
				mesh->GetIndexBuffer()->Bind(m_DeferredCommandBuffers);

				Renderer::BindDescriptorSets(m_Pipeline.get(), m_DeferredCommandBuffers, dynamicOffset, m_CurrentDescriptorSets);
				Renderer::DrawIndexed(m_DeferredCommandBuffers, DrawType::TRIANGLE, mesh->GetIndexBuffer()->GetCount());

				mesh->GetVertexBuffer()->Unbind();
				mesh->GetIndexBuffer()->Unbind();
			}
		}

		void DeferredOffScreenRenderer::CreatePipeline()
		{
			LUMOS_PROFILE_FUNCTION();
			std::vector<Graphics::DescriptorPoolInfo> poolInfo =
				{
					{Graphics::DescriptorType::UNIFORM_BUFFER, MAX_OBJECTS},
					{Graphics::DescriptorType::UNIFORM_BUFFER, MAX_OBJECTS},
					{Graphics::DescriptorType::UNIFORM_BUFFER_DYNAMIC, MAX_OBJECTS},
					{Graphics::DescriptorType::IMAGE_SAMPLER, MAX_OBJECTS},
					{Graphics::DescriptorType::IMAGE_SAMPLER, MAX_OBJECTS},
					{Graphics::DescriptorType::IMAGE_SAMPLER, MAX_OBJECTS},
					{Graphics::DescriptorType::IMAGE_SAMPLER, MAX_OBJECTS},
					{Graphics::DescriptorType::IMAGE_SAMPLER, MAX_OBJECTS},
					{Graphics::DescriptorType::IMAGE_SAMPLER, MAX_OBJECTS}};

			std::vector<Graphics::DescriptorLayoutInfo> layoutInfo =
				{
					{Graphics::DescriptorType::UNIFORM_BUFFER, Graphics::ShaderType::VERTEX, 0},
					{Graphics::DescriptorType::UNIFORM_BUFFER_DYNAMIC, Graphics::ShaderType::VERTEX, 1},
				};

			std::vector<Graphics::DescriptorLayoutInfo> layoutInfoMesh =
				{
					{Graphics::DescriptorType::IMAGE_SAMPLER, Graphics::ShaderType::FRAGMENT, 0},
					{Graphics::DescriptorType::IMAGE_SAMPLER, Graphics::ShaderType::FRAGMENT, 1},
					{Graphics::DescriptorType::IMAGE_SAMPLER, Graphics::ShaderType::FRAGMENT, 2},
					{Graphics::DescriptorType::IMAGE_SAMPLER, Graphics::ShaderType::FRAGMENT, 3},
					{Graphics::DescriptorType::IMAGE_SAMPLER, Graphics::ShaderType::FRAGMENT, 4},
					{Graphics::DescriptorType::IMAGE_SAMPLER, Graphics::ShaderType::FRAGMENT, 5},
					{Graphics::DescriptorType::UNIFORM_BUFFER, Graphics::ShaderType::FRAGMENT, 6},
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
			pipelineCI.pipelineName = "OffScreenRenderer";
			pipelineCI.shader = m_Shader.get();
			pipelineCI.renderpass = m_RenderPass.get();
			pipelineCI.numVertexLayout = static_cast<u32>(attributeDescriptions.size());
			pipelineCI.descriptorLayouts = descriptorLayouts;
			pipelineCI.vertexLayout = attributeDescriptions.data();
			pipelineCI.numLayoutBindings = static_cast<u32>(poolInfo.size());
			pipelineCI.typeCounts = poolInfo.data();
			pipelineCI.strideSize = sizeof(Vertex);
			pipelineCI.numColorAttachments = 6;
			pipelineCI.polygonMode = Graphics::PolygonMode::Fill;
			pipelineCI.cullMode = Graphics::CullMode::BACK;
			pipelineCI.transparencyEnabled = false;
			pipelineCI.depthBiasEnabled = false;
			pipelineCI.maxObjects = MAX_OBJECTS;

			m_Pipeline = Ref<Graphics::Pipeline>(Graphics::Pipeline::Create(pipelineCI));
		}

		void DeferredOffScreenRenderer::CreateBuffer()
		{
			LUMOS_PROFILE_FUNCTION();
			if(m_UniformBuffer == nullptr)
			{
				m_UniformBuffer = Graphics::UniformBuffer::Create();

				uint32_t bufferSize = m_VSSystemUniformBufferSize;
				m_UniformBuffer->Init(bufferSize, nullptr);
			}

			if(m_ModelUniformBuffer == nullptr)
			{
				m_ModelUniformBuffer = Graphics::UniformBuffer::Create();

				uint32_t bufferSize2 = static_cast<uint32_t>(MAX_OBJECTS * m_DynamicAlignment);
				m_UBODataDynamic.model = static_cast<Maths::Matrix4*>(Memory::AlignedAlloc(bufferSize2, m_DynamicAlignment));
				m_ModelUniformBuffer->Init(bufferSize2, nullptr);
			}

			std::vector<Graphics::BufferInfo> bufferInfos;

			Graphics::BufferInfo bufferInfo = {};
			bufferInfo.buffer = m_UniformBuffer;
			bufferInfo.offset = 0;
			bufferInfo.size = m_VSSystemUniformBufferSize;
			bufferInfo.type = Graphics::DescriptorType::UNIFORM_BUFFER;
			bufferInfo.binding = 0;
			bufferInfo.shaderType = ShaderType::VERTEX;
			bufferInfo.systemUniforms = true;
			bufferInfo.name = "UniformBufferObject";

			Graphics::BufferInfo bufferInfo2 = {};
			bufferInfo2.buffer = m_ModelUniformBuffer;
			bufferInfo2.offset = 0;
			bufferInfo2.size = sizeof(Maths::Matrix4);
			bufferInfo2.type = Graphics::DescriptorType::UNIFORM_BUFFER_DYNAMIC;
			bufferInfo2.binding = 1;
			bufferInfo2.shaderType = ShaderType::VERTEX;
			bufferInfo2.systemUniforms = false;
			bufferInfo2.name = "UniformBufferObject2";

			bufferInfos.push_back(bufferInfo);
			bufferInfos.push_back(bufferInfo2);

			m_Pipeline->GetDescriptorSet()->Update(bufferInfos);
		}

		void DeferredOffScreenRenderer::CreateFramebuffer()
		{
			LUMOS_PROFILE_FUNCTION();
			const u32 attachmentCount = 5;
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
			bufferInfo.renderPass = m_RenderPass.get();
			bufferInfo.attachmentTypes = attachmentTypes;

			Texture* attachments[attachmentCount];
			attachments[0] = Application::Get().GetRenderManager()->GetGBuffer()->GetTexture(SCREENTEX_COLOUR);
			attachments[1] = Application::Get().GetRenderManager()->GetGBuffer()->GetTexture(SCREENTEX_POSITION);
			attachments[2] = Application::Get().GetRenderManager()->GetGBuffer()->GetTexture(SCREENTEX_NORMALS);
			attachments[3] = Application::Get().GetRenderManager()->GetGBuffer()->GetTexture(SCREENTEX_PBR);
			attachments[4] = Application::Get().GetRenderManager()->GetGBuffer()->GetDepthTexture();
			bufferInfo.attachments = attachments;

			m_Framebuffers.push_back(Ref<Framebuffer>(Framebuffer::Create(bufferInfo)));
		}

		void DeferredOffScreenRenderer::OnResize(u32 width, u32 height)
		{
			LUMOS_PROFILE_FUNCTION();
			m_Framebuffers.clear();

			DeferredOffScreenRenderer::SetScreenBufferSize(width, height);

			CreateFramebuffer();
		}

		void DeferredOffScreenRenderer::OnImGui()
		{
			ImGui::TextUnformatted("Deferred Offscreen Renderer");
		}
	}
}
