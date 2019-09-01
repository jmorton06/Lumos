#include "LM.h"
#include "VKIndexBuffer.h"
#include "VKVertexBuffer.h"
#include "VKCommandBuffer.h"

namespace Lumos
{
	namespace Graphics
	{
		VKIndexBuffer::VKIndexBuffer(u16* data, u32 count, BufferUsage bufferUsage) : VKBuffer(vk::BufferUsageFlagBits::eIndexBuffer, count * sizeof(u16), data), m_Size(count * sizeof(u16)), m_Count(count), m_Usage(bufferUsage)
		{
		}

		VKIndexBuffer::VKIndexBuffer(u32* data, u32 count, BufferUsage bufferUsage) : VKBuffer(vk::BufferUsageFlagBits::eIndexBuffer, count * sizeof(u32), data) , m_Size(count * sizeof(u32)), m_Count(count), m_Usage(bufferUsage)
		{
		}

		VKIndexBuffer::~VKIndexBuffer()
		{
			
		}

		void VKIndexBuffer::Bind(CommandBuffer* commandBuffer) const
		{
			static_cast<VKCommandBuffer*>(commandBuffer)->GetCommandBuffer().bindIndexBuffer(m_Buffer, 0, vk::IndexType::eUint32);
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
        
        void VKCommandBuffer::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
        }
        
        CommandBuffer* VKCommandBuffer::CreateFuncVulkan()
        {
            return lmnew VKCommandBuffer();
        }
	}
}
