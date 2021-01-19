#include "Precompiled.h"
#include "Renderer2D.h"
#include "Graphics/API/Shader.h"
#include "Graphics/API/Framebuffer.h"
#include "Graphics/API/UniformBuffer.h"
#include "Graphics/API/Renderer.h"
#include "Graphics/API/CommandBuffer.h"
#include "Graphics/API/Swapchain.h"
#include "Graphics/API/RenderPass.h"
#include "Graphics/API/Pipeline.h"
#include "Graphics/API/IndexBuffer.h"
#include "Graphics/API/Texture.h"
#include "Graphics/GBuffer.h"
#include "Graphics/Sprite.h"
#include "Graphics/AnimatedSprite.h"
#include "Scene/Scene.h"
#include "Core/Application.h"
#include "RenderGraph.h"
#include "Platform/OpenGL/GLDescriptorSet.h"
#include "Graphics/Renderable2D.h"
#include "Graphics/Camera/Camera.h"
#include "Maths/Transform.h"
#include "Core/Engine.h"

namespace Lumos
{
	namespace Graphics
	{
		Renderer2D::Renderer2D(u32 width, u32 height, bool clear, bool triangleIndicies, bool renderToDepth)
			: m_IndexCount(0)
			, m_Buffer(nullptr)
			, m_Clear(clear)
			, m_RenderToDepthTexture(renderToDepth)
			, m_TriangleIndicies(triangleIndicies)
		{
			m_Limits.SetMaxQuads(10000);
			m_Limits.MaxQuads = 16; //Renderer::GetCapabilities().MaxTextureUnits;

			Renderer2D::SetScreenBufferSize(width, height);
			Renderer2D::Init();
		}

		Renderer2D::~Renderer2D()
		{
			delete m_IndexBuffer;
			delete m_UniformBuffer;

			delete[] m_VSSystemUniformBuffer;
			for(u32 i = 0; i < m_Limits.MaxBatchDrawCalls; i++)
			{
				delete m_VertexBuffers[i];
				delete m_SecondaryCommandBuffers[i];
			}
		}

		void Renderer2D::Init()
		{
			LUMOS_PROFILE_FUNCTION();
            m_Shader = Application::Get().GetShaderLibrary()->GetResource("/CoreShaders/Batch2D.shader");

			m_TransformationStack.emplace_back(Maths::Matrix4());
			m_TransformationBack = &m_TransformationStack.back();

			m_VSSystemUniformBufferSize = sizeof(Maths::Matrix4);
			m_VSSystemUniformBuffer = new u8[m_VSSystemUniformBufferSize];

			m_UniformBuffer = Graphics::UniformBuffer::Create();

			AttachmentInfo textureTypes[2] =
				{
					{TextureType::COLOUR, TextureFormat::RGBA8}};

			if(m_RenderToDepthTexture)
			{
				textureTypes[1] = {TextureType::DEPTH, TextureFormat::DEPTH};
			}

			Graphics::RenderPassInfo renderpassCI;
			renderpassCI.attachmentCount = m_RenderToDepthTexture ? 2 : 1;
			renderpassCI.textureType = textureTypes;
			renderpassCI.clear = m_Clear;

            m_RenderPass = Graphics::RenderPass::Get(renderpassCI);

			CreateFramebuffers();

			m_CommandBuffers.resize(Renderer::GetSwapchain()->GetSwapchainBufferCount());

			for(auto& commandBuffer : m_CommandBuffers)
			{
				commandBuffer = Ref<Graphics::CommandBuffer>(Graphics::CommandBuffer::Create());
				commandBuffer->Init(true);
			}

			m_SecondaryCommandBuffers.resize(m_Limits.MaxBatchDrawCalls);

			for(auto& cmdBuffer : m_SecondaryCommandBuffers)
			{
				cmdBuffer = Graphics::CommandBuffer::Create();
				cmdBuffer->Init(false);
			}

			CreateGraphicsPipeline();

			uint32_t bufferSize = static_cast<uint32_t>(sizeof(Maths::Matrix4));
			m_UniformBuffer->Init(bufferSize, nullptr);

			std::vector<Graphics::BufferInfo> bufferInfos;

			Graphics::BufferInfo bufferInfo;
			bufferInfo.buffer = m_UniformBuffer;
			bufferInfo.offset = 0;
			bufferInfo.size = bufferSize;
			bufferInfo.type = Graphics::DescriptorType::UNIFORM_BUFFER;
			bufferInfo.shaderType = ShaderType::VERTEX;
			bufferInfo.name = "UniformBufferObject";
			bufferInfo.binding = 0;

			bufferInfos.push_back(bufferInfo);

			m_Pipeline->GetDescriptorSet()->Update(bufferInfos);

			Graphics::DescriptorInfo info{};
            info.pipeline = m_Pipeline.get();
			info.layoutIndex = 1; //?
            info.shader = m_Shader.get();
			m_DescriptorSet = Graphics::DescriptorSet::Create(info);

			m_VertexBuffers.resize(m_Limits.MaxBatchDrawCalls);

			for(auto& vertexBuffer : m_VertexBuffers)
			{
				vertexBuffer = Graphics::VertexBuffer::Create(BufferUsage::DYNAMIC);
				vertexBuffer->Resize(m_Limits.BufferSize);
			}

			u32* indices = new u32[m_Limits.IndiciesSize];

			if(m_TriangleIndicies)
			{
				for(u32 i = 0; i < m_Limits.IndiciesSize; i++)
				{
					indices[i] = i;
				}
			}
			else
			{
				u32 offset = 0;
				for(u32 i = 0; i < m_Limits.IndiciesSize; i += 6)
				{
					indices[i] = offset + 0;
					indices[i + 1] = offset + 1;
					indices[i + 2] = offset + 2;

					indices[i + 3] = offset + 2;
					indices[i + 4] = offset + 3;
					indices[i + 5] = offset + 0;

					offset += 4;
				}
			}
			m_IndexBuffer = IndexBuffer::Create(indices, m_Limits.IndiciesSize);

			delete[] indices;
            
			m_ClearColour = Maths::Vector4(0.2f, 0.2f, 0.2f, 1.0f);
            m_CurrentDescriptorSets.resize(2);
		}

