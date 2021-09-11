#include "Precompiled.h"
#include "GLCommandBuffer.h"
#include "GLPipeline.h"

namespace Lumos
{
    namespace Graphics
    {
        GLCommandBuffer::GLCommandBuffer()
            : primary(false)
            , m_BoundPipeline(nullptr)
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
    
        void GLCommandBuffer::BindPipeline(Pipeline* pipeline)
        {
            if(pipeline != m_BoundPipeline)
            {
                pipeline->Bind(this);
                m_BoundPipeline = pipeline;
            }
        }
    
        void GLCommandBuffer::UnBindPipeline()
        {
            if(m_BoundPipeline)
                m_BoundPipeline->End(this);
            m_BoundPipeline = nullptr;
        }

        CommandBuffer* GLCommandBuffer::CreateFuncGL()
        {
            return new GLCommandBuffer();
        }
    }
}
