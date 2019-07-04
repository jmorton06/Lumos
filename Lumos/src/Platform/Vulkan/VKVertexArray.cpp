#include "LM.h"
#include "VKVertexArray.h"
#include "VKVertexBuffer.h"

namespace Lumos
{
	namespace Graphics
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

		VertexBuffer* VKVertexArray::GetBuffer(u32 index)
		{
			return m_Buffers[index];
		}

		void VKVertexArray::PushBuffer(VertexBuffer* buffer)
		{
			m_Buffers.emplace_back(buffer);
			m_VKBuffers.emplace_back(dynamic_cast<VKVertexBuffer*>(buffer)->GetBuffer());
			m_Offsets.emplace_back(dynamic_cast<VKVertexBuffer*>(buffer)->GetLayout().GetStride());
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

		void VKVertexArray::Draw(u32 count) const
		{
		}
	}
	
}