#include "LM.h"
#include "GLVertexBuffer.h"

#include "GL.h"

namespace Lumos
{
	static uint BufferUsageToOpenGL(const BufferUsage usage)
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

	void GLVertexBuffer::Resize(uint size)
	{
		m_Size = size;

		GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_Handle));
		GLCall(glBufferData(GL_ARRAY_BUFFER, size, NULL, BufferUsageToOpenGL(m_Usage)));
	}

	void GLVertexBuffer::SetLayout(const graphics::BufferLayout& bufferLayout)
	{
		m_Layout = bufferLayout;
		const std::vector<graphics::BufferElement>& layout = bufferLayout.GetLayout();
		for (uint i = 0; i < layout.size(); i++)
		{
			const graphics::BufferElement& element = layout[i];
			GLCall(glEnableVertexAttribArray(i));
			size_t offset = static_cast<size_t>(element.offset);
			GLCall(glVertexAttribPointer(i, element.count, element.type, element.normalized, bufferLayout.GetStride(), reinterpret_cast<const void*>(offset)));
		}
	}

	void GLVertexBuffer::SetData(uint size, const void* data)
	{
		GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_Handle));
		GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, BufferUsageToOpenGL(m_Usage)));
	}


	void GLVertexBuffer::SetDataSub(uint size, const void* data, uint offset)
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
#ifdef LUMOS_DEBUG
		GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
#endif
	}
}
