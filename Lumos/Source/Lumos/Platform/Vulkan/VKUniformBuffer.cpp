#include "Precompiled.h"
#include "VKUniformBuffer.h"
#include "VKDevice.h"
#include "VKTools.h"

namespace Lumos
{
    namespace Graphics
    {
        VKUniformBuffer::VKUniformBuffer(uint32_t size, const void* data)
        {
            VKBuffer::Init(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, size, data);
        }

        VKUniformBuffer::VKUniformBuffer()
        {
#ifdef USE_VMA_ALLOCATOR
            m_Allocation = nullptr;
#endif
        }

        VKUniformBuffer::~VKUniformBuffer()
        {
        }

        void VKUniformBuffer::Init(uint32_t size, const void* data)
        {
            VKBuffer::Init(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, size, data);
        }

        void VKUniformBuffer::SetData(uint32_t size, const void* data)
        {
            VKBuffer::Map();
            memcpy(m_Mapped, data, static_cast<size_t>(size));
            VKBuffer::UnMap();
        }

        void VKUniformBuffer::SetDynamicData(uint32_t size, uint32_t typeSize, const void* data)
        {
            VKBuffer::Map();
            memcpy(m_Mapped, data, size);
            VKBuffer::Flush(size);
            VKBuffer::UnMap();
        }

        void VKUniformBuffer::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
            CreateDataFunc = CreateDataFuncVulkan;
        }

        UniformBuffer* VKUniformBuffer::CreateDataFuncVulkan(uint32_t size, const void* data)
        {
            return new VKUniformBuffer(size, data);
        }

        UniformBuffer* VKUniformBuffer::CreateFuncVulkan()
        {
            return new VKUniformBuffer();
        }
    }
}
