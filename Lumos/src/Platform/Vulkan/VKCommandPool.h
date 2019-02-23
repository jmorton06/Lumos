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
			vk::CommandPool GetCommandPool() const;

		private:
			vk::CommandPool m_CommandPool;
		};
	}
}