		void Renderer2D::Submit(Renderable2D* renderable, const Maths::Matrix4& transform)
		{
			LUMOS_PROFILE_FUNCTION();
            
            RenderCommand2D command;
            command.renderable = renderable;
            command.transform = transform;
            m_CommandQueue2D.push_back(command);
		}

		void Renderer2D::SubmitTriangle(const Maths::Vector3& p1, const Maths::Vector3& p2, const Maths::Vector3& p3, const Maths::Vector4& colour)
		{
			m_Triangles.emplace_back(p1, p2, p3, colour);
		}

		void Renderer2D::SubmitInternal(const TriangleInfo& triangleInfo)
		{
			LUMOS_PROFILE_FUNCTION();
			if(m_IndexCount >= m_Limits.IndiciesSize)
				FlushAndReset();

			float textureSlot = 0.0f;

			m_Buffer->vertex = triangleInfo.p1;
			m_Buffer->uv = {0.0f, 0.0f};
			m_Buffer->tid = Maths::Vector2(textureSlot, 0.0f);
			m_Buffer->color = triangleInfo.col;
			m_Buffer++;

			m_Buffer->vertex = triangleInfo.p2;
			m_Buffer->uv = {0.0f, 0.0f};
			m_Buffer->tid = Maths::Vector2(textureSlot, 0.0f);
			m_Buffer->color = triangleInfo.col;
			m_Buffer++;

			m_Buffer->vertex = triangleInfo.p3;
			m_Buffer->uv = {0.0f, 0.0f};
			m_Buffer->tid = Maths::Vector2(textureSlot, 0.0f);
			m_Buffer->color = triangleInfo.col;
			m_Buffer++;

			m_IndexCount += 3;
		}

		void Renderer2D::BeginSimple()
		{
			LUMOS_PROFILE_FUNCTION();
            m_TextureCount = 0;
			m_Triangles.clear();
		}

		void Renderer2D::BeginRenderPass()
		{
			LUMOS_PROFILE_FUNCTION();
			m_CurrentBufferID = 0;
			if(!m_RenderTexture)
				m_CurrentBufferID = Renderer::GetSwapchain()->GetCurrentBufferId();

            m_RenderPass->BeginRenderpass(m_CommandBuffers[m_CurrentBufferID].get(), m_ClearColour, m_Framebuffers[m_CurrentBufferID].get(), Graphics::SECONDARY, m_ScreenBufferWidth, m_ScreenBufferHeight);

            m_VertexBuffers[m_BatchDrawCallIndex]->Bind(m_CommandBuffers[m_CurrentBufferID].get(), m_Pipeline.get());
			m_Buffer = m_VertexBuffers[m_BatchDrawCallIndex]->GetPointer<VertexData>();
		}

