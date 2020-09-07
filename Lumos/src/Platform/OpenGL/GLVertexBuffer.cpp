#include "Precompiled.h"
#include "GLVertexBuffer.h"
#include "GLPipeline.h"

#include "GL.h"

namespace Lumos
{
	namespace Graphics
	{
		static u32 BufferUsageToOpenGL(const BufferUsage usage)
		{
			switch (usage)
			{
			case BufferUsage::STATIC:  return GL_STATIC_DRAW;
			case BufferUsage::DYNAMIC: return GL_DYNAMIC_DRAW;
			case BufferUsage::STREAM:  return GL_STREAM_DRAW;
			}
			return 0;
		}

		GLVertexBuffer::GLVertexBuffer(BufferUsage usage)
			: m_Usage(usage), m_Size(0)
		{
			GLCall(glGenBuffers(1, &m_Handle));
		}

		GLVertexBuffer::~GLVertexBuffer()
		{
			GLCall(glDeleteBuffers(1, &m_Handle));
		}

		void GLVertexBuffer::Resize(u32 size)
		{
			m_Size = size;

			GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_Handle));
			GLCall(glBufferData(GL_ARRAY_BUFFER, size, NULL, BufferUsageToOpenGL(m_Usage)));
		}

		void GLVertexBuffer::SetData(u32 size, const void* data)
		{
			GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_Handle));
			GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, BufferUsageToOpenGL(m_Usage)));
		}

		void GLVertexBuffer::SetDataSub(u32 size, const void* data, u32 offset)
		{
			GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_Handle));
			GLCall(glBufferSubData(GL_ARRAY_BUFFER, offset, size, data));
		}


		void* GLVertexBuffer::GetPointerInternal()
        {
            void* result = nullptr;
            if(!m_Mapped)
            {
                GLCall(result = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
                m_Mapped = true;
            }
            else
            {
                Lumos::Debug::Log::Warning("Vertex buffer already mapped");
            }

			return result;
		}

		void GLVertexBuffer::ReleasePointer()
		{
            if(m_Mapped)
            {
                GLCall(glUnmapBuffer(GL_ARRAY_BUFFER));
                m_Mapped = false;
            }
		}

		void GLVertexBuffer::Bind(CommandBuffer* commandBuffer, Pipeline* pipeline)
		{
			GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_Handle));
            ((GLPipeline*)pipeline)->BindVertexArray();
		}

		void GLVertexBuffer::Unbind()
		{
			GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
		}

		void GLVertexBuffer::MakeDefault()
		{
			CreateFunc = CreateFuncGL;
		}

		VertexBuffer* GLVertexBuffer::CreateFuncGL(const BufferUsage& usage)
		{
			return new GLVertexBuffer(usage);
		}
	}
}
