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
		}

		VKCommandBuffer::~VKCommandBuffer()
		{
			Unload();
		}

		bool VKCommandBuffer::Init(bool primary)
		{
			vk::CommandBufferAllocateInfo cmdBufferCI{};

			m_Primary = primary;

			cmdBufferCI.commandPool = VKDevice::Instance()->GetVKContext()->GetCommandPool()->GetCommandPool();
			cmdBufferCI.commandBufferCount = 1;

			if (primary)
				cmdBufferCI.level = vk::CommandBufferLevel::ePrimary;
			else
				cmdBufferCI.level = vk::CommandBufferLevel::eSecondary;

			m_CommandBuffer = VKDevice::Instance()->GetDevice().allocateCommandBuffers(cmdBufferCI);

			if (m_CommandBuffer.empty())
				return false;

			vk::FenceCreateInfo fenceCI{};
			fenceCI.flags = vk::FenceCreateFlagBits::eSignaled;
			m_Fence = VKDevice::Instance()->GetDevice().createFence(fenceCI);

			return true;
		}

		void VKCommandBuffer::Unload()
		{
			VKDevice::Instance()->GetDevice().destroyFence(m_Fence);
			VKDevice::Instance()->GetDevice().freeCommandBuffers(VKDevice::Instance()->GetVKContext()->GetCommandPool()->GetCommandPool(), m_CommandBuffer);
		}

		void VKCommandBuffer::BeginRecording()
		{
			if (m_Primary)
			{
				vk::CommandBufferBeginInfo beginCI{};
				beginCI.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
				m_CommandBuffer[0].begin(beginCI);
			}
			else
				LUMOS_CORE_ERROR("WARNING: BeginRecording() called from a secondary command buffer!");
		}

		void VKCommandBuffer::BeginRecordingSecondary(api::RenderPass* renderPass, Framebuffer* framebuffer)
		{
			if (!m_Primary)
			{
				vk::CommandBufferInheritanceInfo inheritanceInfo{};
				inheritanceInfo.subpass = 0;
				inheritanceInfo.renderPass = ((VKRenderpass*)renderPass)->GetRenderpass();
				inheritanceInfo.framebuffer = ((VKFramebuffer*)framebuffer)->GetFramebuffer();

				vk::CommandBufferBeginInfo beginCI{};
				beginCI.flags = vk::CommandBufferUsageFlagBits::eRenderPassContinue;
				beginCI.pInheritanceInfo = &inheritanceInfo;

				m_CommandBuffer[0].begin(beginCI);
			}
			else
				LUMOS_CORE_ERROR("WARNING: BeginRecordingSecondary() called from a primary command buffer!");
		}

		void VKCommandBuffer::EndRecording()
		{
			m_CommandBuffer[0].end();
		}

		void VKCommandBuffer::Execute(bool waitFence)
		{
			ExecuteInternal(vk::PipelineStageFlags(), nullptr, nullptr, waitFence);
		}

		void VKCommandBuffer::ExecuteInternal(vk::PipelineStageFlags flags, vk::Semaphore waitSemaphore, vk::Semaphore signalSemaphore, bool waitFence)
		{
			if (m_Primary)
			{
				uint32_t waitSemaphoreCount = 1, signalSemaphoreCount = 1;

				if (!waitSemaphore)
					waitSemaphoreCount = 0;

				if (!signalSemaphore)
					signalSemaphoreCount = 0;

				vk::SubmitInfo submitInfo{};
				submitInfo.pNext = VK_NULL_HANDLE;
				submitInfo.waitSemaphoreCount = waitSemaphoreCount;
				submitInfo.pWaitSemaphores = &waitSemaphore;
				submitInfo.pWaitDstStageMask = &flags;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &m_CommandBuffer[0];
				submitInfo.signalSemaphoreCount = signalSemaphoreCount;
				submitInfo.pSignalSemaphores = &signalSemaphore;

				if (waitFence)
				{
					VKDevice::Instance()->GetGraphicsQueue().submit(submitInfo, m_Fence);
					VKDevice::Instance()->GetDevice().waitForFences(m_Fence, VK_TRUE, UINT64_MAX);
					VKDevice::Instance()->GetDevice().resetFences(m_Fence);
				}
				else
					VKDevice::Instance()->GetGraphicsQueue().submit(submitInfo, nullptr);
			}
			else
				LUMOS_CORE_ERROR("WARNING: Used Execute on secondary command buffer!");
		}

		void VKCommandBuffer::ExecuteSecondary(CommandBuffer* primaryCmdBuffer)
		{
			if (!m_Primary)
				((VKCommandBuffer*)primaryCmdBuffer)->GetCommandBuffer().executeCommands(m_CommandBuffer[0]);
			else
				LUMOS_CORE_ERROR("WARNING: Used ExecuteSecondary on primary command buffer!");
		}

		void VKCommandBuffer::UpdateViewport(uint width, uint height)
		{
			vk::Viewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = static_cast<float>(width);
			viewport.height = static_cast<float>(height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

            vk::Rect2D scissor = vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(width,height));

			m_CommandBuffer[0].setViewport(0, 1, &viewport);
			m_CommandBuffer[0].setScissor(0, 1, &scissor);
		}
	}
}