		void Renderer2D::Begin()
		{
			LUMOS_PROFILE_FUNCTION();
			m_CurrentBufferID = 0;
			if(!m_RenderTexture)
				m_CurrentBufferID = Renderer::GetSwapchain()->GetCurrentBufferId();

			m_RenderPass->BeginRenderpass(m_CommandBuffers[m_CurrentBufferID].get(), m_ClearColour, m_Framebuffers[m_CurrentBufferID].get(), Graphics::SECONDARY, m_ScreenBufferWidth, m_ScreenBufferHeight);

			m_TextureCount = 0;
			m_Triangles.clear();

            m_VertexBuffers[m_BatchDrawCallIndex]->Bind(m_CommandBuffers[m_CurrentBufferID].get(), m_Pipeline.get());
			m_Buffer = m_VertexBuffers[m_BatchDrawCallIndex]->GetPointer<VertexData>();
		}

		void Renderer2D::SetSystemUniforms(Shader* shader) const
		{
			LUMOS_PROFILE_FUNCTION();
            m_UniformBuffer->SetData(sizeof(Maths::Matrix4), *&m_VSSystemUniformBuffer);
		}

		void Renderer2D::BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform)
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

            auto view = m_CameraTransform->GetWorldMatrix().Inverse();
			auto projView = m_Camera->GetProjectionMatrix() * view;

			memcpy(m_VSSystemUniformBuffer, &projView, sizeof(Maths::Matrix4));

			m_Frustum = m_Camera->GetFrustum(view);
            m_CommandQueue2D.clear();
            
