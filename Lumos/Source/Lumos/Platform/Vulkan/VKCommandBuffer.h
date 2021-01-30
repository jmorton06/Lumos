#pragma once
#include "VK.h"
#include "Graphics/API/CommandBuffer.h"

namespace Lumos
{
	namespace Graphics
	{
		class VKCommandBuffer : public CommandBuffer
		{
		public:
			VKCommandBuffer();
			~VKCommandBuffer();

			bool Init(bool primary) override;
			void Unload() override;
			void BeginRecording() override;
			void BeginRecordingSecondary(RenderPass* renderPass, Framebuffer* framebuffer) override;
			void EndRecording() override;

			void Execute(bool waitFence) override;
			void ExecuteInternal(VkPipelineStageFlags flags, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, bool waitFence);

			void ExecuteSecondary(CommandBuffer* primaryCmdBuffer) override;
            void UpdateViewport(uint32_t width, uint32_t height) override;

			VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffer; };
			VkFence GetFence() const { return m_Fence; };

            static void MakeDefault();
        protected:
            static CommandBuffer* CreateFuncVulkan();
		private:
			VkCommandBuffer m_CommandBuffer;
			VkFence m_Fence;
			bool m_Primary;
		};
	}
}
