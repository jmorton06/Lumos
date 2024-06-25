#include "Precompiled.h"
#include "VKDevice.h"
#include "VKVertexBuffer.h"
#include "VKRenderer.h"
#include "VKPipeline.h"

namespace Lumos
{
    namespace Graphics
    {
        VKVertexBuffer::VKVertexBuffer(const BufferUsage& usage)
            : VKBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, usage == BufferUsage::DYNAMIC ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : 0, 0, nullptr)
            , m_Usage(usage)
            , m_Size(0)
        {
        }

        VKVertexBuffer::VKVertexBuffer(uint32_t size, const void* data, const BufferUsage& usage)
            : VKBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, usage == BufferUsage::DYNAMIC ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : 0, size, data)
            , m_Usage(usage)
            , m_Size(size)
        {
        }

        VKVertexBuffer::~VKVertexBuffer()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            if(m_MappedBuffer)
            {
                VKBuffer::Flush(m_Size);
                VKBuffer::UnMap();
                m_MappedBuffer = false;
            }
        }

        void VKVertexBuffer::Resize(uint32_t size)
        {
            LUMOS_PROFILE_FUNCTION_LOW();

            if(m_Size != size)
            {
                m_Size = size;
                VKBuffer::Resize(size, nullptr);
            }
        }

        void VKVertexBuffer::SetData(uint32_t size, const void* data, bool addBarrier)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            if(m_Size < size)
            {
                m_Size = size;
                VKBuffer::Resize(size, data);
            }
            else
            {
                VKBuffer::SetData(size, data, addBarrier);
            }
        }

        void VKVertexBuffer::SetDataSub(uint32_t size, const void* data, uint32_t offset)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            m_Size = size;

            if(m_Size < size)
            {
                m_Size = size;
                VKBuffer::Resize(size, data);
            }
            else
            {
                VKBuffer::SetData(size, data);
            }
        }

        void* VKVertexBuffer::GetPointerInternal()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            if(!m_MappedBuffer)
            {
                VKBuffer::Map();
                m_MappedBuffer = true;
            }

            return m_Mapped;
        }

        void VKVertexBuffer::ReleasePointer()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            if(m_MappedBuffer)
            {
                VKBuffer::Flush(m_Size);
                VKBuffer::UnMap();
                m_MappedBuffer = false;
            }
        }

        void VKVertexBuffer::Bind(CommandBuffer* commandBuffer, Pipeline* pipeline, uint8_t binding)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            VkDeviceSize offsets[1] = { 0 };
            if(commandBuffer)
                vkCmdBindVertexBuffers(static_cast<VKCommandBuffer*>(commandBuffer)->GetHandle(), binding, 1, &m_Buffer, offsets);
        }

        void VKVertexBuffer::Unbind()
        {
        }

        void VKVertexBuffer::MakeDefault()
        {
            CreateFunc         = CreateFuncVulkan;
            CreateWithDataFunc = CreateFuncWithDataVulkan;
        }

        VertexBuffer* VKVertexBuffer::CreateFuncVulkan(const BufferUsage& usage)
        {
            return new VKVertexBuffer(usage);
        }

        VertexBuffer* VKVertexBuffer::CreateFuncWithDataVulkan(uint32_t size, const void* data, const BufferUsage& usage)
        {
            return new VKVertexBuffer(size, data, usage);
        }
    }
}
