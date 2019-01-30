#pragma once
#include "VK.h"
#include "Graphics/API/CommandBuffer.h"


namespace Lumos
{
	namespace graphics
	{
		class VKCommandBuffer : public api::CommandBuffer
		{
		public:
			VKCommandBuffer();
			~VKCommandBuffer();

			bool Init(bool primary) override;
			void Unload() override;
			void BeginRecording() override;
			void BeginRecordingSecondary(api::RenderPass* renderPass, Framebuffer* framebuffer) override;
			void EndRecording() override;

			void Execute(bool waitFence) override;
			void ExecuteInternal(VkPipelineStageFlags flags, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, bool waitFence);

			void ExecuteSecondary(api::CommandBuffer* primaryCmdBuffer) override;

			VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffer; };

			void UpdateViewport(uint width, uint height) override;

		private:
			VkFence 		m_Fence;
			VkCommandBuffer m_CommandBuffer;
			bool 			m_Primary;
		};
	}
}
