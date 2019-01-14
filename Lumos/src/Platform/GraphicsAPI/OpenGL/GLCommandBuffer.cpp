#include "JM.h"
#include "GLCommandBuffer.h"

namespace jm
{
	namespace graphics
	{
		GLCommandBuffer::GLCommandBuffer()
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

		void GLCommandBuffer::BeginRecordingSecondary(api::RenderPass* renderPass, Framebuffer* framebuffer)
		{
		}

		void GLCommandBuffer::EndRecording()
		{
		}

		void GLCommandBuffer::ExecuteSecondary(api::CommandBuffer* primaryCmdBuffer)
		{
		}
	}
}