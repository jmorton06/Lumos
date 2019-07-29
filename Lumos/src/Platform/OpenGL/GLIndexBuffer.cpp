#include "LM.h"
#include "GLIndexBuffer.h"

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

		GLIndexBuffer::GLIndexBuffer(u16* data, u32 count, BufferUsage bufferUsage)
			: m_Count(count), m_Usage(bufferUsage)
		{
			GLCall(glGenBuffers(1, &m_Handle));
			GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Handle));
			GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(u16), data, BufferUsageToOpenGL(m_Usage)));
		}

		GLIndexBuffer::GLIndexBuffer(u32* data, u32 count, BufferUsage bufferUsage)
			: m_Count(count), m_Usage(bufferUsage)
		{
			GLCall(glGenBuffers(1, &m_Handle));
			GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Handle));
			GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(u32), data, BufferUsageToOpenGL(m_Usage)));
		}

		GLIndexBuffer::~GLIndexBuffer()
		{
			GLCall(glDeleteBuffers(1, &m_Handle));
		}

		void GLIndexBuffer::Bind(CommandBuffer* commandBuffer) const
		{
			GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Handle));
		}

		void GLIndexBuffer::Unbind() const
		{
			GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
		}

		u32 GLIndexBuffer::GetCount() const
		{
			return m_Count;
		}
	}
}