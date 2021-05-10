#include "Precompiled.h"
#include "VKIndexBuffer.h"
#include "VKVertexBuffer.h"
#include "VKCommandBuffer.h"

namespace Lumos
{
    namespace Graphics
    {
        VKIndexBuffer::VKIndexBuffer(uint16_t* data, uint32_t count, BufferUsage bufferUsage)
            : VKBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, count * sizeof(uint16_t), data)
            , m_Size(count * sizeof(uint16_t))
            , m_Count(count)
            , m_Usage(bufferUsage)
        {
        }

        VKIndexBuffer::VKIndexBuffer(uint32_t* data, uint32_t count, BufferUsage bufferUsage)
            : VKBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, count * sizeof(uint32_t), data)
            , m_Size(count * sizeof(uint32_t))
            , m_Count(count)
            , m_Usage(bufferUsage)
        {
        }

        VKIndexBuffer::~VKIndexBuffer()
        {
            if(m_MappedBuffer)
            {
                VKBuffer::Flush(m_Size);
                VKBuffer::UnMap();
                m_MappedBuffer = false;
            }
        }

        void VKIndexBuffer::Bind(CommandBuffer* commandBuffer) const
        {
            vkCmdBindIndexBuffer(static_cast<VKCommandBuffer*>(commandBuffer)->GetHandle(), m_Buffer, 0, VK_INDEX_TYPE_UINT32);
        }

        void VKIndexBuffer::Unbind() const
        {
        }

        uint32_t VKIndexBuffer::GetCount() const
        {
            return m_Count;
        }

        uint32_t VKIndexBuffer::GetSize() const
        {
            return m_Size;
        }

        void* VKIndexBuffer::GetPointerInternal()
        {
            LUMOS_PROFILE_FUNCTION();
            if(!m_MappedBuffer)
            {
                VKBuffer::Map();
                m_MappedBuffer = true;
            }

            return m_Mapped;
        }

        void VKIndexBuffer::ReleasePointer()
        {
            LUMOS_PROFILE_FUNCTION();
            if(m_MappedBuffer)
            {
                VKBuffer::Flush(m_Size);
                VKBuffer::UnMap();
                m_MappedBuffer = false;
            }
        }

        void VKIndexBuffer::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
            Create16Func = CreateFunc16Vulkan;
        }

        IndexBuffer* VKIndexBuffer::CreateFuncVulkan(uint32_t* data, uint32_t count, BufferUsage bufferUsage)
        {
            return new VKIndexBuffer(data, count, bufferUsage);
        }

        IndexBuffer* VKIndexBuffer::CreateFunc16Vulkan(uint16_t* data, uint32_t count, BufferUsage bufferUsage)
        {
            return new VKIndexBuffer(data, count, bufferUsage);
        }
    }
}
