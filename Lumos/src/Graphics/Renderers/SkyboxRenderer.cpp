#include "lmpch.h"
#include "SkyboxRenderer.h"
#include "Graphics/API/Shader.h"
#include "Graphics/API/Framebuffer.h"
#include "Graphics/API/Texture.h"
#include "Graphics/API/UniformBuffer.h"
#include "Graphics/API/Renderer.h"
#include "Graphics/API/CommandBuffer.h"
#include "Graphics/API/Swapchain.h"
#include "Graphics/API/RenderPass.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/API/Pipeline.h"
#include "Graphics/GBuffer.h"
#include "Graphics/Mesh.h"
#include "Graphics/MeshFactory.h"
#include "Graphics/RenderManager.h"
#include "App/Scene.h"
#include "App/Application.h"
#include "Graphics/Camera/Camera.h"

#include <imgui/imgui.h>

namespace Lumos
{
	namespace Graphics
	{
		SkyboxRenderer::SkyboxRenderer(u32 width, u32 height, Texture* cubeMap, bool renderToGBuffer) : m_UniformBuffer(nullptr), m_CubeMap(nullptr)
		{
			m_Pipeline = nullptr;
			m_CubeMap = cubeMap;

			SetScreenBufferSize(width, height);
			Init();
            
            SetRenderToGBufferTexture(renderToGBuffer);
		}

		SkyboxRenderer::~SkyboxRenderer()
		{
			delete m_Shader;
			delete m_UniformBuffer;
			delete m_Skybox;
			delete m_Pipeline;
			delete m_RenderPass;
			delete[] m_VSSystemUniformBuffer;

			for (auto& commandBuffer : m_CommandBuffers)
			{
				delete commandBuffer;
			}

			for (auto& fbo : m_Framebuffers)
			{
				delete fbo;
			}

			m_Framebuffers.clear();
			m_CommandBuffers.clear();
		}

		void SkyboxRenderer::RenderScene(Scene* scene)
		{
			m_CurrentBufferID = 0;
			if (!m_RenderTexture)
				m_CurrentBufferID = Renderer::GetSwapchain()->GetCurrentBufferId();

			Begin();
			SetSystemUniforms(m_Shader);
			m_Pipeline->SetActive(m_CommandBuffers[m_CurrentBufferID]);

			std::vector<Graphics::DescriptorSet*> descriptorSets;
			descriptorSets.emplace_back(m_Pipeline->GetDescriptorSet());

			m_Skybox->GetVertexArray()->Bind(m_CommandBuffers[m_CurrentBufferID]);
			m_Skybox->GetIndexBuffer()->Bind(m_CommandBuffers[m_CurrentBufferID]);

			Renderer::BindDescriptorSets(m_Pipeline, m_CommandBuffers[m_CurrentBufferID], 0, descriptorSets);
			Renderer::DrawIndexed(m_CommandBuffers[m_CurrentBufferID], DrawType::TRIANGLE, m_Skybox->GetIndexBuffer()->GetCount());

			m_Skybox->GetVertexArray()->Unbind();
			m_Skybox->GetIndexBuffer()->Unbind();

			End();

			if (!m_RenderTexture)
				Renderer::Present((m_CommandBuffers[Renderer::GetSwapchain()->GetCurrentBufferId()]));
		}

		enum VSSystemUniformIndices : i32
		{
			VSSystemUniformIndex_InverseProjectionViewMatrix = 0,
			VSSystemUniformIndex_Size
		};

		void SkyboxRenderer::Init()
		{
			m_Shader = Shader::CreateFromFile("Skybox", "/CoreShaders/");
			m_Skybox = Graphics::CreateQuad();

			// Vertex shader System uniforms
			m_VSSystemUniformBufferSize = sizeof(Maths::Matrix4);
			m_VSSystemUniformBuffer = lmnew u8[m_VSSystemUniformBufferSize];
			memset(m_VSSystemUniformBuffer, 0, m_VSSystemUniformBufferSize);
			m_VSSystemUniformBufferOffsets.resize(VSSystemUniformIndex_Size);

			// Per Scene System Uniforms
			m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_InverseProjectionViewMatrix] = 0;

			m_CommandBuffers.resize(2);

			for (auto& commandBuffer : m_CommandBuffers)
			{
				commandBuffer = Graphics::CommandBuffer::Create();
				commandBuffer->Init(true);
			}

			m_RenderPass = Graphics::RenderPass::Create();
			AttachmentInfo textureTypes[2] =
			{
				{ TextureType::COLOUR, TextureFormat::RGBA8 },
				{ TextureType::DEPTH , TextureFormat::DEPTH }
			};

			Graphics::RenderpassInfo renderpassCI{};
			renderpassCI.attachmentCount = 2;
			renderpassCI.textureType = textureTypes;
			renderpassCI.clear = false;

			m_RenderPass->Init(renderpassCI);

			CreateGraphicsPipeline();
			UpdateUniformBuffer();
			CreateFramebuffers();
		}

		void SkyboxRenderer::Begin()
		{
			m_CommandBuffers[m_CurrentBufferID]->BeginRecording();

			m_RenderPass->BeginRenderpass(m_CommandBuffers[m_CurrentBufferID], Maths::Vector4(0.0f), m_Framebuffers[m_CurrentBufferID], Graphics::INLINE, m_ScreenBufferWidth, m_ScreenBufferHeight);
		}

