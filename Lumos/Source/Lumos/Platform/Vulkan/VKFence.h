#pragma once
#include "VK.h"

namespace Lumos
{
    namespace Graphics
    {
        class VKFence
        {
        public:
            VKFence(bool createSignaled = false);
            ~VKFence();

            bool CheckState();
            bool IsSignaled();
            VkFence& GetHandle() { return m_Handle; }

            bool Wait(uint64_t timeoutNanoseconds = 1000000000);
            void Reset();
            void WaitAndReset();

        private:
            VkFence m_Handle;
            bool m_Signaled;
        };
    }
}
