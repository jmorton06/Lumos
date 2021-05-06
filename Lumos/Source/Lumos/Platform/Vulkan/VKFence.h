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

            void SetSignaled(bool signaled)
            {
                m_Signaled = signaled;
            }

            bool Wait();
            void Reset();
            void WaitAndReset();

        private:
            VkFence m_Handle;
            bool m_Signaled;
        };
    }
}
