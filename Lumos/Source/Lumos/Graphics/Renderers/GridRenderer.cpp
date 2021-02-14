#include "Precompiled.h"
#include "GridRenderer.h"
#include "Graphics/API/Shader.h"
#include "Graphics/API/Framebuffer.h"
#include "Graphics/API/Texture.h"
#include "Graphics/API/UniformBuffer.h"
#include "Graphics/API/Renderer.h"
#include "Graphics/API/CommandBuffer.h"
#include "Graphics/API/Swapchain.h"
#include "Graphics/API/RenderPass.h"
#include "Graphics/API/Pipeline.h"
#include "Graphics/GBuffer.h"
#include "Graphics/Mesh.h"
#include "Graphics/MeshFactory.h"
#include "Scene/Scene.h"
#include "Core/Application.h"
#include "Graphics/Camera/Camera.h"
#include "Maths/Transform.h"
#include "Graphics/Renderers/RenderGraph.h"

#include <imgui/imgui.h>

namespace Lumos
{
	namespace Graphics
	{
		GridRenderer::GridRenderer(uint32_t width, uint32_t height)
			: m_UniformBuffer(nullptr)
			, m_UniformBufferFrag(nullptr)
		{
			m_Pipeline = nullptr;
            
			IRenderer::SetScreenBufferSize(width, height);
			GridRenderer::Init();
            
			m_GridRes = 1.4f;
			m_GridSize = 500.0f;
		}
        
		GridRenderer::~GridRenderer()
		{
			delete m_Quad;
			delete m_UniformBuffer;
			delete m_UniformBufferFrag;
			delete[] m_VSSystemUniformBuffer;
            delete[] m_PSSystemUniformBuffer;
		}
        
		void GridRenderer::RenderScene()
		{
			LUMOS_PROFILE_FUNCTION();
			m_CurrentBufferID = 0;
			if(!m_RenderTexture)
				m_CurrentBufferID = Renderer::GetSwapchain()->GetCurrentBufferId();
            
			Begin();
            
			SetSystemUniforms(m_Shader.get());
            
            m_Pipeline->Bind(m_CommandBuffers[m_CurrentBufferID].get());
            
            m_CurrentDescriptorSets[0] = m_Pipeline->GetDescriptorSet();
            
			m_Quad->GetVertexBuffer()->Bind(m_CommandBuffers[m_CurrentBufferID].get(), m_Pipeline.get());
			m_Quad->GetIndexBuffer()->Bind(m_CommandBuffers[m_CurrentBufferID].get());
            
			Renderer::BindDescriptorSets(m_Pipeline.get(), m_CommandBuffers[m_CurrentBufferID].get(), 0, m_CurrentDescriptorSets);
			Renderer::DrawIndexed(m_CommandBuffers[m_CurrentBufferID].get(), DrawType::TRIANGLE, m_Quad->GetIndexBuffer()->GetCount());
            
			m_Quad->GetVertexBuffer()->Unbind();
			m_Quad->GetIndexBuffer()->Unbind();
            
			End();
            
			if(!m_RenderTexture)
				Renderer::Present((m_CommandBuffers[Renderer::GetSwapchain()->GetCurrentBufferId()].get()));
		}
        
		enum VSSystemUniformIndices : int32_t
		{
			VSSystemUniformIndex_InverseProjectionViewMatrix = 0,
			VSSystemUniformIndex_Size
		};
        
