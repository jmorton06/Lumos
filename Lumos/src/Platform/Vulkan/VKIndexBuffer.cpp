#include "LM.h"
#include "VKIndexBuffer.h"
#include "VKVertexBuffer.h"

namespace lumos
{
	namespace graphics
	{
		VKIndexBuffer::VKIndexBuffer(uint16* data, uint count, BufferUsage bufferUsage) : VKBuffer(vk::BufferUsageFlagBits::eIndexBuffer, count * sizeof(uint16), data), m_Size(count * sizeof(uint16)), m_Count(count), m_Usage(bufferUsage)
		{
		}

		VKIndexBuffer::VKIndexBuffer(uint* data, uint count, BufferUsage bufferUsage) : VKBuffer(vk::BufferUsageFlagBits::eIndexBuffer, count * sizeof(uint), data) , m_Size(count * sizeof(uint)), m_Count(count), m_Usage(bufferUsage)
		{
		}

		VKIndexBuffer::~VKIndexBuffer()
		{
			
		}

		void VKIndexBuffer::Bind() const 
		{
		}

		void VKIndexBuffer::Unbind() const
		{
		}

		uint VKIndexBuffer::GetCount() const
		{
			return m_Count;
		}

		uint VKIndexBuffer::GetSize() const
		{
			return m_Size;
		}
	}
}
