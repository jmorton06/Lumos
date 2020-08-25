#include "lmpch.h"
#include "VKCommandBuffer.h"

#include "VKDevice.h"
#include "VKCommandPool.h"
#include "VKFramebuffer.h"
#include "VKTools.h"

namespace Lumos
{
	namespace Graphics
	{
		VKCommandBuffer::VKCommandBuffer(): m_CommandBuffer(nullptr), m_Fence(VK_NULL_HANDLE), m_Primary(false)
		{
		}

		VKCommandBuffer::~VKCommandBuffer()
		{
			Unload();
		}

		bool VKCommandBuffer::Init(bool primary)
		{
			m_Primary = primary;

			VkCommandBufferAllocateInfo cmdBufferCI{};

			cmdBufferCI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmdBufferCI.commandPool = VKDevice::Get().GetVKContext()->GetCommandPool()->GetCommandPool();
			cmdBufferCI.commandBufferCount = 1;
			cmdBufferCI.level = primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;

			VK_CHECK_RESULT(vkAllocateCommandBuffers(VKDevice::Get().GetDevice(), &cmdBufferCI, &m_CommandBuffer));

			VkFenceCreateInfo fenceCI{};
			fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			VK_CHECK_RESULT(vkCreateFence(VKDevice::Get().GetDevice(), &fenceCI, nullptr, &m_Fence));

			return true;
		}

		void VKCommandBuffer::Unload()
		{
			vkDestroyFence(VKDevice::Get().GetDevice(), m_Fence, nullptr);
			vkFreeCommandBuffers(VKDevice::Get().GetDevice(), VKDevice::Get().GetVKContext()->GetCommandPool()->GetCommandPool(),1, &m_CommandBuffer);
		}

		void VKCommandBuffer::BeginRecording()
		{
			if (m_Primary)
			{
				VkCommandBufferBeginInfo beginCI{};
				beginCI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginCI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				VK_CHECK_RESULT(vkBeginCommandBuffer(m_CommandBuffer, &beginCI));
			}
			else
				Debug::Log::Critical("BeginRecording() called from a secondary command buffer!");
		}

		void VKCommandBuffer::BeginRecordingSecondary(RenderPass* renderPass, Framebuffer* framebuffer)
		{
			if (!m_Primary)
			{
				VkCommandBufferInheritanceInfo inheritanceInfo{};
				inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
				inheritanceInfo.subpass = 0;
				inheritanceInfo.renderPass = static_cast<VKRenderpass*>(renderPass)->GetRenderpass();
				inheritanceInfo.framebuffer = static_cast<VKFramebuffer*>(framebuffer)->GetFramebuffer();

				VkCommandBufferBeginInfo beginCI{};
				beginCI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginCI.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
				beginCI.pInheritanceInfo = &inheritanceInfo;

				VK_CHECK_RESULT(vkBeginCommandBuffer(m_CommandBuffer, &beginCI));
			}
			else
				Debug::Log::Critical("BeginRecordingSecondary() called from a primary command buffer!");
		}

		void VKCommandBuffer::EndRecording()
		{
			VK_CHECK_RESULT(vkEndCommandBuffer(m_CommandBuffer));
		}

		void VKCommandBuffer::Execute(bool waitFence)
		{
			ExecuteInternal(VkPipelineStageFlags(), VK_NULL_HANDLE, VK_NULL_HANDLE, waitFence);
		}

		void VKCommandBuffer::ExecuteInternal(VkPipelineStageFlags flags, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, bool waitFence)
		{
			if (m_Primary)
			{
				uint32_t waitSemaphoreCount = 1, signalSemaphoreCount = 1;

				if (!waitSemaphore)
					waitSemaphoreCount = 0;

				if (!signalSemaphore)
					signalSemaphoreCount = 0;

				VkSubmitInfo submitInfo{};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.pNext = VK_NULL_HANDLE;
				submitInfo.waitSemaphoreCount = waitSemaphoreCount;
				submitInfo.pWaitSemaphores = &waitSemaphore;
				submitInfo.pWaitDstStageMask = &flags;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &m_CommandBuffer;
				submitInfo.signalSemaphoreCount = signalSemaphoreCount;
				submitInfo.pSignalSemaphores = &signalSemaphore;

				if (waitFence)
				{
                    VK_CHECK_RESULT(vkWaitForFences(VKDevice::Get().GetDevice(), 1, &m_Fence, VK_TRUE, UINT64_MAX));
                    VK_CHECK_RESULT(vkResetFences(VKDevice::Get().GetDevice(), 1, &m_Fence));
					VK_CHECK_RESULT(vkQueueSubmit(VKDevice::Get().GetGraphicsQueue(), 1, &submitInfo, m_Fence));
				}
				else 
				{
					VK_CHECK_RESULT(vkQueueSubmit(VKDevice::Get().GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE));
					VK_CHECK_RESULT(vkQueueWaitIdle(VKDevice::Get().GetGraphicsQueue()));
				}
					
			}
			else
				Debug::Log::Critical("Used Execute on secondary command buffer!");
		}

		void VKCommandBuffer::ExecuteSecondary(CommandBuffer* primaryCmdBuffer)
		{
			if (!m_Primary)
				vkCmdExecuteCommands(static_cast<VKCommandBuffer*>(primaryCmdBuffer)->GetCommandBuffer(), 1, &m_CommandBuffer);
			else
				Debug::Log::Critical("Used ExecuteSecondary on primary command buffer!");
		}

		void VKCommandBuffer::UpdateViewport(u32 width, u32 height)
		{
			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = static_cast<float>(width);
			viewport.height = static_cast<float>(height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			VkRect2D scissor = {};
			scissor.offset = { 0, 0 };
			scissor.extent = { width, height };

			vkCmdSetViewport(m_CommandBuffer, 0, 1, &viewport);
			vkCmdSetScissor(m_CommandBuffer, 0, 1, &scissor);
		}
        
        void VKCommandBuffer::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
        }
        
        CommandBuffer* VKCommandBuffer::CreateFuncVulkan()
        {
            return new VKCommandBuffer();
        }
	}
}
