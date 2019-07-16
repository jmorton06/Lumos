#include "LM.h"
#include "VKDevice.h"
#include "VKVertexBuffer.h"
#include "VKRenderer.h"

namespace Lumos
{ 
	namespace Graphics
	{
		VKVertexBuffer::VKVertexBuffer(BufferUsage usage)
			: VKBuffer(), m_Usage(usage), m_Size(0)
		{
		}

		VKVertexBuffer::~VKVertexBuffer()
		{
			if (m_MappedBuffer)
			{
				VKBuffer::Flush(m_Size);
				VKBuffer::UnMap();
				m_MappedBuffer = false;
			}
		}

		void VKVertexBuffer::Resize(u32 size)
		{
			m_Size = size;

			VKBuffer::Init(vk::BufferUsageFlagBits::eVertexBuffer, size, nullptr);
		}

		void VKVertexBuffer::SetLayout(const Graphics::BufferLayout& bufferLayout)
		{
			m_Layout = bufferLayout;
		}

		void VKVertexBuffer::SetData(u32 size, const void* data)
		{
			VKBuffer::Init(vk::BufferUsageFlagBits::eVertexBuffer, size, data);
		}


		void VKVertexBuffer::SetDataSub(u32 size, const void* data, u32 offset)
		{
			VKBuffer::Init(vk::BufferUsageFlagBits::eVertexBuffer, size, data);
		}


		void* VKVertexBuffer::GetPointerInternal()
		{
			if (!m_MappedBuffer)
			{
				VKBuffer::Map();
				m_MappedBuffer = true;
			}
			
			return m_Mapped;
		}

		void VKVertexBuffer::ReleasePointer()
		{
			if (m_MappedBuffer)
			{
				VKBuffer::Flush(m_Size);
				VKBuffer::UnMap();
				m_MappedBuffer = false;
			}
		}

		void VKVertexBuffer::Bind()
		{
		}

		void VKVertexBuffer::Unbind()
		{
		}
	}
}
