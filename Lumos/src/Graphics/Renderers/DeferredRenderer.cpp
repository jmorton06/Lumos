#include "Precompiled.h"
#include "DeferredRenderer.h"
#include "DeferredOffScreenRenderer.h"
#include "ShadowRenderer.h"

#include "Scene/Scene.h"
#include "Core/Application.h"
#include "Maths/Maths.h"
#include "Maths/Transform.h"
 
#include "RenderGraph.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Mesh.h"
#include "Graphics/MeshFactory.h"
#include "Graphics/Material.h"
#include "Graphics/GBuffer.h"
#include "Graphics/Light.h"

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
#include "Graphics/Environment.h"
#include "Embedded/BRDFTexture.inl"
#include "Utilities/AssetManager.h"

#include <imgui/imgui.h>

#define MAX_LIGHTS 32
#define MAX_SHADOWMAPS 16

namespace Lumos
{
	namespace Graphics
	{
		enum PSSystemUniformIndices : i32
		{
			PSSystemUniformIndex_Lights = 0,
			PSSystemUniformIndex_CameraPosition,
			PSSystemUniformIndex_ViewMatrix,
			PSSystemUniformIndex_ShadowTransforms,
			PSSystemUniformIndex_ShadowSplitDepths,
			PSSystemUniformIndex_BiasMatrix,
			PSSystemUniformIndex_LightCount,
			PSSystemUniformIndex_ShadowCount,
			PSSystemUniformIndex_RenderMode,
			PSSystemUniformIndex_cubemapMipLevels,
			PSSystemUniformIndex_Size
		};

		DeferredRenderer::DeferredRenderer(u32 width, u32 height)
		{
			DeferredRenderer::SetScreenBufferSize(width, height);
			DeferredRenderer::Init();
		}

		DeferredRenderer::~DeferredRenderer()
		{
			delete m_UniformBuffer;
			delete m_LightUniformBuffer;
			delete m_ScreenQuad;
			delete m_OffScreenRenderer;

			delete[] m_PSSystemUniformBuffer;
			delete m_DeferredCommandBuffers;
		}

