#include "Precompiled.h"
#include "VKCommandPool.h"
#include "VKDevice.h"

namespace Lumos
{
    namespace Graphics
    {
        VKCommandPool::VKCommandPool(int queueIndex, VkCommandPoolCreateFlags flags)
        {
            VkCommandPoolCreateInfo cmdPoolCI {};
            cmdPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            cmdPoolCI.queueFamilyIndex = queueIndex;
            cmdPoolCI.flags = flags;

            vkCreateCommandPool(VKDevice::Get().GetDevice(), &cmdPoolCI, nullptr, &m_Handle);
        }

        VKCommandPool::~VKCommandPool()
        {
            vkDestroyCommandPool(VKDevice::Get().GetDevice(), m_Handle, nullptr);
        }

        void VKCommandPool::Reset()
        {
            vkResetCommandPool(VKDevice::Get().GetDevice(), m_Handle, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
        }
    }
}
