#include "Precompiled.h"
#include "VKCommandPool.h"
#include "VKDevice.h"

namespace Lumos
{
    namespace Graphics
    {
        VKCommandPool::VKCommandPool(int queueIndex)
        {
            Init(queueIndex);
        }

        VKCommandPool::~VKCommandPool()
        {
            vkDestroyCommandPool(VKDevice::Get().GetDevice(), m_CommandPool, nullptr);
        }

        void VKCommandPool::Init(int queueIndex)
        {
            VkCommandPoolCreateInfo cmdPoolCI {};

            cmdPoolCI.queueFamilyIndex = queueIndex;
            cmdPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            cmdPoolCI.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

            vkCreateCommandPool(VKDevice::Get().GetDevice(), &cmdPoolCI, nullptr, &m_CommandPool);
        }
    
        void VKCommandPool::Reset()
        {
            vkResetCommandPool(VKDevice::Get().GetDevice(), m_CommandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
        }
    }
}
