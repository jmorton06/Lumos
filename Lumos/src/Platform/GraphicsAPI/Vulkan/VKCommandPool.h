#pragma once

#include "VK.h"

namespace Lumos
{
	namespace graphics
	{
		class VKCommandPool
		{
		public:
			VKCommandPool();
			~VKCommandPool();

			bool Init();
			void Unload() const;
			VkCommandPool GetCommandPool() const;

		private:
			VkCommandPool m_CommandPool;
		};
	}
}
