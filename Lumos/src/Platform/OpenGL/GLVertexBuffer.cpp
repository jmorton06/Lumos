#include "LM.h"
#include "GLVertexBuffer.h"

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

		void GLVertexBuffer::SetLayout(const Graphics::BufferLayout& bufferLayout)
        {
			m_Layout = bufferLayout;
			const std::vector<Graphics::BufferElement>& layout = bufferLayout.GetLayout();
                        
			for (u32 i = 0; i < layout.size(); i++)
            {
				const Graphics::BufferElement& element = layout[i];
				GLCall(glEnableVertexAttribArray(i));
				size_t offset = static_cast<size_t>(element.offset);
				GLCall(glVertexAttribPointer(i, element.count, element.type, element.normalized, bufferLayout.GetStride(), reinterpret_cast<const void*>(offset)));
			}
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
			GLCall(void* result = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
			return result;
		}

		void GLVertexBuffer::ReleasePointer()
		{
			GLCall(glUnmapBuffer(GL_ARRAY_BUFFER));
			SetLayout(m_Layout);
		}

		void GLVertexBuffer::Bind()
		{
			GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_Handle));
			// SetLayout(m_Layout);
		}

		void GLVertexBuffer::Unbind()
		{
			GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
		}
	}
}