		void SkyboxRenderer::BeginScene(Scene* scene)
		{
			auto camera = scene->GetCamera();
			auto proj = camera->GetProjectionMatrix();

			auto invViewProj = Maths::Matrix4::Inverse(proj * camera->GetViewMatrix());
			memcpy(m_VSSystemUniformBuffer + m_VSSystemUniformBufferOffsets[VSSystemUniformIndex_InverseProjectionViewMatrix], &invViewProj, sizeof(Maths::Matrix4));
		}

		void SkyboxRenderer::End()
		{
			m_RenderPass->EndRenderpass(m_CommandBuffers[m_CurrentBufferID]);
			m_CommandBuffers[m_CurrentBufferID]->EndRecording();


			if (m_RenderTexture)
				m_CommandBuffers[m_CurrentBufferID]->Execute(true);
		}

		void SkyboxRenderer::SetSystemUniforms(Shader* shader) const
		{
			m_UniformBuffer->SetData(sizeof(UniformBufferObject), *&m_VSSystemUniformBuffer);
		}

		void SkyboxRenderer::SetRenderToGBufferTexture(bool set)
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

		void SkyboxRenderer::OnResize(u32 width, u32 height)
		{
			delete m_Pipeline;

			for (auto fbo : m_Framebuffers)
				delete fbo;
			m_Framebuffers.clear();

			if (m_RenderToGBufferTexture)
				m_RenderTexture = Application::Instance()->GetRenderManager()->GetGBuffer()->GetTexture(SCREENTEX_OFFSCREEN0);

			SetScreenBufferSize(width, height);

			CreateGraphicsPipeline();
			UpdateUniformBuffer();
			CreateFramebuffers();
		}

		void SkyboxRenderer::CreateGraphicsPipeline()
		{
			std::vector<Graphics::DescriptorPoolInfo> poolInfo =
			{
				{ Graphics::DescriptorType::UNIFORM_BUFFER, 1 },
				{ Graphics::DescriptorType::IMAGE_SAMPLER, 1 }
			};

			std::vector<Graphics::DescriptorLayoutInfo> layoutInfo =
			{
				{ Graphics::DescriptorType::UNIFORM_BUFFER, Graphics::ShaderType::VERTEX, 0 },
				{ Graphics::DescriptorType::IMAGE_SAMPLER,Graphics::ShaderType::FRAGMENT , 1 }
			};

			auto attributeDescriptions = Vertex::getAttributeDescriptions();

			std::vector<Graphics::DescriptorLayout> descriptorLayouts;

			Graphics::DescriptorLayout sceneDescriptorLayout{};
			sceneDescriptorLayout.count = static_cast<u32>(layoutInfo.size());
			sceneDescriptorLayout.layoutInfo = layoutInfo.data();

			descriptorLayouts.push_back(sceneDescriptorLayout);

			Graphics::PipelineInfo pipelineCI{};
			pipelineCI.pipelineName = "SkyRenderer";
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
			pipelineCI.maxObjects = 1;

			m_Pipeline = Graphics::Pipeline::Create(pipelineCI);
		}

		void SkyboxRenderer::UpdateUniformBuffer()
		{
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
			bufferInfo.binding = 0;
			bufferInfo.shaderType = ShaderType::VERTEX;
			bufferInfo.systemUniforms = true;

			bufferInfos.push_back(bufferInfo);

			std::vector<Graphics::ImageInfo> imageInfos;


			if (m_CubeMap)
			{
				Graphics::ImageInfo imageInfo = {};
				imageInfo.texture = { m_CubeMap };
				imageInfo.name = "u_CubeMap";
				imageInfo.binding = 1;
				imageInfo.type = TextureType::CUBE;

				imageInfos.push_back(imageInfo);
			}

			if (m_Pipeline != nullptr)
				m_Pipeline->GetDescriptorSet()->Update(imageInfos, bufferInfos);
		}

		void SkyboxRenderer::SetCubeMap(Texture* cubeMap)
		{
			m_CubeMap = cubeMap;
			UpdateUniformBuffer();
		}

		void SkyboxRenderer::SetRenderTarget(Texture* texture)
		{
			m_RenderTexture = texture;

			for (auto fbo : m_Framebuffers)
				delete fbo;
			m_Framebuffers.clear();

			CreateFramebuffers();
		}

		void SkyboxRenderer::CreateFramebuffers()
		{
			TextureType attachmentTypes[2];
			attachmentTypes[0] = TextureType::COLOUR;
			attachmentTypes[1] = TextureType::DEPTH;

			Texture* attachments[2];
			FramebufferInfo bufferInfo{};
			bufferInfo.width = m_ScreenBufferWidth;
			bufferInfo.height = m_ScreenBufferHeight;
			bufferInfo.attachmentCount = 2;
			bufferInfo.renderPass = m_RenderPass;
			bufferInfo.attachmentTypes = attachmentTypes;

			attachments[1] = dynamic_cast<Texture*>(Application::Instance()->GetRenderManager()->GetGBuffer()->GetDepthTexture());

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

		void SkyboxRenderer::OnImGui()
		{
			ImGui::TextUnformatted("Skybox Renderer");
			if (ImGui::TreeNode("CubeMap"))
			{
				bool flipImage = Graphics::GraphicsContext::GetContext()->FlipImGUITexture();

				ImGui::Image(m_CubeMap->GetHandle(), ImVec2(128, 128), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));

				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::Image(m_CubeMap->GetHandle(), ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
					ImGui::EndTooltip();
				}

				ImGui::TreePop();
			}
		}
	}
}
