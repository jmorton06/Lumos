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

            bool Wait(uint64_t timeoutNanoseconds = 5000000000 /*5 Seconds*/);
            void Reset();
            bool WaitAndReset();

        private:
            VkFence m_Handle;
            bool m_Signaled;
        };
    }
}
