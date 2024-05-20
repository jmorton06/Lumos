#pragma once
#include "VK.h"

namespace Lumos
{
    namespace Graphics
    {
        class VKSemaphore
        {
        public:
            VKSemaphore(bool timeline = false);
            ~VKSemaphore();

            bool IsSignaled();
            VkSemaphore& GetHandle() { return m_Handle; }

            void Signal(uint64_t value);
            void Wait(uint64_t value, uint64_t timeout);

            uint64_t GetValue();

        private:
            VkSemaphore m_Handle;
            bool m_Timeline;
        };
    }
}
