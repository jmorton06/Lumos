#include "lmpch.h"
#include "DeferredRenderer.h"
#include "DeferredOffScreenRenderer.h"
#include "ShadowRenderer.h"

#include "App/Scene.h"
#include "App/Application.h"
#include "Maths/Maths.h"
#include "Maths/Transform.h"
#include "Core/Profiler.h"

#include "Graphics/RenderManager.h"
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
			PSSystemUniformIndex_LightCount,
			PSSystemUniformIndex_ShadowCount,
			PSSystemUniformIndex_RenderMode,
			PSSystemUniformIndex_ShadowMode,
			PSSystemUniformIndex_Size
		};

		DeferredRenderer::DeferredRenderer(u32 width, u32 height, bool renderToGBuffer)
		{
			DeferredRenderer::SetScreenBufferSize(width, height);
			DeferredRenderer::Init();
            SetRenderToGBufferTexture(renderToGBuffer);
		}

		DeferredRenderer::~DeferredRenderer()
		{
			delete m_Shader;
			delete m_UniformBuffer;
			delete m_LightUniformBuffer;
			delete m_RenderPass;
			delete m_Pipeline;
			delete m_ScreenQuad;
			delete m_DescriptorSet;
			lmdel m_OffScreenRenderer;

			delete[] m_PSSystemUniformBuffer;
			for (auto& commandBuffer : m_CommandBuffers)
			{
				delete commandBuffer;
			}

			for (auto fbo : m_Framebuffers)
				delete fbo;

			m_CommandBuffers.clear();

			delete m_DeferredCommandBuffers;
		}

		void DeferredRenderer::Init()
		{
			LUMOS_PROFILE_FUNC;
			m_OffScreenRenderer = lmnew DeferredOffScreenRenderer(m_ScreenBufferWidth, m_ScreenBufferHeight);

			m_Shader = Shader::CreateFromFile("DeferredLight", "/CoreShaders/");

			TextureParameters param;
			param.filter = TextureFilter::LINEAR;
			param.format = TextureFormat::RGBA;
			param.wrap = TextureWrap::CLAMP_TO_EDGE;
			m_PreintegratedFG = Scope<Texture2D>(Texture2D::CreateFromFile("PreintegratedFG", "/CoreTextures/PreintegratedFG.png", param));

			m_LightUniformBuffer = nullptr;
			m_UniformBuffer = nullptr;

			m_ScreenQuad = Graphics::CreateQuad();

			m_DescriptorSet = nullptr;
			
			// Pixel/fragment shader System uniforms
			m_PSSystemUniformBufferSize = sizeof(Light) * MAX_LIGHTS + sizeof(Maths::Vector4) + sizeof(Maths::Matrix4) + (sizeof(Maths::Matrix4) + sizeof(Maths::Vector4))* MAX_SHADOWMAPS + sizeof(int) * 4;
			m_PSSystemUniformBuffer = lmnew u8[m_PSSystemUniformBufferSize];
			memset(m_PSSystemUniformBuffer, 0, m_PSSystemUniformBufferSize);
			m_PSSystemUniformBufferOffsets.resize(PSSystemUniformIndex_Size);

			// Per Scene System Uniforms
			m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_Lights]				= 0;
			m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_CameraPosition]		= m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_Lights] + sizeof(Light) * MAX_LIGHTS;
			m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ViewMatrix]			= m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_CameraPosition] + sizeof(Maths::Vector4);
			m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowTransforms]	= m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ViewMatrix] + sizeof(Maths::Matrix4);
			m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowSplitDepths]	= m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowTransforms] + sizeof(Maths::Matrix4) * MAX_SHADOWMAPS;
			m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_LightCount]			= m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowSplitDepths] + sizeof(Maths::Vector4) * MAX_SHADOWMAPS;
			m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowCount]		= m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_LightCount] + sizeof(int);
			m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_RenderMode]			= m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowCount] + sizeof(int);
            m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowMode]         = m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_RenderMode] + sizeof(int);

			m_RenderPass = Graphics::RenderPass::Create();

			AttachmentInfo textureTypes[2] = 
			{ 
				{ TextureType::COLOUR, TextureFormat::RGBA8 }
			};
			Graphics::RenderpassInfo renderpassCI{};
			renderpassCI.attachmentCount = 1;
			renderpassCI.textureType = textureTypes;

			m_RenderPass->Init(renderpassCI);

			m_CommandBuffers.resize(Renderer::GetSwapchain()->GetSwapchainBufferCount());

			for (auto& commandBuffer : m_CommandBuffers)
			{
				commandBuffer = Graphics::CommandBuffer::Create();
				commandBuffer->Init(true);
			}

			m_DeferredCommandBuffers = Graphics::CommandBuffer::Create();
			m_DeferredCommandBuffers->Init(true);

			CreateDeferredPipeline();
			CreateFramebuffers();
			CreateLightBuffer();

			Graphics::DescriptorInfo info{};
			info.pipeline = m_Pipeline;
			info.layoutIndex = 1; //?
			info.shader = m_Shader;
			m_DescriptorSet = Graphics::DescriptorSet::Create(info);

			m_ClearColour = Maths::Vector4(0.1f, 0.1f, 0.1f, 1.0f);

			CreateScreenDescriptorSet();
		}

		void DeferredRenderer::RenderScene(Scene* scene)
		{
			LUMOS_PROFILE_FUNC;

			m_OffScreenRenderer->RenderScene(scene);

			SubmitLightSetup(scene);

			SetSystemUniforms(m_Shader);

			int commandBufferIndex = 0;
			if (!m_RenderTexture)
				commandBufferIndex = Renderer::GetSwapchain()->GetCurrentBufferId();

			Begin(commandBufferIndex);
			Present();
			End();

			if (!m_RenderTexture)
				PresentToScreen();
		}

		void DeferredRenderer::PresentToScreen()
		{
			Renderer::Present((m_CommandBuffers[Renderer::GetSwapchain()->GetCurrentBufferId()]));
		}

		void DeferredRenderer::Begin(int commandBufferID)
		{
			m_CommandQueue.clear();
			m_SystemUniforms.clear();

			m_CommandBufferIndex = commandBufferID;

			m_CommandBuffers[m_CommandBufferIndex]->BeginRecording();

			m_RenderPass->BeginRenderpass(m_CommandBuffers[m_CommandBufferIndex], m_ClearColour, m_Framebuffers[m_CommandBufferIndex], Graphics::INLINE, m_ScreenBufferWidth, m_ScreenBufferHeight);
		}

		void DeferredRenderer::BeginScene(Scene* scene)
		{
			if (Application::Instance()->GetRenderManager()->GetSkyBox())
			{
				if (Application::Instance()->GetRenderManager()->GetSkyBox() != m_CubeMap)
				{
					SetCubeMap(Application::Instance()->GetRenderManager()->GetSkyBox());
					CreateScreenDescriptorSet();
				}
			}
			else
			{
				if (m_CubeMap)
				{
					m_CubeMap = nullptr;
					CreateScreenDescriptorSet();
				}
			}
		}

		void DeferredRenderer::Submit(const RenderCommand& command)
		{
			m_CommandQueue.push_back(command);
		}

		void DeferredRenderer::SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix)
		{
			RenderCommand command;
			command.mesh = mesh;
			command.transform = transform;
			command.textureMatrix = textureMatrix;
			command.material = material;
			Submit(command);
		}

		void DeferredRenderer::SubmitLightSetup(Scene* scene)
		{
			LUMOS_PROFILE_FUNC;

            auto& registry = scene->GetRegistry();
                      
            auto group = registry.group<Graphics::Light>(entt::get<Maths::Transform>);

            u32 numLights = 0;

			auto& frustum = scene->GetCamera()->GetFrustum();
			
			for(auto entity : group)
			{
				const auto &[light, trans] = group.get<Graphics::Light, Maths::Transform>(entity);

				if (light.m_Type != float(LightType::DirectionalLight))
				{
                    light.m_Position = trans.GetWorldPosition();

					auto inside = frustum.IsInsideFast(Maths::Sphere(light.m_Position.ToVector3(), light.m_Radius));

					if (inside == Maths::Intersection::OUTSIDE)
						continue;
				}

				Maths::Vector3 forward = Maths::Vector3::FORWARD;
				forward = trans.GetWorldOrientation() * forward;

				light.m_Direction = forward.Normalized();

				memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_Lights] + sizeof(Graphics::Light) * numLights, &light, sizeof(Graphics::Light));
				numLights++;
			}
            
            Maths::Vector4 cameraPos = Maths::Vector4(scene->GetCamera()->GetPosition());
            memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_CameraPosition], &cameraPos, sizeof(Maths::Vector4));
            
            auto shadowRenderer = Application::Instance()->GetRenderManager()->GetShadowRenderer();
            if (shadowRenderer)
            {
                Maths::Matrix4* shadowTransforms = shadowRenderer->GetShadowProjView();
                auto viewMat = scene->GetCamera()->GetViewMatrix();
                Lumos::Maths::Vector4* uSplitDepth = shadowRenderer->GetSplitDepths();
                
                memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ViewMatrix], &viewMat, sizeof(Maths::Matrix4));
                memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowTransforms], shadowTransforms, sizeof(Maths::Matrix4) * MAX_SHADOWMAPS);
                memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowSplitDepths], uSplitDepth, sizeof(Maths::Vector4) * MAX_SHADOWMAPS);
            }
            
            u32 numShadows = shadowRenderer ? shadowRenderer->GetShadowMapNum() : 0;
            
            memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_LightCount], &numLights, sizeof(int));
            memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowCount], &numShadows, sizeof(int));
            memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_RenderMode], &m_RenderMode, sizeof(int));
            memcpy(m_PSSystemUniformBuffer + m_PSSystemUniformBufferOffsets[PSSystemUniformIndex_ShadowMode], &m_ShadowMode, sizeof(int));
		}

		void DeferredRenderer::EndScene()
		{
		}

		void DeferredRenderer::End()
		{
			m_RenderPass->EndRenderpass(m_CommandBuffers[m_CommandBufferIndex]);
			m_CommandBuffers[m_CommandBufferIndex]->EndRecording();

			if (m_RenderTexture)
				m_CommandBuffers[0]->Execute(true);
		}

		void DeferredRenderer::SetSystemUniforms(Shader* shader) const
		{
			m_LightUniformBuffer->SetData(m_PSSystemUniformBufferSize, *&m_PSSystemUniformBuffer);
		}

		void DeferredRenderer::Present()
		{
			LUMOS_PROFILE_FUNC;
			Graphics::CommandBuffer* currentCMDBuffer = m_CommandBuffers[m_CommandBufferIndex];

			m_Pipeline->SetActive(currentCMDBuffer);

			std::vector<Graphics::DescriptorSet*> descriptorSets;
			descriptorSets.emplace_back(m_Pipeline->GetDescriptorSet());
			descriptorSets.emplace_back(m_DescriptorSet);

			m_ScreenQuad->GetVertexArray()->Bind(currentCMDBuffer);
			m_ScreenQuad->GetIndexBuffer()->Bind(currentCMDBuffer);

			Renderer::BindDescriptorSets(m_Pipeline, currentCMDBuffer, 0, descriptorSets);
			Renderer::DrawIndexed(currentCMDBuffer, DrawType::TRIANGLE, m_ScreenQuad->GetIndexBuffer()->GetCount());

			m_ScreenQuad->GetVertexArray()->Unbind();
			m_ScreenQuad->GetIndexBuffer()->Unbind();
		}

		void DeferredRenderer::CreateDeferredPipeline()
		{
			std::vector<Graphics::DescriptorPoolInfo> poolInfo =
			{
				{ Graphics::DescriptorType::IMAGE_SAMPLER , 1 },
				{ Graphics::DescriptorType::IMAGE_SAMPLER , 1 },
				{ Graphics::DescriptorType::IMAGE_SAMPLER , 1 },
				{ Graphics::DescriptorType::IMAGE_SAMPLER , 1 },
				{ Graphics::DescriptorType::IMAGE_SAMPLER , 1 },
				{ Graphics::DescriptorType::IMAGE_SAMPLER , 1 },
				{ Graphics::DescriptorType::IMAGE_SAMPLER , 1 },
				{ Graphics::DescriptorType::IMAGE_SAMPLER , 1 },
				{ Graphics::DescriptorType::UNIFORM_BUFFER, 1 }
			};

			std::vector<Graphics::DescriptorLayoutInfo> layoutInfo =
			{
				{ Graphics::DescriptorType::UNIFORM_BUFFER, Graphics::ShaderType::FRAGMENT, 0 },

			};

			std::vector<Graphics::DescriptorLayoutInfo> layoutInfoMesh =
			{
				 { Graphics::DescriptorType::IMAGE_SAMPLER,Graphics::ShaderType::FRAGMENT , 0 },
				 { Graphics::DescriptorType::IMAGE_SAMPLER,Graphics::ShaderType::FRAGMENT , 1 },
				 { Graphics::DescriptorType::IMAGE_SAMPLER,Graphics::ShaderType::FRAGMENT , 2 },
				 { Graphics::DescriptorType::IMAGE_SAMPLER,Graphics::ShaderType::FRAGMENT , 3 },
				 { Graphics::DescriptorType::IMAGE_SAMPLER,Graphics::ShaderType::FRAGMENT , 4 },
				 { Graphics::DescriptorType::IMAGE_SAMPLER,Graphics::ShaderType::FRAGMENT , 5 },
				 { Graphics::DescriptorType::IMAGE_SAMPLER,Graphics::ShaderType::FRAGMENT , 6 },
				 { Graphics::DescriptorType::IMAGE_SAMPLER,Graphics::ShaderType::FRAGMENT , 7 }
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
			pipelineCI.pipelineName = "Deferred";
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
			pipelineCI.cullMode = Graphics::CullMode::NONE;
			pipelineCI.transparencyEnabled = false;
			pipelineCI.depthBiasEnabled = false;
			pipelineCI.width = m_ScreenBufferWidth;
			pipelineCI.height = m_ScreenBufferHeight;
			pipelineCI.maxObjects = 10;

			m_Pipeline = Graphics::Pipeline::Create(pipelineCI);
		}

		void DeferredRenderer::SetRenderTarget(Texture* texture)
		{
			m_RenderTexture = texture;

			for (auto fbo : m_Framebuffers)
				delete fbo;
			m_Framebuffers.clear();

			CreateFramebuffers();
		}

		void DeferredRenderer::SetRenderToGBufferTexture(bool set)
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

		String RenderModeToString(int mode)
		{
			switch (mode)
			{
			case 0 : return "Lighting";
			case 1 : return "Colour";
			case 2 : return "Specular";
			case 3 : return "Roughness";
			case 4 : return "AO";
			case 5 : return "Emissive";
			case 6 : return "Normal";
            case 7 : return "Shadow Cascades";
			default: return "Lighting";
			}
		}
        
        String ShadowModeToString(int mode)
        {
            switch (mode)
            {
                case 0 : return "Normal";
                case 1 : return "PCF";
                default: return "Normal";
            }
        }

		void DeferredRenderer::OnImGui()
		{
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
			if (ImGui::BeginMenu(RenderModeToString(m_RenderMode).c_str()))
			{
                const int numRenderModes = 8;
                
                for(int i = 0; i < numRenderModes; i++)
                {
                        if (ImGui::MenuItem(RenderModeToString(i).c_str(), "", m_RenderMode == i, true)) { m_RenderMode = i; }
                }
				ImGui::EndMenu();
			}
			ImGui::PopItemWidth();
			ImGui::NextColumn();
            
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Shadow Mode");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::BeginMenu(ShadowModeToString(m_ShadowMode).c_str()))
            {
                if (ImGui::MenuItem(ShadowModeToString(0).c_str(), "", m_ShadowMode == 0, true)) { m_ShadowMode = 0; }
                if (ImGui::MenuItem(ShadowModeToString(1).c_str(), "", m_ShadowMode == 1, true)) { m_ShadowMode = 1; }
                
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

		void DeferredRenderer::CreateLightBuffer()
		{
			if (m_LightUniformBuffer == nullptr)
			{
				m_LightUniformBuffer = Graphics::UniformBuffer::Create();

				uint32_t bufferSize = m_PSSystemUniformBufferSize;
				m_LightUniformBuffer->Init(bufferSize, nullptr);
			}

			std::vector<Graphics::BufferInfo> bufferInfos;

			Graphics::BufferInfo bufferInfo = {};
			bufferInfo.name = "LightData";
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
			LUMOS_PROFILE_FUNC;
			delete m_Pipeline;

			for (auto fbo : m_Framebuffers)
				delete fbo;
			m_Framebuffers.clear();

			if (m_RenderToGBufferTexture)
				m_RenderTexture = Application::Instance()->GetRenderManager()->GetGBuffer()->GetTexture(SCREENTEX_OFFSCREEN0);

			DeferredRenderer::SetScreenBufferSize(width, height);

			CreateDeferredPipeline();
			CreateFramebuffers();
			CreateLightBuffer();

			Graphics::DescriptorInfo info{};
			info.pipeline = m_Pipeline;
			info.layoutIndex = 1; //?
			info.shader = m_Shader;
			if (m_DescriptorSet)
				delete m_DescriptorSet;
			m_DescriptorSet = Graphics::DescriptorSet::Create(info);

			m_OffScreenRenderer->OnResize(width, height);

			m_CubeMap = nullptr;

			m_ClearColour = Maths::Vector4(0.8f, 0.8f, 0.8f, 1.0f);
		}

		void DeferredRenderer::SetCubeMap(Texture* cubeMap)
		{
			m_CubeMap = cubeMap;
		}

		void DeferredRenderer::CreateScreenDescriptorSet()
		{
			std::vector<Graphics::ImageInfo> bufferInfos;

			Graphics::ImageInfo imageInfo = {};
			imageInfo.texture = { Application::Instance()->GetRenderManager()->GetGBuffer()->GetTexture(SCREENTEX_COLOUR) };
			imageInfo.binding = 0;
			imageInfo.name = "uColourSampler";

			Graphics::ImageInfo imageInfo2 = {};
			imageInfo2.texture = { Application::Instance()->GetRenderManager()->GetGBuffer()->GetTexture(SCREENTEX_POSITION) };
			imageInfo2.binding = 1;
			imageInfo2.name = "uPositionSampler";

			Graphics::ImageInfo imageInfo3 = {};
			imageInfo3.texture = { Application::Instance()->GetRenderManager()->GetGBuffer()->GetTexture(SCREENTEX_NORMALS) };
			imageInfo3.binding = 2;
			imageInfo3.name = "uNormalSampler";

			Graphics::ImageInfo imageInfo4 = {};
			imageInfo4.texture = { Application::Instance()->GetRenderManager()->GetGBuffer()->GetTexture(SCREENTEX_PBR) };
			imageInfo4.binding = 3;
			imageInfo4.name = "uPBRSampler";

			Graphics::ImageInfo imageInfo5 = {};
			imageInfo5.texture = { m_PreintegratedFG.get() };
			imageInfo5.binding = 4;
			imageInfo5.name = "uPreintegratedFG";

			Graphics::ImageInfo imageInfo6 = {};
			imageInfo6.texture = { m_CubeMap };
			imageInfo6.binding = 5;
			imageInfo6.type = TextureType::CUBE;
			imageInfo6.name = "uEnvironmentMap";

			Graphics::ImageInfo imageInfo7 = {};
			auto shadowRenderer = Application::Instance()->GetRenderManager()->GetShadowRenderer();
			if (shadowRenderer)
			{
				imageInfo7.texture = { reinterpret_cast<Texture*>(shadowRenderer->GetTexture()) };
				imageInfo7.binding = 6;
				imageInfo7.type = TextureType::DEPTHARRAY;
				imageInfo7.name = "uShadowMap";
			}

			Graphics::ImageInfo imageInfo8 = {};
			imageInfo8.texture = { Application::Instance()->GetRenderManager()->GetGBuffer()->GetDepthTexture() };
			imageInfo8.binding = 7;
			imageInfo8.type = TextureType::DEPTH;
			imageInfo8.name = "uDepthSampler";

			bufferInfos.push_back(imageInfo);
			bufferInfos.push_back(imageInfo2);
			bufferInfos.push_back(imageInfo3);
			bufferInfos.push_back(imageInfo4);
			bufferInfos.push_back(imageInfo5);
			if (m_CubeMap)
				bufferInfos.push_back(imageInfo6);
			if (shadowRenderer)
				bufferInfos.push_back(imageInfo7);

			m_DescriptorSet->Update(bufferInfos);
		}
	}
}
