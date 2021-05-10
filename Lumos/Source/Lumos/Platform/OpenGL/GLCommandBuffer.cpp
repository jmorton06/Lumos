#include "Precompiled.h"
#include "GLCommandBuffer.h"

namespace Lumos
{
    namespace Graphics
    {
        GLCommandBuffer::GLCommandBuffer()
            : primary(false)
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

        void GLCommandBuffer::MakeDefault()
        {
            CreateFunc = CreateFuncGL;
        }

        CommandBuffer* GLCommandBuffer::CreateFuncGL()
        {
            return new GLCommandBuffer();
        }
    }
}