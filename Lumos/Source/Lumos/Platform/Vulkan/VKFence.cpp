#include "Precompiled.h"
#include "VKFence.h"
#include "VKDevice.h"
#include "VKTools.h"

namespace Lumos
{
    namespace Graphics
    {
        VKFence::VKFence(bool createSignaled)
        {
            LUMOS_PROFILE_FUNCTION();
            m_Signaled = createSignaled;

            VkFenceCreateInfo fenceCreateInfo {};
            fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceCreateInfo.flags = createSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

            vkCreateFence(VKDevice::Get().GetDevice(), &fenceCreateInfo, nullptr, &m_Handle);
        }

        VKFence::~VKFence()
        {
            vkDestroyFence(VKDevice::Get().GetDevice(), m_Handle, nullptr);
        }

        bool VKFence::IsSignaled()
        {
            LUMOS_PROFILE_FUNCTION();
            if(m_Signaled)
                return true;
            else
            {
                return CheckState();
            }
        }

        bool VKFence::CheckState()
        {
            LUMOS_ASSERT(!m_Signaled, "Fence Signaled");

            const VkResult result = vkGetFenceStatus(VKDevice::Get().GetDevice(), m_Handle);
            if(result == VK_SUCCESS)
            {
                m_Signaled = true;
                return true;
            }

            return false;
        }

        bool VKFence::Wait()
        {
            LUMOS_PROFILE_FUNCTION();
            LUMOS_ASSERT(!m_Signaled, "Fence Signaled");

            const VkResult result = vkWaitForFences(VKDevice::Get().GetDevice(), 1, &m_Handle, true, UINT32_MAX);

            VK_CHECK_RESULT(result);
            if(result == VK_SUCCESS)
            {
                m_Signaled = true;
                return false;
            }

            return true;
        }

        void VKFence::Reset()
        {
            LUMOS_PROFILE_FUNCTION();
            if(m_Signaled)
                VK_CHECK_RESULT(vkResetFences(VKDevice::Get().GetDevice(), 1, &m_Handle));

            m_Signaled = false;
        }

        void VKFence::WaitAndReset()
        {
            LUMOS_PROFILE_FUNCTION();
            if(!IsSignaled())
                Wait();

            Reset();
        }
    }
}
