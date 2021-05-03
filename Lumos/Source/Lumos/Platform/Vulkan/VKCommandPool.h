#pragma once

#include "VK.h"

namespace Lumos
{
    namespace Graphics
    {
        class VKCommandPool
        {
        public:
            VKCommandPool(int queueIndex);
            ~VKCommandPool();

            void Init(int queueIndex);
            void Reset();

            const VkCommandPool& GetCommandPool() const { return m_CommandPool; }

        private:
            VkCommandPool m_CommandPool;
        };
    }
}