		void GridRenderer::Init()
		{
			LUMOS_PROFILE_FUNCTION();
			m_Shader = Application::Get().GetShaderLibrary()->GetResource("/CoreShaders/Grid.shader");
			m_Quad = Graphics::CreatePlane(5000.0f, 5000.f, Maths::Vector3(0.0f, 1.0f, 0.0f));
            
			// Vertex shader System uniforms
			m_VSSystemUniformBufferSize = sizeof(Maths::Matrix4);
			m_VSSystemUniformBuffer = new uint8_t[m_VSSystemUniformBufferSize];
			memset(m_VSSystemUniformBuffer, 0, m_VSSystemUniformBufferSize);
            
			m_PSSystemUniformBufferSize = sizeof(UniformBufferObjectFrag);
			m_PSSystemUniformBuffer = new uint8_t[m_PSSystemUniformBufferSize];
			memset(m_PSSystemUniformBuffer, 0, m_PSSystemUniformBufferSize);
            
			m_CommandBuffers.resize(Renderer::GetSwapchain()->GetSwapchainBufferCount());
            
			for(auto& commandBuffer : m_CommandBuffers)
			{
				commandBuffer = Ref<Graphics::CommandBuffer>(Graphics::CommandBuffer::Create());
				commandBuffer->Init(true);
			}
            
			AttachmentInfo textureTypes[2] =
            {
                {TextureType::COLOUR, TextureFormat::RGBA8},
				{TextureType::DEPTH, TextureFormat::DEPTH},
            };
            
			Graphics::RenderPassInfo renderpassCI;
			renderpassCI.attachmentCount = 2;
			renderpassCI.textureType = textureTypes;
			renderpassCI.clear = false;
            
            m_RenderPass = Graphics::RenderPass::Get(renderpassCI);

			CreateGraphicsPipeline();
			UpdateUniformBuffer();
			CreateFramebuffers();
            
            m_CurrentDescriptorSets.resize(1);
		}
        
		void GridRenderer::Begin()
		{
			LUMOS_PROFILE_FUNCTION();
			m_RenderPass->BeginRenderpass(m_CommandBuffers[m_CurrentBufferID].get(), Maths::Vector4(0.0f), m_Framebuffers[m_CurrentBufferID].get(), Graphics::INLINE, m_ScreenBufferWidth, m_ScreenBufferHeight);
		}
        
		void GridRenderer::BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform)
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
            
			auto proj = m_Camera->GetProjectionMatrix();
            
			UniformBufferObjectFrag test;
			test.res = m_GridRes;
			test.scale = m_GridSize;
			test.cameraPos = m_CameraTransform->GetWorldPosition();
			test.maxDistance = m_MaxDistance;
            
