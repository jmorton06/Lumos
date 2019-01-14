#include "JM.h"
#include "GLVertexArray.h"

#include "GL.h"
#include "GLDebug.h"

namespace jm
{

	GLVertexArray::GLVertexArray()
	{
		GLCall(glGenVertexArrays(1, &m_Handle));
		GLCall(glBindVertexArray(m_Handle));
	}

	GLVertexArray::~GLVertexArray()
	{
		for (auto buffer : m_Buffers)
		{
			delete buffer;
		}
	}

	VertexBuffer* GLVertexArray::GetBuffer(uint index)
	{
		return m_Buffers[index];
	}

	void GLVertexArray::PushBuffer(VertexBuffer* buffer)
	{
		m_Buffers.push_back(buffer);
	}

	void GLVertexArray::Bind() const
	{
		GLCall(glBindVertexArray(m_Handle));
		if (!m_Buffers.empty())
			m_Buffers.front()->Bind();
	}

	void GLVertexArray::Unbind() const
	{
#ifdef JM_DEBUG
		if (!m_Buffers.empty())
			m_Buffers.front()->Unbind();
		GLCall(glBindVertexArray(0));
#endif
	}

	void GLVertexArray::Draw(uint count) const
	{
		GLCall(glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, NULL));
	}
}
