#include "LM.h"
#include "GLVertexArray.h"
#include "GLVertexBuffer.h"

#include "GL.h"
#include "GLDebug.h"

namespace Lumos
{
	namespace Graphics
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

		VertexBuffer* GLVertexArray::GetBuffer(u32 index)
		{
			return m_Buffers[index];
		}

		void GLVertexArray::PushBuffer(VertexBuffer* buffer)
		{
			m_Buffers.push_back(buffer);
		}

		void GLVertexArray::Bind(CommandBuffer* commandBuffer) const
		{
			GLCall(glBindVertexArray(m_Handle));
			if (!m_Buffers.empty())
				m_Buffers.front()->Bind();
		}

		void GLVertexArray::Unbind() const
		{
			if (!m_Buffers.empty())
				m_Buffers.front()->Unbind();
			GLCall(glBindVertexArray(0));
		}

		void GLVertexArray::MakeDefault()
		{
			CreateFunc = CreateFuncGL;
		}

		VertexArray* GLVertexArray::CreateFuncGL()
		{
			return lmnew GLVertexArray();
		}
	}
}
