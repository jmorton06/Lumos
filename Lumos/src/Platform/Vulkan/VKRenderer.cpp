#include "lmpch.h"
#include "VKRenderer.h"
#include "VKDevice.h"
#include "VKShader.h"
#include "VKDescriptorSet.h"

namespace Lumos
{
	namespace Graphics
	{
		void VKRenderer::InitInternal()
		{
			m_Context = VKContext::Get();

			m_RendererTitle = "Vulkan";

            VKDevice::Instance();
            
            m_Swapchain = CreateRef<VKSwapchain>(m_Width, m_Height);
            m_Swapchain->Init();
            
            CreateSemaphores();
		}

        VKRenderer::~VKRenderer()
		{
			m_Swapchain.reset();

			for (int i = 0; i < 5; i++)
			{
				vkDestroySemaphore(VKDevice::Instance()->GetDevice(), m_ImageAvailableSemaphore[i], nullptr);
			}
            
            m_Context->Unload();

			VKDevice::Release();
		}

		void VKRenderer::PresentInternal(CommandBuffer* cmdBuffer)
		{
			dynamic_cast<VKCommandBuffer*>(cmdBuffer)->ExecuteInternal(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				m_ImageAvailableSemaphore[m_CurrentSemaphoreIndex], m_ImageAvailableSemaphore[m_CurrentSemaphoreIndex + 1], false);
			m_CurrentSemaphoreIndex++;
		}

		void VKRenderer::ClearSwapchainImage() const
		{
			for(int i = 0; i < m_Swapchain->GetSwapchainBufferCount(); i++)
			{
				auto cmd = VKTools::BeginSingleTimeCommands();

				VkImageSubresourceRange subresourceRange = {};	
				subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				subresourceRange.baseMipLevel = 0;
				subresourceRange.layerCount = 1;
				subresourceRange.levelCount = 1;

				VkClearColorValue clearColorValue = VkClearColorValue({ 0.0f, 0.0f, 0.0f, 0.0f });

				vkCmdClearColorImage(cmd, static_cast<VKTexture2D*>(m_Swapchain->GetImage(i))->GetImage(),VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, &clearColorValue, 1, &subresourceRange);

				VKTools::EndSingleTimeCommands(cmd);
			}
		}

		void VKRenderer::PresentInternal()
        {
			m_Swapchain->Present(m_ImageAvailableSemaphore[m_CurrentSemaphoreIndex]);
        }

		void VKRenderer::OnResize(u32 width, u32 height)
		{
			if (width == 0 || height == 0) return;

			m_Width  = width;
			m_Height = height;

			m_Swapchain = CreateRef<VKSwapchain>(width, height);
			m_Swapchain->Init();
		}

		void VKRenderer::CreateSemaphores()
		{
			VkSemaphoreCreateInfo semaphoreInfo = {};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			semaphoreInfo.pNext = nullptr;

			for (int i = 0; i < 5; i++)
			{
				auto result = vkCreateSemaphore(VKDevice::Instance()->GetDevice(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphore[i]);
				if (result != VK_SUCCESS)
				{
					LUMOS_LOG_CRITICAL("[VULKAN] Failed to create semaphores!");
				}
			}
		}

		void VKRenderer::Begin()
		{
			m_CurrentSemaphoreIndex = 0;
			auto result = m_Swapchain->AcquireNextImage(m_ImageAvailableSemaphore[m_CurrentSemaphoreIndex]);

			if (result == VK_ERROR_OUT_OF_DATE_KHR)
			{
				OnResize(m_Width, m_Height);
				return;
			}
			else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			{
				LUMOS_LOG_CRITICAL("[VULKAN] Failed to acquire swap chain image!");
			}
		}

		const String & VKRenderer::GetTitleInternal() const
		{
			return m_RendererTitle;
		}

		void VKRenderer::BindDescriptorSetsInternal(Graphics::Pipeline* pipeline, Graphics::CommandBuffer* cmdBuffer, u32 dynamicOffset, std::vector<Graphics::DescriptorSet*>& descriptorSets)
		{
			u32 numDynamicDescriptorSets = 0;
			u32 numDesciptorSets = 0;

			for (auto descriptorSet : descriptorSets)
			{
				auto vkDesSet = static_cast<Graphics::VKDescriptorSet*>(descriptorSet);
				if (vkDesSet->GetIsDynamic())
					numDynamicDescriptorSets++;

				m_DescriptorSetPool[numDesciptorSets] = vkDesSet->GetDescriptorSet();

				u32 index = 0;
				for (auto& pc : vkDesSet->GetPushConstants())
				{
					vkCmdPushConstants(static_cast<Graphics::VKCommandBuffer*>(cmdBuffer)->GetCommandBuffer(), static_cast<Graphics::VKPipeline*>(pipeline)->GetPipelineLayout(), VKTools::ShaderTypeToVK(pc.shaderStage), index, pc.size, pc.data);
				}

				numDesciptorSets++;
			}

			vkCmdBindDescriptorSets(static_cast<Graphics::VKCommandBuffer*>(cmdBuffer)->GetCommandBuffer(),VK_PIPELINE_BIND_POINT_GRAPHICS, static_cast<Graphics::VKPipeline*>(pipeline)->GetPipelineLayout(), 0, numDesciptorSets, m_DescriptorSetPool, numDynamicDescriptorSets, &dynamicOffset);
		}

		void VKRenderer::DrawIndexedInternal(CommandBuffer* commandBuffer, DrawType type, u32 count, u32 start) const
		{
			vkCmdDrawIndexed(static_cast<VKCommandBuffer*>(commandBuffer)->GetCommandBuffer(), count, 1, 0, 0, 0);
		}

		void VKRenderer::DrawInternal(CommandBuffer* commandBuffer, DrawType type, u32 count, DataType datayType, void* indices) const
		{
			vkCmdDraw(static_cast<VKCommandBuffer*>(commandBuffer)->GetCommandBuffer(), count, 1, 0, 0);
		}
        
        void VKRenderer::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
        }
        
		Renderer* VKRenderer::CreateFuncVulkan(u32 width, u32 height)
        {
            return lmnew VKRenderer(width, height);
        }
	}
}
