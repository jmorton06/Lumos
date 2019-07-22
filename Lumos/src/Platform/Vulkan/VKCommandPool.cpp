#include "LM.h"
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
			vkDestroyCommandPool(VKDevice::Instance()->GetDevice(), m_CommandPool, VK_NULL_HANDLE);
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
