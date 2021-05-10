#include "Precompiled.h"
#include "VKCommandBuffer.h"
#include "VKDevice.h"
#include "VKCommandPool.h"
#include "VKFramebuffer.h"
#include "VKTools.h"

#include <Tracy/TracyVulkan.hpp>

namespace Lumos
{
    namespace Graphics
    {
        VKCommandBuffer::VKCommandBuffer()
            : m_CommandBuffer(nullptr)
            , m_Primary(false)
            , m_State(CommandBufferState::Idle)
        {
        }

        VKCommandBuffer::VKCommandBuffer(VkCommandBuffer commandBuffer)
            : m_CommandBuffer(commandBuffer)
            , m_Primary(true)
            , m_State(CommandBufferState::Idle)
        {
        }

        VKCommandBuffer::~VKCommandBuffer()
        {
            Unload();
        }

        bool VKCommandBuffer::Init(bool primary)
        {
            LUMOS_PROFILE_FUNCTION();
            m_Primary = primary;

            VkCommandBufferAllocateInfo cmdBufferCI {};

            m_CommandPool = VKDevice::Get().GetCommandPool()->GetHandle();

            cmdBufferCI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmdBufferCI.commandPool = m_CommandPool;
            cmdBufferCI.commandBufferCount = 1;
            cmdBufferCI.level = primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;

            VK_CHECK_RESULT(vkAllocateCommandBuffers(VKDevice::Get().GetDevice(), &cmdBufferCI, &m_CommandBuffer));

            return true;
        }

        bool VKCommandBuffer::Init(bool primary, VkCommandPool commandPool)
        {
            LUMOS_PROFILE_FUNCTION();
            m_Primary = primary;

            VkCommandBufferAllocateInfo cmdBufferCI {};

            m_CommandPool = commandPool;

            cmdBufferCI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmdBufferCI.commandPool = m_CommandPool;
            cmdBufferCI.commandBufferCount = 1;
            cmdBufferCI.level = primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;

            VK_CHECK_RESULT(vkAllocateCommandBuffers(VKDevice::Get().GetDevice(), &cmdBufferCI, &m_CommandBuffer));
            return true;
        }

        void VKCommandBuffer::Unload()
        {
            LUMOS_PROFILE_FUNCTION();
            vkFreeCommandBuffers(VKDevice::Get().GetDevice(), m_CommandPool, 1, &m_CommandBuffer);
        }

        void VKCommandBuffer::BeginRecording()
        {
            LUMOS_PROFILE_FUNCTION();
            LUMOS_ASSERT(m_Primary, "BeginRecording() called from a secondary command buffer!");

            m_State = CommandBufferState::Recording;
            VkCommandBufferBeginInfo beginCI {};
            beginCI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginCI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            VK_CHECK_RESULT(vkBeginCommandBuffer(m_CommandBuffer, &beginCI));
        }

        void VKCommandBuffer::BeginRecordingSecondary(RenderPass* renderPass, Framebuffer* framebuffer)
        {
            LUMOS_PROFILE_FUNCTION();
            LUMOS_ASSERT(!m_Primary, "BeginRecordingSecondary() called from a primary command buffer!");
            m_State = CommandBufferState::Recording;

            VkCommandBufferInheritanceInfo inheritanceInfo {};
            inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
            inheritanceInfo.subpass = 0;
            inheritanceInfo.renderPass = static_cast<VKRenderpass*>(renderPass)->GetHandle();
            inheritanceInfo.framebuffer = static_cast<VKFramebuffer*>(framebuffer)->GetFramebuffer();

            VkCommandBufferBeginInfo beginCI {};
            beginCI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginCI.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
            beginCI.pInheritanceInfo = &inheritanceInfo;

            VK_CHECK_RESULT(vkBeginCommandBuffer(m_CommandBuffer, &beginCI));
        }

        void VKCommandBuffer::EndRecording()
        {
            LUMOS_PROFILE_FUNCTION();
            LUMOS_ASSERT(m_State == CommandBufferState::Recording, "CommandBuffer ended before started recording");
            VK_CHECK_RESULT(vkEndCommandBuffer(m_CommandBuffer));
            m_State = CommandBufferState::Ended;
        }

        void VKCommandBuffer::Execute(bool waitFence)
        {
            LUMOS_PROFILE_FUNCTION();
            LUMOS_ASSERT(m_State == CommandBufferState::Ended, "CommandBuffer executed before ended recording");

            ExecuteInternal(VkPipelineStageFlags(), VK_NULL_HANDLE, VK_NULL_HANDLE, waitFence);
            m_State = CommandBufferState::Submitted;
        }

        void VKCommandBuffer::ExecuteInternal(VkPipelineStageFlags flags, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, bool waitFence)
        {
            LUMOS_PROFILE_FUNCTION();
            LUMOS_ASSERT(m_Primary, "Used Execute on secondary command buffer!");
            LUMOS_ASSERT(m_State == CommandBufferState::Ended, "CommandBuffer executed before ended recording");
            uint32_t waitSemaphoreCount = waitSemaphore ? 1 : 0, signalSemaphoreCount = signalSemaphore ? 1 : 0;

            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.pNext = VK_NULL_HANDLE;
            submitInfo.waitSemaphoreCount = waitSemaphoreCount;
            submitInfo.pWaitSemaphores = &waitSemaphore;
            submitInfo.pWaitDstStageMask = &flags;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &m_CommandBuffer;
            submitInfo.signalSemaphoreCount = signalSemaphoreCount;
            submitInfo.pSignalSemaphores = &signalSemaphore;

            VK_CHECK_RESULT(vkQueueSubmit(VKDevice::Get().GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE));
            m_State = CommandBufferState::Submitted;
        }

        void VKCommandBuffer::ExecuteSecondary(CommandBuffer* primaryCmdBuffer)
        {
            LUMOS_PROFILE_FUNCTION();
            LUMOS_ASSERT(!m_Primary, "Used ExecuteSecondary on primary command buffer!");
            m_State = CommandBufferState::Submitted;

            vkCmdExecuteCommands(static_cast<VKCommandBuffer*>(primaryCmdBuffer)->GetHandle(), 1, &m_CommandBuffer);
        }

        void VKCommandBuffer::UpdateViewport(uint32_t width, uint32_t height)
        {
            LUMOS_PROFILE_FUNCTION();
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

        void VKCommandBuffer::Reset()
        {
            LUMOS_PROFILE_FUNCTION();
            VK_CHECK_RESULT(vkResetCommandBuffer(m_CommandBuffer, 0));
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