            auto group = registry.group<Graphics::Sprite>(entt::get<Maths::Transform>);
            for(auto entity : group)
            {
                const auto& [sprite, trans] = group.get<Graphics::Sprite, Maths::Transform>(entity);

                auto bb = Maths::BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetPosition() + sprite.GetScale()));
                bb.Transform(trans.GetWorldMatrix());
                auto inside = m_Frustum.IsInside(bb);

                if(inside == Maths::Intersection::OUTSIDE)
                    continue;

                Submit(&sprite, trans.GetWorldMatrix());
            };
            
            auto group2 = registry.group<Graphics::AnimatedSprite>(entt::get<Maths::Transform>);
            for(auto entity : group2)
            {
                const auto& [sprite, trans] = group2.get<Graphics::AnimatedSprite, Maths::Transform>(entity);
                
                auto bb = Maths::BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetPosition() + sprite.GetScale()));
                bb.Transform(trans.GetWorldMatrix());
                auto inside = m_Frustum.IsInside(bb);
                
                if(inside == Maths::Intersection::OUTSIDE)
                    continue;
                
                Submit(&sprite, trans.GetWorldMatrix());
            };
		}

		void Renderer2D::Present()
		{
			LUMOS_PROFILE_FUNCTION();
            if(m_IndexCount == 0)
            {
                m_VertexBuffers[m_BatchDrawCallIndex]->ReleasePointer();
                m_Empty = true;
                return;
            }

            m_Empty = false;
			UpdateDesciptorSet();

			Graphics::CommandBuffer* currentCMDBuffer = m_SecondaryCommandBuffers[m_BatchDrawCallIndex];

			currentCMDBuffer->BeginRecordingSecondary(m_RenderPass.get(), m_Framebuffers[m_CurrentBufferID].get());
			currentCMDBuffer->UpdateViewport(m_ScreenBufferWidth, m_ScreenBufferHeight);
			m_Pipeline->Bind(currentCMDBuffer);

			m_VertexBuffers[m_BatchDrawCallIndex]->ReleasePointer();
			m_VertexBuffers[m_BatchDrawCallIndex]->Unbind();

			m_IndexBuffer->SetCount(m_IndexCount);

            m_CurrentDescriptorSets[0] = m_Pipeline->GetDescriptorSet();
            m_CurrentDescriptorSets[1] = m_DescriptorSet.get();

			m_VertexBuffers[m_BatchDrawCallIndex]->Bind(currentCMDBuffer, m_Pipeline.get());
			m_IndexBuffer->Bind(currentCMDBuffer);

			Renderer::BindDescriptorSets(m_Pipeline.get(), currentCMDBuffer, 0, m_CurrentDescriptorSets);
			Renderer::DrawIndexed(currentCMDBuffer, DrawType::TRIANGLE, m_IndexCount);

			m_VertexBuffers[m_BatchDrawCallIndex]->Unbind();
			m_IndexBuffer->Unbind();

			m_IndexCount = 0;

			currentCMDBuffer->EndRecording();
			currentCMDBuffer->ExecuteSecondary(m_CommandBuffers[m_CurrentBufferID].get());

			m_BatchDrawCallIndex++;
		}

		void Renderer2D::End()
		{
			LUMOS_PROFILE_FUNCTION();
			m_RenderPass->EndRenderpass(m_CommandBuffers[m_CurrentBufferID].get());

			if(m_RenderTexture)
				m_CommandBuffers[m_CurrentBufferID]->Execute(true);

			if(!m_RenderTexture && !m_Empty)
				PresentToScreen();

			m_BatchDrawCallIndex = 0;
		}
    
        void Renderer2D::SubmitQueue()
        {
            for(auto& command : m_CommandQueue2D)
            {
                Engine::Get().Statistics().NumRenderedObjects++;
                    
                if(m_IndexCount >= m_Limits.IndiciesSize)
                    FlushAndReset();
                
                auto& renderable = command.renderable;
                auto& transform = command.transform;

                const Maths::Vector2 min = renderable->GetPosition();
                const Maths::Vector2 max = renderable->GetPosition() + renderable->GetScale();

                const Maths::Vector4 colour = renderable->GetColour();
                const auto& uv = renderable->GetUVs();
                const Texture* texture = renderable->GetTexture();

                float textureSlot = 0.0f;
                if(texture)
                    textureSlot = SubmitTexture(renderable->GetTexture());

                Maths::Vector3 vertex = transform * Maths::Vector3(min.x, min.y, 0.0f);
                m_Buffer->vertex = vertex;
                m_Buffer->uv = uv[0];
                m_Buffer->tid = Maths::Vector2(textureSlot, 0.0f);
                m_Buffer->color = colour;
                m_Buffer++;

                vertex = transform * Maths::Vector3(max.x, min.y, 0.0f);
                m_Buffer->vertex = vertex;
                m_Buffer->uv = uv[1];
                m_Buffer->tid = Maths::Vector2(textureSlot, 0.0f);
                m_Buffer->color = colour;
                m_Buffer++;

                vertex = transform * Maths::Vector3(max.x, max.y, 0.0f);
                m_Buffer->vertex = vertex;
                m_Buffer->uv = uv[2];
                m_Buffer->tid = Maths::Vector2(textureSlot, 0.0f);
                m_Buffer->color = colour;
                m_Buffer++;

                vertex = transform * Maths::Vector3(min.x, max.y, 0.0f);
                m_Buffer->vertex = vertex;
                m_Buffer->uv = uv[3];
                m_Buffer->tid = Maths::Vector2(textureSlot, 0.0f);
                m_Buffer->color = colour;
                m_Buffer++;

                m_IndexCount += 6;
            }
        }

		void Renderer2D::RenderScene()
		{
			LUMOS_PROFILE_FUNCTION();
			Begin();

			SetSystemUniforms(m_Shader.get());
            SubmitQueue();
			Present();

			End();
		}

		float Renderer2D::SubmitTexture(Texture* texture)
		{
			LUMOS_PROFILE_FUNCTION();
			float result = 0.0f;
			bool found = false;
			for(u32 i = 0; i < m_TextureCount; i++)
			{
				if(m_Textures[i] == texture) //Temp
				{
					result = static_cast<float>(i + 1);
					found = true;
					break;
				}
			}

			if(!found)
			{
				if(m_TextureCount >= m_Limits.MaxTextures)
				{
					FlushAndReset();
				}
				m_Textures[m_TextureCount] = texture;
                m_TextureCount++;
				result = static_cast<float>(m_TextureCount);
			}
			return result;
		}

		void Renderer2D::OnResize(u32 width, u32 height)
		{
			LUMOS_PROFILE_FUNCTION();
			m_Framebuffers.clear();

			SetScreenBufferSize(width, height);

			CreateFramebuffers();
		}

		void Renderer2D::PresentToScreen()
		{
			LUMOS_PROFILE_FUNCTION();
			Renderer::Present((m_CommandBuffers[Renderer::GetSwapchain()->GetCurrentBufferId()].get()));
		}

		void Renderer2D::SetScreenBufferSize(u32 width, u32 height)
		{
			LUMOS_PROFILE_FUNCTION();
			if(width == 0)
			{
				width = 1;
				LUMOS_LOG_CRITICAL("Width 0");
			}
			if(height == 0)
			{
				height = 1;
				LUMOS_LOG_CRITICAL("Height 0");
			}
			m_ScreenBufferWidth = width;
			m_ScreenBufferHeight = height;
		}

		void Renderer2D::CreateGraphicsPipeline()
		{
			LUMOS_PROFILE_FUNCTION();
            Graphics::BufferLayout vertexBufferLayout;
            vertexBufferLayout.Push<Maths::Vector3>("position");
            vertexBufferLayout.Push<Maths::Vector2>("uv");
            vertexBufferLayout.Push<Maths::Vector2>("tid");
            vertexBufferLayout.Push<Maths::Vector4>("colour");

			Graphics::PipelineInfo pipelineCreateInfo;
			pipelineCreateInfo.shader = m_Shader;
			pipelineCreateInfo.renderpass = m_RenderPass;
            pipelineCreateInfo.vertexBufferLayout = vertexBufferLayout;
			pipelineCreateInfo.polygonMode = Graphics::PolygonMode::FILL;
			pipelineCreateInfo.cullMode = Graphics::CullMode::BACK;
			pipelineCreateInfo.transparencyEnabled = true;
			pipelineCreateInfo.depthBiasEnabled = false;

			m_Pipeline = Graphics::Pipeline::Get(pipelineCreateInfo);
		}

		void Renderer2D::CreateFramebuffers()
		{
			LUMOS_PROFILE_FUNCTION();
			TextureType attachmentTypes[2];
			attachmentTypes[0] = TextureType::COLOUR;
			Texture* attachments[2];

			if(m_RenderToDepthTexture)
			{
				attachmentTypes[1] = TextureType::DEPTH;
				attachments[1] = reinterpret_cast<Texture*>(Application::Get().GetRenderGraph()->GetGBuffer()->GetDepthTexture());
			}

			FramebufferInfo bufferInfo{};
			bufferInfo.width = m_ScreenBufferWidth;
			bufferInfo.height = m_ScreenBufferHeight;
			bufferInfo.attachmentCount = m_RenderToDepthTexture ? 2 : 1;
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

		void Renderer2D::UpdateDesciptorSet()
		{
			LUMOS_PROFILE_FUNCTION();
			if(m_TextureCount == 0)
				return;
			std::vector<Graphics::ImageInfo> imageInfos;

			Graphics::ImageInfo imageInfo = {};

			imageInfo.binding = 0;
			imageInfo.name = "textures";
            if(m_TextureCount > 1)
                imageInfo.textures = m_Textures;
            else
                imageInfo.texture = m_Textures[0];
			imageInfo.count = m_TextureCount;

			imageInfos.push_back(imageInfo);

			m_DescriptorSet->Update(imageInfos);
		}

		void Renderer2D::SetRenderTarget(Texture* texture, bool rebuildFramebuffer)
		{
			LUMOS_PROFILE_FUNCTION();
			m_RenderTexture = texture;
			if(rebuildFramebuffer)
			{
				m_Framebuffers.clear();
				CreateFramebuffers();
			}
		}

		void Renderer2D::FlushAndReset()
		{
			LUMOS_PROFILE_FUNCTION();
			Present();

			m_TextureCount = 0;
			m_Triangles.clear();

			m_VertexBuffers[m_BatchDrawCallIndex]->Bind(nullptr, nullptr);
			m_Buffer = m_VertexBuffers[m_BatchDrawCallIndex]->GetPointer<VertexData>();
		}

		void Renderer2D::SubmitTriangles()
		{
			LUMOS_PROFILE_FUNCTION();
			for(auto& triangle : m_Triangles)
				SubmitInternal(triangle);
		}
	}
}
