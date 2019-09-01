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

			vk::CommandPool GetCommandPool() const { return m_CommandPool; }
            
        private:
			vk::CommandPool m_CommandPool;
		};
	}
}