			auto invViewProj = proj * m_CameraTransform->GetWorldMatrix().Inverse();
			memcpy(m_VSSystemUniformBuffer, &invViewProj, sizeof(Maths::Matrix4));
			memcpy(m_PSSystemUniformBuffer, &test, sizeof(UniformBufferObjectFrag));
		}
        
		void GridRenderer::End()
		{
			LUMOS_PROFILE_FUNCTION();
			m_RenderPass->EndRenderpass(m_CommandBuffers[m_CurrentBufferID].get());
            
			if(m_RenderTexture)
				m_CommandBuffers[m_CurrentBufferID]->Execute(true);
		}
        
		void GridRenderer::SetSystemUniforms(Shader* shader) const
		{
			LUMOS_PROFILE_FUNCTION();
			m_UniformBuffer->SetData(sizeof(UniformBufferObject), *&m_VSSystemUniformBuffer);
			m_UniformBufferFrag->SetData(sizeof(UniformBufferObjectFrag), *&m_PSSystemUniformBuffer);
		}
        
		void GridRenderer::OnImGui()
		{
			LUMOS_PROFILE_FUNCTION();
			ImGui::TextUnformatted("Grid Renderer");
            
			if(ImGui::TreeNode("Parameters"))
			{
				ImGui::DragFloat("Resolution", &m_GridRes, 1.0f, 0.0f, 10.0f);
				ImGui::DragFloat("Scale", &m_GridSize, 1.0f, 1.0f, 10000.0f);
				ImGui::DragFloat("Max Distance", &m_MaxDistance, 1.0f, 1.0f, 10000.0f);
                
				ImGui::TreePop();
			}
		}
        
		void GridRenderer::OnResize(uint32_t width, uint32_t height)
		{
			LUMOS_PROFILE_FUNCTION();
			m_Framebuffers.clear();
            
			SetScreenBufferSize(width, height);
            
			UpdateUniformBuffer();
			CreateFramebuffers();
		}
        
		void GridRenderer::CreateGraphicsPipeline()
		{
			LUMOS_PROFILE_FUNCTION();
                        
            Graphics::BufferLayout vertexBufferLayout;
            vertexBufferLayout.Push<Maths::Vector3>("position");
            vertexBufferLayout.Push<Maths::Vector4>("colour");
            vertexBufferLayout.Push<Maths::Vector2>("uv");
            vertexBufferLayout.Push<Maths::Vector3>("normal");
            vertexBufferLayout.Push<Maths::Vector3>("tangent");

			Graphics::PipelineInfo pipelineCreateInfo;
			pipelineCreateInfo.shader = m_Shader;
			pipelineCreateInfo.renderpass = m_RenderPass;
            pipelineCreateInfo.vertexBufferLayout = vertexBufferLayout;
			pipelineCreateInfo.polygonMode = Graphics::PolygonMode::FILL;
			pipelineCreateInfo.cullMode = Graphics::CullMode::NONE;
			pipelineCreateInfo.transparencyEnabled = true;
			pipelineCreateInfo.depthBiasEnabled = false;
            
			m_Pipeline = Graphics::Pipeline::Get(pipelineCreateInfo);
		}
        
		void GridRenderer::UpdateUniformBuffer()
		{
			LUMOS_PROFILE_FUNCTION();
			if(m_UniformBuffer == nullptr)
			{
				m_UniformBuffer = Graphics::UniformBuffer::Create();
				uint32_t bufferSize = static_cast<uint32_t>(sizeof(UniformBufferObject));
				m_UniformBuffer->Init(bufferSize, nullptr);
			}
            
			std::vector<Graphics::BufferInfo> bufferInfos;
            
			Graphics::BufferInfo bufferInfo;
			bufferInfo.name = "UniformBufferObject";
			bufferInfo.buffer = m_UniformBuffer;
			bufferInfo.offset = 0;
			bufferInfo.size = sizeof(UniformBufferObject);
			bufferInfo.type = Graphics::DescriptorType::UNIFORM_BUFFER;
			bufferInfo.binding = 0;
			bufferInfo.shaderType = ShaderType::VERTEX;
            
			if(m_UniformBufferFrag == nullptr)
			{
				m_UniformBufferFrag = Graphics::UniformBuffer::Create();
				uint32_t bufferSize = static_cast<uint32_t>(sizeof(UniformBufferObjectFrag));
				m_UniformBufferFrag->Init(bufferSize, nullptr);
			}
            
			Graphics::BufferInfo bufferInfo2;
			bufferInfo2.name = "UniformBuffer";
			bufferInfo2.buffer = m_UniformBufferFrag;
			bufferInfo2.offset = 0;
			bufferInfo2.size = sizeof(UniformBufferObjectFrag);
			bufferInfo2.type = Graphics::DescriptorType::UNIFORM_BUFFER;
			bufferInfo2.binding = 1;
			bufferInfo2.shaderType = ShaderType::FRAGMENT;
            
			bufferInfos.push_back(bufferInfo);
			bufferInfos.push_back(bufferInfo2);
			if(m_Pipeline != nullptr)
				m_Pipeline->GetDescriptorSet()->Update(bufferInfos);
		}
        
		void GridRenderer::SetRenderTarget(Texture* texture, bool rebuildFramebuffer)
		{
			LUMOS_PROFILE_FUNCTION();
			m_RenderTexture = texture;
            
			if(!rebuildFramebuffer)
				return;
            
			m_Framebuffers.clear();
            
			CreateFramebuffers();
		}
        
		void GridRenderer::CreateFramebuffers()
		{
			LUMOS_PROFILE_FUNCTION();
			TextureType attachmentTypes[2];
			attachmentTypes[0] = TextureType::COLOUR;
            attachmentTypes[1] = TextureType::DEPTH;
			
			Texture* attachments[2];
			FramebufferInfo bufferInfo{};
			bufferInfo.width = m_ScreenBufferWidth;
			bufferInfo.height = m_ScreenBufferHeight;
			bufferInfo.attachmentCount = 2;
			bufferInfo.renderPass = m_RenderPass.get();
			bufferInfo.attachmentTypes = attachmentTypes;
			
			attachments[1] = Application::Get().GetRenderGraph()->GetGBuffer()->GetDepthTexture();
            
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