		void DeferredRenderer::Init()
		{
			LUMOS_PROFILE_FUNCTION();
			m_OffScreenRenderer = new DeferredOffScreenRenderer(m_ScreenBufferWidth, m_ScreenBufferHeight);

            m_Shader = Application::Get().GetShaderLibrary()->GetResource("/CoreShaders/DeferredLight.shader");

			switch(Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case Graphics::RenderAPI::OPENGL:
				m_BiasMatrix = Maths::Matrix4(0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f);
				break;
#endif

#ifdef LUMOS_RENDER_API_VULKAN
			case Graphics::RenderAPI::VULKAN:
				m_BiasMatrix = Maths::Matrix4(0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
				break;
#endif

#ifdef LUMOS_RENDER_API_DIRECT3D
			case Graphics::RenderAPI::DIRECT3D:
				m_BiasMatrix = Maths::Matrix4(0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
				break;
#endif
			default:
				break;
			}

			TextureParameters param;
			param.minFilter = TextureFilter::LINEAR;
			param.magFilter = TextureFilter::LINEAR;
			param.format = TextureFormat::RGBA;
			param.wrap = TextureWrap::CLAMP_TO_EDGE;
			m_PreintegratedFG = UniqueRef<Texture2D>(Texture2D::CreateFromSource(BRDFTextureWidth, BRDFTextureHeight, (void*)BRDFTexture, param));

			m_LightUniformBuffer = nullptr;
			m_UniformBuffer = nullptr;

			m_ScreenQuad = Graphics::CreateQuad();

			// Pixel/fragment shader System uniforms
			m_PSSystemUniformBufferSize = sizeof(Light) * MAX_LIGHTS + sizeof(Maths::Vector4) + sizeof(Maths::Matrix4) * 2 + (sizeof(Maths::Matrix4) + sizeof(Maths::Vector4)) * MAX_SHADOWMAPS + sizeof(int) * 4;
			m_PSSystemUniformBuffer = new u8[m_PSSystemUniformBufferSize];
			memset(m_PSSystemUniformBuffer, 0, m_PSSystemUniformBufferSize);
			m_PSSystemUniformBufferOffsets.resize(PSSystemUniformIndex_Size);

			// Per Scene System Uniforms
			m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_Lights] = 0;
			m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_CameraPosition] = m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_Lights] + sizeof(Light) * MAX_LIGHTS;
			m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ViewMatrix] = m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_CameraPosition] + sizeof(Maths::Vector4);
			m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowTransforms] = m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ViewMatrix] + sizeof(Maths::Matrix4);
			m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowSplitDepths] = m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowTransforms] + sizeof(Maths::Matrix4) * MAX_SHADOWMAPS;
			m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_BiasMatrix] = m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowSplitDepths] + sizeof(Maths::Vector4) * MAX_SHADOWMAPS;
			m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_LightCount] = m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_BiasMatrix] + sizeof(Maths::Matrix4);
			m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowCount] = m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_LightCount] + sizeof(int);
			m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_RenderMode] = m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowCount] + sizeof(int);
			m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_cubemapMipLevels] = m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_RenderMode] + sizeof(int);

			m_RenderPass = Ref<Graphics::RenderPass>(Graphics::RenderPass::Create());

			AttachmentInfo textureTypes[2] =
				{
					{TextureType::COLOUR, TextureFormat::RGBA32}};
			Graphics::RenderpassInfo renderpassCI{};
			renderpassCI.attachmentCount = 1;
			renderpassCI.textureType = textureTypes;
			renderpassCI.clear = true;

			m_RenderPass->Init(renderpassCI);

			m_CommandBuffers.resize(Renderer::GetSwapchain()->GetSwapchainBufferCount());

			for(auto& commandBuffer : m_CommandBuffers)
			{
				commandBuffer = Ref<Graphics::CommandBuffer>(Graphics::CommandBuffer::Create());
				commandBuffer->Init(true);
			}

			m_DeferredCommandBuffers = Graphics::CommandBuffer::Create();
			m_DeferredCommandBuffers->Init(true);

			CreateDeferredPipeline();
			CreateFramebuffers();
			CreateLightBuffer();

			Graphics::DescriptorInfo info{};
            info.pipeline = m_Pipeline.get();
			info.layoutIndex = 1;
			info.shader = m_Shader.get();
			m_DescriptorSet = Ref<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));

			m_ClearColour = Maths::Vector4(0.2f, 0.2f, 0.2f, 1.0f);

			UpdateScreenDescriptorSet();
            
            m_CurrentDescriptorSets.resize(2);
		}

		void DeferredRenderer::RenderScene(Scene* scene)
		{
			LUMOS_PROFILE_FUNCTION();

			m_OffScreenRenderer->RenderScene(scene);

			SetSystemUniforms(m_Shader.get());

			int commandBufferIndex = 0;
			if(!m_RenderTexture)
				commandBufferIndex = Renderer::GetSwapchain()->GetCurrentBufferId();

			Begin(commandBufferIndex);
			Present();
			End();

			if(!m_RenderTexture)
				PresentToScreen();
		}

		void DeferredRenderer::PresentToScreen()
		{
			LUMOS_PROFILE_FUNCTION();
			Renderer::Present((m_CommandBuffers[Renderer::GetSwapchain()->GetCurrentBufferId()].get()));
		}

		void DeferredRenderer::Begin(int commandBufferID)
		{
			LUMOS_PROFILE_FUNCTION();
			m_CommandQueue.clear();
			m_SystemUniforms.clear();

			m_CommandBufferIndex = commandBufferID;
			m_RenderPass->BeginRenderpass(m_CommandBuffers[m_CommandBufferIndex].get(), m_ClearColour, m_Framebuffers[m_CommandBufferIndex].get(), Graphics::INLINE, m_ScreenBufferWidth, m_ScreenBufferHeight);
		}

		void DeferredRenderer::BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform)
		{
			LUMOS_PROFILE_FUNCTION();
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

			auto view = registry.view<Graphics::Environment>();

			if(view.size() == 0)
			{
				if(m_EnvironmentMap)
				{
					m_EnvironmentMap = nullptr;
					m_IrradianceMap = nullptr;

					Graphics::DescriptorInfo info{};
					info.pipeline = m_Pipeline.get();
					info.layoutIndex = 1;
					info.shader = m_Shader.get();
					m_DescriptorSet = Ref<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));

					UpdateScreenDescriptorSet();
				}
			}
			else
			{
				//Just use first
				const auto& env = view.get<Graphics::Environment>(view.front());

				if(m_EnvironmentMap != env.GetEnvironmentMap())
				{
					m_EnvironmentMap = env.GetEnvironmentMap();
					m_IrradianceMap = env.GetIrradianceMap();
					UpdateScreenDescriptorSet();
				}
			}

			SubmitLightSetup(scene);

			m_OffScreenRenderer->BeginScene(scene, m_Camera, m_CameraTransform);
		}

		void DeferredRenderer::Submit(const RenderCommand& command)
		{
			LUMOS_PROFILE_FUNCTION();
			m_CommandQueue.push_back(command);
		}

		void DeferredRenderer::SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix)
		{
			LUMOS_PROFILE_FUNCTION();
			RenderCommand command;
			command.mesh = mesh;
			command.transform = transform;
			command.textureMatrix = textureMatrix;
			command.material = material;
			Submit(command);
		}

		void DeferredRenderer::SubmitLightSetup(Scene* scene)
		{
			LUMOS_PROFILE_FUNCTION();

			auto& registry = scene->GetRegistry();

			auto group = registry.group<Graphics::Light>(entt::get<Maths::Transform>);

			u32 numLights = 0;

			auto viewMatrix = m_CameraTransform->GetWorldMatrix().Inverse();

			auto& frustum = m_Camera->GetFrustum(viewMatrix);

			for(auto entity : group)
			{
				const auto& [light, trans] = group.get<Graphics::Light, Maths::Transform>(entity);
				light.Position = trans.GetWorldPosition();

				if(light.Type != float(LightType::DirectionalLight))
				{
					auto inside = frustum.IsInsideFast(Maths::Sphere(light.Position.ToVector3(), light.Radius));

					if(inside == Maths::Intersection::OUTSIDE)
						continue;
				}

				Maths::Vector3 forward = Maths::Vector3::FORWARD;
				forward = trans.GetWorldOrientation() * forward;

				light.Direction = forward.Normalized();

				memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_Lights] + sizeof(Graphics::Light) * numLights, &light, sizeof(Graphics::Light));
				numLights++;
			}

			Maths::Vector4 cameraPos = Maths::Vector4(m_CameraTransform->GetWorldPosition());
			memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_CameraPosition], &cameraPos, sizeof(Maths::Vector4));

			auto shadowRenderer = Application::Get().GetRenderGraph()->GetShadowRenderer();
			if(shadowRenderer)
			{
				Maths::Matrix4* shadowTransforms = shadowRenderer->GetShadowProjView();
				Lumos::Maths::Vector4* uSplitDepth = shadowRenderer->GetSplitDepths();

				memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ViewMatrix], &viewMatrix, sizeof(Maths::Matrix4));
				memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowTransforms], shadowTransforms, sizeof(Maths::Matrix4) * MAX_SHADOWMAPS);
				memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowSplitDepths], uSplitDepth, sizeof(Maths::Vector4) * MAX_SHADOWMAPS);
				memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_BiasMatrix], &m_BiasMatrix, sizeof(Maths::Matrix4));
			}

			int numShadows = shadowRenderer ? int(shadowRenderer->GetShadowMapNum()) : 0;

			auto cubemapMipLevels = m_EnvironmentMap ? m_EnvironmentMap->GetMipMapLevels() : 0;
			memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_LightCount], &numLights, sizeof(int));
			memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowCount], &numShadows, sizeof(int));
			memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_RenderMode], &m_RenderMode, sizeof(int));
			memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_cubemapMipLevels], &cubemapMipLevels, sizeof(int));
		}

		void DeferredRenderer::EndScene()
		{
		}

		void DeferredRenderer::End()
		{
			LUMOS_PROFILE_FUNCTION();
			m_RenderPass->EndRenderpass(m_CommandBuffers[m_CommandBufferIndex].get());

			if(m_RenderTexture)
				m_CommandBuffers[0]->Execute(true);
		}

		void DeferredRenderer::SetSystemUniforms(Shader* shader) const
		{
			LUMOS_PROFILE_FUNCTION();
			m_LightUniformBuffer->SetData(m_PSSystemUniformBufferSize, *&m_PSSystemUniformBuffer);
		}

		void DeferredRenderer::Present()
		{
			LUMOS_PROFILE_FUNCTION();
			Graphics::CommandBuffer* currentCMDBuffer = m_CommandBuffers[m_CommandBufferIndex].get();

			m_Pipeline->Bind(currentCMDBuffer);

            m_CurrentDescriptorSets[0] = m_Pipeline->GetDescriptorSet();
            m_CurrentDescriptorSets[1] = m_DescriptorSet.get();

            m_ScreenQuad->GetVertexBuffer()->Bind(currentCMDBuffer, m_Pipeline.get());
			m_ScreenQuad->GetIndexBuffer()->Bind(currentCMDBuffer);

			Renderer::BindDescriptorSets(m_Pipeline.get(), currentCMDBuffer, 0, m_CurrentDescriptorSets);
			Renderer::DrawIndexed(currentCMDBuffer, DrawType::TRIANGLE, m_ScreenQuad->GetIndexBuffer()->GetCount());

			m_ScreenQuad->GetVertexBuffer()->Unbind();
			m_ScreenQuad->GetIndexBuffer()->Unbind();
		}

		void DeferredRenderer::CreateDeferredPipeline()
		{
			LUMOS_PROFILE_FUNCTION();
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
			pipelineCreateInfo.cullMode = Graphics::CullMode::FRONT; //TODO
			pipelineCreateInfo.transparencyEnabled = false;
			pipelineCreateInfo.depthBiasEnabled = false;

			m_Pipeline = Ref<Graphics::Pipeline>(Graphics::Pipeline::Create(pipelineCreateInfo));
		}

		void DeferredRenderer::SetRenderTarget(Texture* texture, bool rebuildFramebuffer)
		{
			LUMOS_PROFILE_FUNCTION();
			m_RenderTexture = texture;

			if(rebuildFramebuffer)
			{
				m_Framebuffers.clear();
				CreateFramebuffers();
			}
		}

		std::string RenderModeToString(int mode)
		{
			switch(mode)
			{
			case 0:
				return "Lighting";
			case 1:
				return "Colour";
			case 2:
				return "Metallic";
			case 3:
				return "Roughness";
			case 4:
				return "AO";
			case 5:
				return "Emissive";
			case 6:
				return "Normal";
			case 7:
				return "Shadow Cascades";
			default:
				return "Lighting";
			}
		}

		void DeferredRenderer::OnImGui()
		{
			LUMOS_PROFILE_FUNCTION();
			m_OffScreenRenderer->OnImGui();

			ImGui::TextUnformatted("Deferred Renderer");

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Number Of Renderables");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::Text("%5.2lu", m_CommandQueue.size());
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Render Mode");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			if(ImGui::BeginMenu(RenderModeToString(m_RenderMode).c_str()))
			{
				const int numRenderModes = 8;

				for(int i = 0; i < numRenderModes; i++)
				{
					if(ImGui::MenuItem(RenderModeToString(i).c_str(), "", m_RenderMode == i, true))
					{
						m_RenderMode = i;
					}
				}
				ImGui::EndMenu();
			}
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::PopStyleVar();
		}

		void DeferredRenderer::CreateFramebuffers()
		{
			TextureType attachmentTypes[2];
			attachmentTypes[0] = TextureType::COLOUR;

			Texture* attachments[1];
			FramebufferInfo bufferInfo{};
			bufferInfo.width = m_ScreenBufferWidth;
			bufferInfo.height = m_ScreenBufferHeight;
			bufferInfo.attachmentCount = 1;
			bufferInfo.renderPass = m_RenderPass.get();
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

		void DeferredRenderer::CreateLightBuffer()
		{
			if(m_LightUniformBuffer == nullptr)
			{
				m_LightUniformBuffer = Graphics::UniformBuffer::Create();

				uint32_t bufferSize = m_PSSystemUniformBufferSize;
				m_LightUniformBuffer->Init(bufferSize, nullptr);
			}

			std::vector<Graphics::BufferInfo> bufferInfos;

			Graphics::BufferInfo bufferInfo = {};
			bufferInfo.name = "UniformBufferLight";
			bufferInfo.buffer = m_LightUniformBuffer;
			bufferInfo.offset = 0;
			bufferInfo.size = m_PSSystemUniformBufferSize;
			bufferInfo.type = Graphics::DescriptorType::UNIFORM_BUFFER;
			bufferInfo.binding = 0;
			bufferInfo.shaderType = ShaderType::FRAGMENT;
			bufferInfo.systemUniforms = false;

			bufferInfos.push_back(bufferInfo);

			m_Pipeline->GetDescriptorSet()->Update(bufferInfos);
		}

		void DeferredRenderer::OnResize(u32 width, u32 height)
		{
			LUMOS_PROFILE_FUNCTION();
			m_Framebuffers.clear();

			DeferredRenderer::SetScreenBufferSize(width, height);

			CreateFramebuffers();

			m_OffScreenRenderer->OnResize(width, height);
            
            m_EnvironmentMap = nullptr;
            m_IrradianceMap = nullptr;

			//Update DescriptorSet with updated gbuffer textures
			UpdateScreenDescriptorSet();
		}

		void DeferredRenderer::UpdateScreenDescriptorSet()
		{
			std::vector<Graphics::ImageInfo> bufferInfos;

			Graphics::ImageInfo imageInfo = {};
			imageInfo.texture = {Application::Get().GetRenderGraph()->GetGBuffer()->GetTexture(SCREENTEX_COLOUR)};
			imageInfo.binding = 0;
			imageInfo.name = "uColourSampler";

			Graphics::ImageInfo imageInfo2 = {};
			imageInfo2.texture = {Application::Get().GetRenderGraph()->GetGBuffer()->GetTexture(SCREENTEX_POSITION)};
			imageInfo2.binding = 1;
			imageInfo2.name = "uPositionSampler";

			Graphics::ImageInfo imageInfo3 = {};
			imageInfo3.texture = {Application::Get().GetRenderGraph()->GetGBuffer()->GetTexture(SCREENTEX_NORMALS)};
			imageInfo3.binding = 2;
			imageInfo3.name = "uNormalSampler";

			Graphics::ImageInfo imageInfo4 = {};
			imageInfo4.texture = {Application::Get().GetRenderGraph()->GetGBuffer()->GetTexture(SCREENTEX_PBR)};
			imageInfo4.binding = 3;
			imageInfo4.name = "uPBRSampler";

			Graphics::ImageInfo imageInfo5 = {};
			imageInfo5.texture = {m_PreintegratedFG.get()};
			imageInfo5.binding = 4;
			imageInfo5.name = "uPreintegratedFG";

			Graphics::ImageInfo imageInfo6 = {};
			imageInfo6.texture = {m_EnvironmentMap};
			imageInfo6.binding = 5;
			imageInfo6.type = TextureType::CUBE;
			imageInfo6.name = "uEnvironmentMap";

			Graphics::ImageInfo imageInfo7 = {};
			imageInfo7.texture = {m_IrradianceMap};
			imageInfo7.binding = 6;
			imageInfo7.type = TextureType::CUBE;
			imageInfo7.name = "uIrradianceMap";

			Graphics::ImageInfo imageInfo8 = {};
			auto shadowRenderer = Application::Get().GetRenderGraph()->GetShadowRenderer();
			if(shadowRenderer)
			{
				imageInfo8.texture = {reinterpret_cast<Texture*>(shadowRenderer->GetTexture())};
				imageInfo8.binding = 7;
				imageInfo8.type = TextureType::DEPTHARRAY;
				imageInfo8.name = "uShadowMap";
			}

			Graphics::ImageInfo imageInfo9 = {};
			imageInfo9.texture = {Application::Get().GetRenderGraph()->GetGBuffer()->GetDepthTexture()};
			imageInfo9.binding = 8;
			imageInfo9.type = TextureType::DEPTH;
			imageInfo9.name = "uDepthSampler";

			bufferInfos.push_back(imageInfo);
			bufferInfos.push_back(imageInfo2);
			bufferInfos.push_back(imageInfo3);
			bufferInfos.push_back(imageInfo4);
			bufferInfos.push_back(imageInfo5);
			if(m_EnvironmentMap)
				bufferInfos.push_back(imageInfo6);
			if(m_IrradianceMap)
				bufferInfos.push_back(imageInfo7);
			if(shadowRenderer)
				bufferInfos.push_back(imageInfo8);

			m_DescriptorSet->Update(bufferInfos);
		}
	}
}
