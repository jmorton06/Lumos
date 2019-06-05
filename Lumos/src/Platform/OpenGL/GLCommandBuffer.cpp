#include "LM.h"
#include "GLCommandBuffer.h"

namespace Lumos
{
	namespace Graphics
	{
		GLCommandBuffer::GLCommandBuffer(): primary(false)
		{
		}

		GLCommandBuffer::~GLCommandBuffer()
		{
		}

		bool GLCommandBuffer::Init(bool primary)
		{
			return true;
		}

		void GLCommandBuffer::Unload()
		{
		}

		void GLCommandBuffer::BeginRecording()
		{
		}

		void GLCommandBuffer::BeginRecordingSecondary(RenderPass* renderPass, Framebuffer* framebuffer)
		{
		}

		void GLCommandBuffer::EndRecording()
		{
		}

		void GLCommandBuffer::ExecuteSecondary(CommandBuffer* primaryCmdBuffer)
		{
		}
	}
}