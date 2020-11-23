#include "Precompiled.h"
#include "VKRenderer.h"
#include "VKDevice.h"
#include "VKShader.h"
#include "VKDescriptorSet.h"
#include "VKTools.h"
#include "VKPipeline.h"
#include "Core/Engine.h"
 
namespace Lumos
{
	namespace Graphics
	{
		void VKRenderer::InitInternal()
		{
			LUMOS_PROFILE_FUNCTION();

			m_RendererTitle = "Vulkan";

			CreateSemaphores();
		}

		VKRenderer::~VKRenderer()
		{
			for(int i = 0; i < NUM_SEMAPHORES; i++)
			{
				vkDestroySemaphore(VKDevice::Get().GetDevice(), m_ImageAvailableSemaphore[i], nullptr);
                vkDestroySemaphore(VKDevice::Get().GetDevice(), m_RenderFinishedSemaphore[i], nullptr);

			}
		}

		void VKRenderer::PresentInternal(CommandBuffer* cmdBuffer)
		{
			LUMOS_PROFILE_FUNCTION();
            TracyVkCollect(VKDevice::Get().GetTracyContext(), static_cast<VKCommandBuffer*>(cmdBuffer)->GetCommandBuffer());
			static_cast<VKCommandBuffer*>(cmdBuffer)->ExecuteInternal(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				m_ImageAvailableSemaphore[m_CurrentSemaphoreIndex],
				m_RenderFinishedSemaphore[m_CurrentSemaphoreIndex],
				true);
            m_CurrentSemaphoreIndex = (m_CurrentSemaphoreIndex + 1) % NUM_SEMAPHORES;
		}

		void VKRenderer::ClearSwapchainImage() const
		{
			LUMOS_PROFILE_FUNCTION();
            
            auto m_Swapchain = VKContext::Get()->GetSwapchain();
			for(int i = 0; i < m_Swapchain->GetSwapchainBufferCount(); i++)
			{
				auto cmd = VKTools::BeginSingleTimeCommands();

				VkImageSubresourceRange subresourceRange = {};
				subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				subresourceRange.baseMipLevel = 0;
				subresourceRange.layerCount = 1;
				subresourceRange.levelCount = 1;

				VkClearColorValue clearColorValue = VkClearColorValue({{0.0f, 0.0f, 0.0f, 0.0f}});

				vkCmdClearColorImage(cmd, static_cast<VKTexture2D*>(m_Swapchain->GetImage(i))->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, &clearColorValue, 1, &subresourceRange);

				VKTools::EndSingleTimeCommands(cmd);
			}
		}

		void VKRenderer::PresentInternal()
		{
			LUMOS_PROFILE_FUNCTION();
            VKContext::Get()->GetSwapchain()->Present(m_ImageAvailableSemaphore[m_CurrentSemaphoreIndex]);
		}

		void VKRenderer::OnResize(u32 width, u32 height)
		{
			LUMOS_PROFILE_FUNCTION();
			if(width == 0 || height == 0)
				return;

			m_Width = width;
			m_Height = height;
			
			VKContext::Get()->OnResize(m_Width, m_Height);
        }

		void VKRenderer::CreateSemaphores()
		{
			LUMOS_PROFILE_FUNCTION();
			VkSemaphoreCreateInfo semaphoreInfo = {};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			semaphoreInfo.pNext = nullptr;

			for(int i = 0; i < NUM_SEMAPHORES; i++)
			{
				VK_CHECK_RESULT(vkCreateSemaphore(VKDevice::Get().GetDevice(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphore[i]));
                VK_CHECK_RESULT(vkCreateSemaphore(VKDevice::Get().GetDevice(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphore[i]));
			}
		}
    
        Swapchain* VKRenderer::GetSwapchainInternal() const
        {
            return VKContext::Get()->GetSwapchain().get();
        }

		void VKRenderer::Begin()
		{
			LUMOS_PROFILE_FUNCTION();
            auto m_Swapchain = VKContext::Get()->GetSwapchain();
			
			auto result = m_Swapchain->AcquireNextImage(m_ImageAvailableSemaphore[m_CurrentSemaphoreIndex]);
			if(result == VK_ERROR_OUT_OF_DATE_KHR)
			{
				OnResize(m_Width, m_Height);
				return;
			}
			else if(result == VK_SUBOPTIMAL_KHR)
			{
				LUMOS_LOG_WARN("[VULKAN] Swapchain Image - SubOptimal!");
			}
			else if(result != VK_SUCCESS)
			{
				LUMOS_LOG_CRITICAL("[VULKAN] Failed to acquire swap chain image!");
			}
		}

		const std::string& VKRenderer::GetTitleInternal() const
		{
			return m_RendererTitle;
		}

		void VKRenderer::BindDescriptorSetsInternal(Graphics::Pipeline* pipeline, Graphics::CommandBuffer* cmdBuffer, u32 dynamicOffset, std::vector<Graphics::DescriptorSet*>& descriptorSets)
		{
			LUMOS_PROFILE_FUNCTION();
			u32 numDynamicDescriptorSets = 0;
			u32 numDesciptorSets = 0;

			for(auto descriptorSet : descriptorSets)
			{
				auto vkDesSet = static_cast<Graphics::VKDescriptorSet*>(descriptorSet);
				if(vkDesSet->GetIsDynamic())
					numDynamicDescriptorSets++;

				m_DescriptorSetPool[numDesciptorSets] = vkDesSet->GetDescriptorSet();

				u32 index = 0;
				for(auto& pc : vkDesSet->GetPushConstants())
				{
					vkCmdPushConstants(static_cast<Graphics::VKCommandBuffer*>(cmdBuffer)->GetCommandBuffer(), static_cast<Graphics::VKPipeline*>(pipeline)->GetPipelineLayout(), VKTools::ShaderTypeToVK(pc.shaderStage), index, pc.size, pc.data);
				}

				numDesciptorSets++;
			}

			vkCmdBindDescriptorSets(static_cast<Graphics::VKCommandBuffer*>(cmdBuffer)->GetCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, static_cast<Graphics::VKPipeline*>(pipeline)->GetPipelineLayout(), 0, numDesciptorSets, m_DescriptorSetPool, numDynamicDescriptorSets, &dynamicOffset);
		}

		void VKRenderer::DrawIndexedInternal(CommandBuffer* commandBuffer, DrawType type, u32 count, u32 start) const
		{
			LUMOS_PROFILE_FUNCTION();
			Engine::Get().Statistics().NumDrawCalls++;
			vkCmdDrawIndexed(static_cast<VKCommandBuffer*>(commandBuffer)->GetCommandBuffer(), count, 1, 0, 0, 0);
		}

		void VKRenderer::DrawInternal(CommandBuffer* commandBuffer, DrawType type, u32 count, DataType datayType, void* indices) const
		{
			LUMOS_PROFILE_FUNCTION();
            Engine::Get().Statistics().NumDrawCalls++;
			vkCmdDraw(static_cast<VKCommandBuffer*>(commandBuffer)->GetCommandBuffer(), count, 1, 0, 0);
		}

		void VKRenderer::MakeDefault()
		{
			CreateFunc = CreateFuncVulkan;
		}

		Renderer* VKRenderer::CreateFuncVulkan(u32 width, u32 height)
		{
			return new VKRenderer(width, height);
		}
	}
}
