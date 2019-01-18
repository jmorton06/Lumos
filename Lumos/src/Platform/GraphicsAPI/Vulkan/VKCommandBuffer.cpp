#include "LM.h"
#include "VKCommandBuffer.h"

#include "VKDevice.h"
#include "VKCommandPool.h"
#include "VKFramebuffer.h"
#include "VKTools.h"

namespace Lumos
{
	namespace graphics
	{

		VKCommandBuffer::VKCommandBuffer()
		{
			m_CommandBuffer = VK_NULL_HANDLE;
		}

		VKCommandBuffer::~VKCommandBuffer()
		{
			Unload();
			m_CommandBuffer = VK_NULL_HANDLE;
		}

		bool VKCommandBuffer::Init(bool primary)
		{
			VkResult result;
			VkCommandBufferAllocateInfo cmdBufferCI{};

			m_Primary = primary;

			cmdBufferCI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmdBufferCI.commandPool = VKDevice::Instance()->GetVKContext()->GetCommandPool()->GetCommandPool();
			cmdBufferCI.commandBufferCount = 1;

			if (primary)
				cmdBufferCI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			else
				cmdBufferCI.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;

			result = vkAllocateCommandBuffers(VKDevice::Instance()->GetDevice(), &cmdBufferCI, &m_CommandBuffer);
			if (result != VK_SUCCESS)
				return false;

			VkFenceCreateInfo fenceCI{};
			fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

			vkCreateFence(VKDevice::Instance()->GetDevice(), &fenceCI, VK_NULL_HANDLE, &m_Fence);

			return true;
		}

		void VKCommandBuffer::Unload()
		{
			vkDestroyFence(VKDevice::Instance()->GetDevice(), m_Fence, VK_NULL_HANDLE);
			vkFreeCommandBuffers(VKDevice::Instance()->GetDevice(), VKDevice::Instance()->GetVKContext()->GetCommandPool()->GetCommandPool(), 1, &m_CommandBuffer);
		}

		void VKCommandBuffer::BeginRecording()
		{
			if (m_Primary)
			{
				VkCommandBufferBeginInfo beginCI{};
				beginCI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginCI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

				vkBeginCommandBuffer(m_CommandBuffer, &beginCI);
			}
			else
				LUMOS_CORE_ERROR("WARNING: BeginRecording() called from a secondary command buffer!");
		}

		void VKCommandBuffer::BeginRecordingSecondary(api::RenderPass* renderPass, Framebuffer* framebuffer)
		{
			if (!m_Primary)
			{
				VkCommandBufferInheritanceInfo inheritanceInfo{};
				inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
				inheritanceInfo.subpass = 0;
				inheritanceInfo.renderPass = ((VKRenderpass*)renderPass)->GetRenderpass();
				inheritanceInfo.framebuffer = ((VKFramebuffer*)framebuffer)->GetFramebuffer();

				VkCommandBufferBeginInfo beginCI{};
				beginCI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginCI.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
				beginCI.pInheritanceInfo = &inheritanceInfo;

				vkBeginCommandBuffer(m_CommandBuffer, &beginCI);
			}
			else
				LUMOS_CORE_ERROR("WARNING: BeginRecordingSecondary() called from a primary command buffer!");
		}

		void VKCommandBuffer::EndRecording()
		{
			vkEndCommandBuffer(m_CommandBuffer);
		}

		void VKCommandBuffer::Execute(bool waitFence)
		{
			ExecuteInternal(VkPipelineStageFlags(), NULL, NULL, waitFence);
		}

		void VKCommandBuffer::ExecuteInternal(VkPipelineStageFlags flags, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, bool waitFence)
		{
			if (m_Primary)
			{
				uint32_t waitSemaphoreCount = 1, signalSemaphoreCount = 1;

				if (waitSemaphore == NULL)
					waitSemaphoreCount = 0;

				if (signalSemaphore == NULL)
					signalSemaphoreCount = 0;

				VkSubmitInfo submitInfo{};
				submitInfo.pNext = VK_NULL_HANDLE;
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.waitSemaphoreCount = waitSemaphoreCount;
				submitInfo.pWaitSemaphores = &waitSemaphore;
				submitInfo.pWaitDstStageMask = &flags;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &m_CommandBuffer;
				submitInfo.signalSemaphoreCount = signalSemaphoreCount;
				submitInfo.pSignalSemaphores = &signalSemaphore;

				if (waitFence)
				{
					VK_CHECK_RESULT(vkQueueSubmit(VKDevice::Instance()->GetGraphicsQueue(), 1, &submitInfo, m_Fence));

					vkWaitForFences(VKDevice::Instance()->GetDevice(), 1, &m_Fence, VK_TRUE, UINT64_MAX);
					vkResetFences(VKDevice::Instance()->GetDevice(), 1, &m_Fence);
				}
				else
					VK_CHECK_RESULT(vkQueueSubmit(VKDevice::Instance()->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE));
			}
			else
				LUMOS_CORE_ERROR("WARNING: Used Execute on secondary command buffer!");
		}

		void VKCommandBuffer::ExecuteSecondary(CommandBuffer* primaryCmdBuffer)
		{
			if (!m_Primary)
				vkCmdExecuteCommands(((VKCommandBuffer*)primaryCmdBuffer)->GetCommandBuffer(), 1, &m_CommandBuffer);
			else
				LUMOS_CORE_ERROR("WARNING: Used ExecuteSecondary on primary command buffer!");
		}

		void VKCommandBuffer::UpdateViewport(uint width, uint height)
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
	}
}
