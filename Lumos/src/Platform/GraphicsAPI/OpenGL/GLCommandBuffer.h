#pragma once
#include "JM.h"
#include "Graphics/API/CommandBuffer.h"

namespace jm
{
	namespace graphics
	{
		class GLCommandBuffer : public api::CommandBuffer
		{
		public:
			GLCommandBuffer();
			~GLCommandBuffer();

			bool Init(bool primary) override;
			void Unload() override;
			void BeginRecording() override;
			void BeginRecordingSecondary(api::RenderPass* renderPass, Framebuffer* framebuffer) override;
			void EndRecording() override;
			void Execute(bool waitFence) override {};
			void ExecuteSecondary(api::CommandBuffer* primaryCmdBuffer) override;

			void UpdateViewport(uint width, uint height) override {};

		private:
			bool primary;
		};
	}
}