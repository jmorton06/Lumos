#pragma once

#include "VK.h"

namespace Lumos
{
    namespace Graphics
    {
        class VKCommandPool
        {
        public:
            VKCommandPool();
            ~VKCommandPool();

            void Init();

            const VkCommandPool& GetCommandPool() const { return m_CommandPool; }

        private:
            VkCommandPool m_CommandPool;
        };
    }
}
