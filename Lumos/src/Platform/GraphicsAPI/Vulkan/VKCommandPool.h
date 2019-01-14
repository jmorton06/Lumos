#pragma once

#include "Dependencies/vulkan/vulkan.h"

namespace jm
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
