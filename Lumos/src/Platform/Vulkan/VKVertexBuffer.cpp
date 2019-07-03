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
		}

		void VKVertexBuffer::Resize(u32 size)
		{
			m_Size = size;
			VKTools::CreateBuffer(size, vk::BufferUsageFlagBits::eVertexBuffer,
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_Buffer, m_Memory);

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
			void* temp;
			VKDevice::Instance()->GetDevice().mapMemory(m_Memory, 0, m_Size, vk::MemoryMapFlagBits(), &temp);
			return temp;
		}

		void VKVertexBuffer::ReleasePointer()
		{
			vk::MappedMemoryRange memoryRange;
			memoryRange.memory = m_Memory;
			memoryRange.size = m_Size;
			VKDevice::Instance()->GetDevice().flushMappedMemoryRanges(1, &memoryRange);
			VKDevice::Instance()->GetDevice().unmapMemory(m_Memory);
		}

		void VKVertexBuffer::Bind()
		{
		}

		void VKVertexBuffer::Unbind()
		{
		}
	}
}
