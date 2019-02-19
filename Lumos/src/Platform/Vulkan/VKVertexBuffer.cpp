#include "LM.h"
#include "VKDevice.h"
#include "VKVertexBuffer.h"

namespace Lumos
{ 
	namespace graphics
	{
		static uint BufferUsageToOpenVK(const BufferUsage usage)
		{
			switch (usage)
			{
			case BufferUsage::STATIC:  return 0;
			case BufferUsage::DYNAMIC: return 0;
			case BufferUsage::STREAM:  return 0;
			}
			return 0;
		}

		VKVertexBuffer::VKVertexBuffer(BufferUsage usage)
			: VKBuffer(), m_Usage(usage), m_Size(0)
		{
		}

		VKVertexBuffer::~VKVertexBuffer()
		{
		}

		void VKVertexBuffer::Resize(uint size)
		{
			m_Size = size;

		}

		void VKVertexBuffer::SetLayout(const graphics::BufferLayout& bufferLayout)
		{
			m_Layout = bufferLayout;
		}

		void VKVertexBuffer::SetData(uint size, const void* data)
		{
			VKBuffer::Init(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, size, data);
		}


		void VKVertexBuffer::SetDataSub(uint size, const void* data, uint offset)
		{
			VKBuffer::Init(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, size, data);
		}


		void* VKVertexBuffer::GetPointerInternal()
		{
			return nullptr;
		}

		void VKVertexBuffer::ReleasePointer()
		{
		}

		void VKVertexBuffer::Bind()
		{
		}

		void VKVertexBuffer::Unbind()
		{
		}
	}
}
