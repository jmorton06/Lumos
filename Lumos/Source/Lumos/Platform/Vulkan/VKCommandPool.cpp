#include "Precompiled.h"
#include "VKCommandPool.h"
#include "VKDevice.h"

namespace Lumos
{
    namespace Graphics
    {
        VKCommandPool::VKCommandPool()
        {
            Init();
        }

        VKCommandPool::~VKCommandPool()
        {
            vkDestroyCommandPool(VKDevice::Get().GetDevice(), m_CommandPool, nullptr);
        }

        void VKCommandPool::Init()
        {
            VkCommandPoolCreateInfo cmdPoolCI {};

            cmdPoolCI.queueFamilyIndex = VKDevice::Get().GetPhysicalDevice()->GetGraphicsQueueFamilyIndex();
            cmdPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            cmdPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            vkCreateCommandPool(VKDevice::Get().GetDevice(), &cmdPoolCI, nullptr, &m_CommandPool);
        }
    }
}
