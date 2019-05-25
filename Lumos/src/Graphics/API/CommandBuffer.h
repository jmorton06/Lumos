#pragma once
#include "LM.h"

namespace lumos
{
	namespace graphics
	{
		class RenderPass;
		class Framebuffer;

		enum PipelineStageFlags
		{
				
		};

		class Semaphore
		{
				
		};

		class LUMOS_EXPORT CommandBuffer
		{
		public:

			virtual ~CommandBuffer() = default;

			static CommandBuffer* Create();

			virtual bool Init(bool primary) = 0;
			virtual void Unload() = 0;
			virtual void BeginRecording() = 0;
			virtual void BeginRecordingSecondary(RenderPass* renderPass, Framebuffer* framebuffer) = 0;
			virtual void EndRecording() = 0;
			virtual void Execute(bool waitFence) = 0;
			virtual void ExecuteSecondary(CommandBuffer* primaryCmdBuffer) = 0;
			virtual void UpdateViewport(uint width, uint height) = 0;
		};
	}
}
