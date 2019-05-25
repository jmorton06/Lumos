#include "LM.h"
#include "VKCommandPool.h"
#include "VKDevice.h"

namespace lumos
{
	namespace graphics
	{
		VKCommandPool::VKCommandPool()
		{
			Init();
		}

		VKCommandPool::~VKCommandPool()
		{
			Unload();
		}

		bool VKCommandPool::Init()
		{
			vk::CommandPoolCreateInfo cmdPoolCI{};

			cmdPoolCI.queueFamilyIndex = VKDevice::Instance()->GetGraphicsQueueFamilyIndex();
			cmdPoolCI.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

			m_CommandPool = VKDevice::Instance()->GetDevice().createCommandPool(cmdPoolCI);

			return true;
		}

		void VKCommandPool::Unload() const
		{
			vkDestroyCommandPool(VKDevice::Instance()->GetDevice(), m_CommandPool, VK_NULL_HANDLE);
		}

		vk::CommandPool VKCommandPool::GetCommandPool() const
		{
			return m_CommandPool;
		}
	}
}
