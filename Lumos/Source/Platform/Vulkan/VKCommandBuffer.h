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
            void UpdateViewport(u32 width, u32 height) override;

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
