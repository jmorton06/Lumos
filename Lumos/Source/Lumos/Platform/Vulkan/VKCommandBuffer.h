#pragma once
#include "VK.h"
#include "Graphics/API/CommandBuffer.h"

namespace Lumos
{
    namespace Graphics
    {
        enum class CommandBufferState : uint8_t
        {
            Idle,
            Recording,
            Ended,
            Submitted
        };

        class VKCommandBuffer : public CommandBuffer
        {
        public:
            VKCommandBuffer();
            VKCommandBuffer(VkCommandBuffer commandBuffer);
            ~VKCommandBuffer();

            bool Init(bool primary) override;
            bool Init(bool primary, VkCommandPool commandPool);
            void Unload() override;
            void BeginRecording() override;
            void BeginRecordingSecondary(RenderPass* renderPass, Framebuffer* framebuffer) override;
            void EndRecording() override;
            void Reset();

            void Execute(bool waitFence) override;
            void ExecuteInternal(VkPipelineStageFlags flags, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, bool waitFence);

            void ExecuteSecondary(CommandBuffer* primaryCmdBuffer) override;
            void UpdateViewport(uint32_t width, uint32_t height) override;

            VkCommandBuffer GetHandle() const { return m_CommandBuffer; };
            CommandBufferState GetState() const { return m_State; }

            static void MakeDefault();

        protected:
            static CommandBuffer* CreateFuncVulkan();

        private:
            VkCommandBuffer m_CommandBuffer;
            VkCommandPool m_CommandPool;
            bool m_Primary;
            CommandBufferState m_State;
        };
    }
}
