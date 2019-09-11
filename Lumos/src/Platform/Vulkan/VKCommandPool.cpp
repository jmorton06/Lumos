#include "lmpch.h"
#include "VKCommandPool.h"
#include "VKDevice.h"

namespace Lumos
{
	namespace Graphics
	{
		VKCommandPool::VKCommandPool()
		{
			Init();
		}

		VKCommandPool::~VKCommandPool()
		{
			VKDevice::Instance()->GetDevice().destroyCommandPool(m_CommandPool);
		}

		void VKCommandPool::Init()
		{
			vk::CommandPoolCreateInfo cmdPoolCI{};

			cmdPoolCI.queueFamilyIndex = VKDevice::Instance()->GetGraphicsQueueFamilyIndex();
			cmdPoolCI.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

			m_CommandPool = VKDevice::Instance()->GetDevice().createCommandPool(cmdPoolCI);
		}
	}
}
