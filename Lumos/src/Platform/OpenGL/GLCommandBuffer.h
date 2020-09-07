#pragma once

#include "Graphics/API/CommandBuffer.h"

namespace Lumos
{
	namespace Graphics
	{
		class GLCommandBuffer : public CommandBuffer
		{
		public:
			GLCommandBuffer();
			~GLCommandBuffer();

			bool Init(bool primary) override;
			void Unload() override;
			void BeginRecording() override;
			void BeginRecordingSecondary(RenderPass* renderPass, Framebuffer* framebuffer) override;
			void EndRecording() override;
			void Execute(bool waitFence) override {};
			void ExecuteSecondary(CommandBuffer* primaryCmdBuffer) override;

			void UpdateViewport(u32 width, u32 height) override {};
            static void MakeDefault();
        protected:
            static CommandBuffer* CreateFuncGL();
		private:
			bool primary;
		};
	}
}
