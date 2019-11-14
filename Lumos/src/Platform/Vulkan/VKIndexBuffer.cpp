#include "lmpch.h"
#include "VKIndexBuffer.h"
#include "VKVertexBuffer.h"
#include "VKCommandBuffer.h"

namespace Lumos
{
	namespace Graphics
	{
		VKIndexBuffer::VKIndexBuffer(u16* data, u32 count, BufferUsage bufferUsage) : VKBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, count * sizeof(u16), data), m_Size(count * sizeof(u16)), m_Count(count), m_Usage(bufferUsage)
		{
		}

		VKIndexBuffer::VKIndexBuffer(u32* data, u32 count, BufferUsage bufferUsage) : VKBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, count * sizeof(u32), data) , m_Size(count * sizeof(u32)), m_Count(count), m_Usage(bufferUsage)
		{
		}

		VKIndexBuffer::~VKIndexBuffer()
		{
		}

		void VKIndexBuffer::Bind(CommandBuffer* commandBuffer) const
		{
			vkCmdBindIndexBuffer(static_cast<VKCommandBuffer*>(commandBuffer)->GetCommandBuffer(), m_Buffer, 0,VK_INDEX_TYPE_UINT32);
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
        
        void VKIndexBuffer::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
			Create16Func = CreateFunc16Vulkan;
        }
        
		IndexBuffer* VKIndexBuffer::CreateFuncVulkan(u32* data, u32 count, BufferUsage bufferUsage)
        {
            return lmnew VKIndexBuffer(data, count, bufferUsage);
        }

		IndexBuffer* VKIndexBuffer::CreateFunc16Vulkan(u16* data, u32 count, BufferUsage bufferUsage)
		{
			return lmnew VKIndexBuffer(data, count, bufferUsage);
		}
	}
}
