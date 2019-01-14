#pragma once

#include "Dependencies/vulkan/vulkan.h"

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
