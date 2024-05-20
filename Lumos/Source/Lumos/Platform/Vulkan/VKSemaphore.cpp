#include "Precompiled.h"
#include "VKSemaphore.h"
#include "VKDevice.h"
#include "VKUtilities.h"

namespace Lumos
{
    namespace Graphics
    {
        VKSemaphore::VKSemaphore(bool timeline)
        {
            LUMOS_PROFILE_FUNCTION();
            m_Timeline = timeline;

            VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo = {};
            semaphoreTypeCreateInfo.sType                     = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
            semaphoreTypeCreateInfo.pNext                     = nullptr;
            semaphoreTypeCreateInfo.semaphoreType             = VK_SEMAPHORE_TYPE_TIMELINE;
            semaphoreTypeCreateInfo.initialValue              = 0;

            VkSemaphoreCreateInfo semaphoreInfo = {};
            semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            semaphoreInfo.pNext                 = m_Timeline ? &semaphoreTypeCreateInfo : nullptr;
            semaphoreInfo.flags                 = 0;

            VK_CHECK_RESULT(vkCreateSemaphore(VKDevice::Get().GetDevice(), &semaphoreInfo, nullptr, &m_Handle));
        }

        VKSemaphore::~VKSemaphore()
        {
            vkDestroySemaphore(VKDevice::Get().GetDevice(), m_Handle, nullptr);
        }

        void VKSemaphore::Wait(uint64_t value, uint64_t timeout)
        {
            LUMOS_PROFILE_FUNCTION();
            LUMOS_ASSERT(m_Timeline);

            VkSemaphoreWaitInfo semaphoreWaitInfo = {};
            semaphoreWaitInfo.sType               = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
            semaphoreWaitInfo.pNext               = nullptr;
            semaphoreWaitInfo.flags               = 0;
            semaphoreWaitInfo.semaphoreCount      = 1;
            semaphoreWaitInfo.pSemaphores         = &m_Handle;
            semaphoreWaitInfo.pValues             = &value;

            LUMOS_ASSERT(vkWaitSemaphores(VKDevice::Get().GetDevice(), &semaphoreWaitInfo, timeout), "Failed to wait for semaphore");
        }

        void VKSemaphore::Signal(uint64_t value)
        {
            LUMOS_ASSERT(m_Timeline);

            VkSemaphoreSignalInfo semaphoreSignalInfo = {};
            semaphoreSignalInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
            semaphoreSignalInfo.pNext                 = nullptr;
            semaphoreSignalInfo.semaphore             = m_Handle;
            semaphoreSignalInfo.value                 = value;

            VK_CHECK_RESULT(vkSignalSemaphore(VKDevice::Get().GetDevice(), &semaphoreSignalInfo));
        }

        uint64_t VKSemaphore::GetValue()
        {
            LUMOS_ASSERT(m_Timeline);

            uint64_t value = 0;
            VK_CHECK_RESULT(
                vkGetSemaphoreCounterValue(VKDevice::Get().GetDevice(), m_Handle, &value));

            return value;
        }
    }
}
