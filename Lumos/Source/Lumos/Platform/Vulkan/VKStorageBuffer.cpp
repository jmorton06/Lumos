#include "Precompiled.h"
#include "VKStorageBuffer.h"
#include "VKDevice.h"

namespace Lumos
{
    namespace Graphics
    {
        VKStorageBuffer::VKStorageBuffer(uint32_t size, const void* data, bool readback)
        {
            // cached memory when the CPU reads it back (write-combined reads crawl)
            VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            if(readback)
                flags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
            VKBuffer::Init(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, flags, size, data);
        }

        VKStorageBuffer::VKStorageBuffer()
        {
        }

        VKStorageBuffer::~VKStorageBuffer()
        {
        }

        void VKStorageBuffer::SetData(uint32_t size, const void* data)
        {
            VKBuffer::Map();
            memcpy(m_Mapped, data, static_cast<size_t>(size));
            VKBuffer::UnMap();
        }

        void VKStorageBuffer::Resize(uint32_t size, const void* data)
        {
            VKBuffer::Resize(size, data);
        }

        void VKStorageBuffer::Unmap()
        {
            VKBuffer::UnMap();
        }

        void* VKStorageBuffer::GetPointerInternal()
        {
            VKBuffer::Map();
            return m_Mapped;
        }

        void VKStorageBuffer::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
        }

        StorageBuffer* VKStorageBuffer::CreateFuncVulkan(uint32_t size, const void* data, bool readback)
        {
            return new VKStorageBuffer(size, data, readback);
        }
    }
}
