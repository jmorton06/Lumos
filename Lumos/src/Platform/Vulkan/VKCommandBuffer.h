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
			void ExecuteInternal(vk::PipelineStageFlags flags, vk::Semaphore waitSemaphore, vk::Semaphore signalSemaphore, bool waitFence);

			void ExecuteSecondary(CommandBuffer* primaryCmdBuffer) override;
            void UpdateViewport(u32 width, u32 height) override;

			vk::CommandBuffer GetCommandBuffer() const { return m_CommandBuffer[0]; };
			vk::Fence GetFence() const { return m_Fence; };

		private:
			std::vector<vk::CommandBuffer> m_CommandBuffer;
			vk::Fence m_Fence;
			bool m_Primary;
		};
	}
}
