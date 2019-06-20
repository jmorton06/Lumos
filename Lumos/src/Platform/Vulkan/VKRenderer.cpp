#include "LM.h"
#include "VKRenderer.h"
#include "VKDevice.h"
#include "VKShader.h"
#include "VKVertexBuffer.h"
#include "VKIndexBuffer.h"
#include "VKDescriptorSet.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"
#include "System/System.h"

namespace Lumos
{
	namespace Graphics
	{
		void VKRenderer::InitInternal()
		{
			m_Context = VKContext::Get();

			m_RendererTitle = "Vulkan";

            VKDevice::Instance();
            
            m_Swapchain = new VKSwapchain(m_Width, m_Height);
            m_Swapchain->Init();
            
            CreateSemaphores();
		}

        VKRenderer::~VKRenderer()
		{
            delete m_Swapchain;
            
			for (int i = 0; i < 5; i++)
			{
				VKDevice::Instance()->GetDevice().destroySemaphore(m_ImageAvailableSemaphore[i]);
			}
            
            m_Context->Unload();
		}

		void VKRenderer::PresentInternal(CommandBuffer* cmdBuffer)
		{
			dynamic_cast<VKCommandBuffer*>(cmdBuffer)->ExecuteInternal(vk::PipelineStageFlagBits::eColorAttachmentOutput,
				m_ImageAvailableSemaphore[m_CurrentSemaphoreIndex], m_ImageAvailableSemaphore[m_CurrentSemaphoreIndex + 1], false);
			m_CurrentSemaphoreIndex++;
		}

		void VKRenderer::ClearSwapchainImage() const
		{
			for(int i = 0; i < m_Swapchain->GetSwapchainBufferCount(); i++)
			{
				auto cmd = VKTools::BeginSingleTimeCommands();

				vk::ImageSubresourceRange subresourceRange = {};
				subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
				subresourceRange.baseMipLevel = 0;
				subresourceRange.layerCount = 1;
				subresourceRange.levelCount = 1;

				vk::ClearColorValue clearColorValue = vk::ClearColorValue(std::array<float, 4> { 0.0f, 0.0f, 0.0f, 0.0f });

				cmd.clearColorImage(static_cast<VKTexture2D*>(m_Swapchain->GetImage(i))->GetImage(), vk::ImageLayout::eTransferSrcOptimal, &clearColorValue, 1, &subresourceRange);

				VKTools::EndSingleTimeCommands(cmd);
			}
		}

		void VKRenderer::PresentInternal()
        {
			m_Swapchain->Present(m_ImageAvailableSemaphore[m_CurrentSemaphoreIndex]);
			//VKDevice::Instance()->GetPresentQueue().waitIdle();
        }

		void VKRenderer::OnResize(uint width, uint height)
		{
			if (width == 0 || height == 0) return;

			m_Width  = width;
			m_Height = height;

			delete m_Swapchain;
			m_Swapchain = new VKSwapchain(width, height);
			m_Swapchain->Init();
		}

		void VKRenderer::CreateSemaphores()
		{
			vk::SemaphoreCreateInfo semaphoreInfo = {};

			for (int i = 0; i < 5; i++)
			{
				m_ImageAvailableSemaphore[i] = VKDevice::Instance()->GetDevice().createSemaphore(semaphoreInfo);
				if (!m_ImageAvailableSemaphore[i])
				{
					throw std::runtime_error("failed to create semaphores!");
				}
			}
		}

		void VKRenderer::Render(IndexBuffer* indexBuffer, VertexArray* vertexArray,
			VKCommandBuffer* commandBuffer, std::vector<vk::DescriptorSet>& descriptorSet, vk::PipelineLayout layout, uint32_t offset, uint numDynamicDescriptorSets)
		{
			uint vertexArraySize = vertexArray->GetCount();
			vk::Buffer* vertexBuffers = new vk::Buffer[vertexArraySize];
			vk::DeviceSize* offsets = new vk::DeviceSize[vertexArraySize];
			for (uint i = 0; i < vertexArraySize; i++)
			{
				auto* buffer = dynamic_cast<VKVertexBuffer*>(vertexArray->GetBuffer(i));
				vertexBuffers[i] = buffer->GetBuffer();
				offsets[i] = static_cast<vk::DeviceSize>(buffer->GetLayout().GetStride());
			}

			commandBuffer->GetCommandBuffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 0, static_cast<uint32_t>(descriptorSet.size()), descriptorSet.data(), numDynamicDescriptorSets, &offset);
			commandBuffer->GetCommandBuffer().bindVertexBuffers(0,1, vertexBuffers, offsets);
			commandBuffer->GetCommandBuffer().bindIndexBuffer(dynamic_cast<VKIndexBuffer*>(indexBuffer)->GetBuffer(), 0, vk::IndexType::eUint32);
			commandBuffer->GetCommandBuffer().drawIndexed(static_cast<uint32_t>(indexBuffer->GetCount()), 1, 0, 0, 0);

			delete[] vertexBuffers;
			delete[] offsets;
		}

