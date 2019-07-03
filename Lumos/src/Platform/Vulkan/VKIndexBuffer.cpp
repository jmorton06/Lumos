#include "LM.h"
#include "VKIndexBuffer.h"
#include "VKVertexBuffer.h"

namespace Lumos
{
	namespace Graphics
	{
		VKIndexBuffer::VKIndexBuffer(uint16* data, u32 count, BufferUsage bufferUsage) : VKBuffer(vk::BufferUsageFlagBits::eIndexBuffer, count * sizeof(uint16), data), m_Size(count * sizeof(uint16)), m_Count(count), m_Usage(bufferUsage)
		{
		}

		VKIndexBuffer::VKIndexBuffer(u32* data, u32 count, BufferUsage bufferUsage) : VKBuffer(vk::BufferUsageFlagBits::eIndexBuffer, count * sizeof(u32), data) , m_Size(count * sizeof(u32)), m_Count(count), m_Usage(bufferUsage)
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

		u32 VKIndexBuffer::GetCount() const
		{
			return m_Count;
		}

		u32 VKIndexBuffer::GetSize() const
		{
			return m_Size;
		}
	}
}
