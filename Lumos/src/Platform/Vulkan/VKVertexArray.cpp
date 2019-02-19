#include "LM.h"
#include "VKVertexArray.h"

namespace Lumos
{
	namespace graphics
	{
		VKVertexArray::VKVertexArray()
		{
		}

		VKVertexArray::~VKVertexArray()
		{
			for (auto buffer : m_Buffers)
			{
				delete buffer;
			}
		}

		VertexBuffer* VKVertexArray::GetBuffer(uint index)
		{
			return m_Buffers[index];
		}

		void VKVertexArray::PushBuffer(VertexBuffer* buffer)
		{
			m_Buffers.push_back(buffer);
		}

		void VKVertexArray::Bind() const
		{
			if (!m_Buffers.empty())
				m_Buffers.front()->Bind();
		}

		void VKVertexArray::Unbind() const
		{
#ifdef LUMOS_DEBUG
			if (!m_Buffers.empty())
				m_Buffers.front()->Unbind();
#endif
		}

		void VKVertexArray::Draw(uint count) const
		{
		}
	}
	
}