		void VKRenderer::Begin()
		{
			m_CurrentSemaphoreIndex = 0;
			auto result = m_Swapchain->AcquireNextImage(m_ImageAvailableSemaphore[m_CurrentSemaphoreIndex]);

			if (result == vk::Result::eErrorOutOfDateKHR)
			{
				OnResize(m_Width, m_Height);
				return;
			}
			else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
			{
				throw std::runtime_error("failed to acquire swap chain image!");
			}

			ClearSwapchainImage();
		}
		void VKRenderer::BindScreenFBOInternal()
		{
		}
		void VKRenderer::ClearInternal(uint buffer)
		{
		}
		void VKRenderer::SetColourMaskInternal(bool r, bool g, bool b, bool a)
		{
		}
		void VKRenderer::SetDepthTestingInternal(bool enabled)
		{
		}
		void VKRenderer::SetStencilTestInternal(bool enabled)
		{
		}
		void VKRenderer::SetCullingInternal(bool enabled, bool front)
		{
		}
		void VKRenderer::SetBlendInternal(bool enabled)
		{
		}
		void VKRenderer::SetDepthMaskInternal(bool enabled)
		{
		}
		void VKRenderer::SetViewportInternal(uint x, uint y, uint width, uint height)
		{
		}
		void VKRenderer::SetBlendFunctionInternal(RendererBlendFunction source, RendererBlendFunction destination)
		{
		}
		void VKRenderer::SetBlendEquationInternal(RendererBlendFunction blendEquation)
		{
		}
		void VKRenderer::SetStencilFunctionInternal(StencilType type, uint ref, uint mask)
		{
		}
		void VKRenderer::SetStencilOpInternal(StencilType fail, StencilType zfail, StencilType zpass)
		{
		}
		const String & VKRenderer::GetTitleInternal() const
		{
			return m_RendererTitle;
		}
		void VKRenderer::DrawArraysInternal(DrawType type, uint numIndices) const
		{
		}
		void VKRenderer::DrawArraysInternal(DrawType type, uint start, uint numIndices) const
		{
		}
		void VKRenderer::DrawInternal(DrawType type, uint count, DataType datayType, void * indices) const
		{
		}
		void VKRenderer::SetRenderTargets(uint numTargets)
		{
		}

		void VKRenderer::SetPixelPackType(PixelPackType type)
		{
		}
		void VKRenderer::SetRenderModeInternal(RenderMode mode)
		{
		}

		void VKRenderer::RenderMeshInternal(Mesh *mesh, Graphics::Pipeline *pipeline, Graphics::CommandBuffer* cmdBuffer, uint dynamicOffset, std::vector<Graphics::DescriptorSet*>& descriptorSets)
		{
			std::vector<vk::DescriptorSet> vkdescriptorSets;
			uint numDynamicDescriptorSets = 0;

			for (auto descriptorSet : descriptorSets)
			{
				auto vkDescriptorSet = dynamic_cast<Graphics::VKDescriptorSet*>(descriptorSet);
				if (vkDescriptorSet->GetIsDynamic())
					numDynamicDescriptorSets++;

				vkdescriptorSets.push_back(vkDescriptorSet->GetDescriptorSet());

				uint index = 0;
				for (auto pc : vkDescriptorSet->GetPushConstants())
				{
					dynamic_cast<Graphics::VKCommandBuffer*>(cmdBuffer)->GetCommandBuffer().pushConstants(dynamic_cast<Graphics::VKPipeline*>(pipeline)->GetPipelineLayout(), VKTools::ShaderTypeToVK(pc.shaderStage), index, pc.size, pc.data);
				}
			}

			Graphics::VKRenderer::Render(mesh->GetIndexBuffer().get(), mesh->GetVertexArray().get(), dynamic_cast<Graphics::VKCommandBuffer*>(cmdBuffer), vkdescriptorSets,
			                             dynamic_cast<Graphics::VKPipeline*>(pipeline)->GetPipelineLayout(), dynamicOffset, numDynamicDescriptorSets);

		}

		void VKRenderer::Render(VertexArray* vertexArray, IndexBuffer* indexBuffer,
			Graphics::CommandBuffer* cmdBuffer, std::vector<Graphics::DescriptorSet*>& descriptorSets,
			Graphics::Pipeline* pipeline, uint dynamicOffset)
		{
			std::vector<vk::DescriptorSet> vkdescriptorSets;
			uint numDynamicDescriptorSets = 0;

			for(auto descriptorSet : descriptorSets)
			{
				if (dynamic_cast<Graphics::VKDescriptorSet*>(descriptorSet)->GetIsDynamic())
					numDynamicDescriptorSets++;

				vkdescriptorSets.push_back(dynamic_cast<Graphics::VKDescriptorSet*>(descriptorSet)->GetDescriptorSet());

				uint index = 0;
				for (auto pc : dynamic_cast<Graphics::VKDescriptorSet*>(descriptorSet)->GetPushConstants())
				{
					dynamic_cast<Graphics::VKCommandBuffer*>(cmdBuffer)->GetCommandBuffer().pushConstants(dynamic_cast<Graphics::VKPipeline*>(pipeline)->GetPipelineLayout(), VKTools::ShaderTypeToVK(pc.shaderStage), index, pc.size, pc.data);
				}
			}

			Render(indexBuffer, vertexArray, dynamic_cast<Graphics::VKCommandBuffer*>(cmdBuffer), vkdescriptorSets, dynamic_cast<Graphics::VKPipeline*>(pipeline)->GetPipelineLayout(), dynamicOffset, numDynamicDescriptorSets);

		}
	}
}
