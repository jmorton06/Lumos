#pragma once
#include "VK.h"
#include "Graphics/RHI/CommandBuffer.h"
#include "VKFence.h"

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

        class RenderPass;
        class Pipeline;

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
            bool Flush() override;
            bool Wait();

            void Submit() override;

            void BindPipeline(Pipeline* pipeline) override;
            void UnBindPipeline() override;

            void Execute(VkPipelineStageFlags flags, VkSemaphore signalSemaphore, bool waitFence);

            void ExecuteSecondary(CommandBuffer* primaryCmdBuffer) override;
            void UpdateViewport(uint32_t width, uint32_t height, bool flipViewport) override;

            VkCommandBuffer GetHandle() const { return m_CommandBuffer; };
            CommandBufferState GetState() const { return m_State; }

            VkSemaphore GetSemaphore() const { return m_Semaphore; }

            static void MakeDefault();

        protected:
            static CommandBuffer* CreateFuncVulkan();

        private:
            VkCommandBuffer m_CommandBuffer;
            VkCommandPool m_CommandPool;
            bool m_Primary;
            CommandBufferState m_State;
            SharedPtr<VKFence> m_Fence;
            VkSemaphore m_Semaphore;

            Pipeline* m_BoundPipeline = nullptr;
            RenderPass* m_BoundRenderPass = nullptr;
        };
    }
}
