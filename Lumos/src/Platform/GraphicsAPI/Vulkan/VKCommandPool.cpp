#include "JM.h"
#include "VKCommandPool.h"
#include "VKDevice.h"

namespace jm
{
	namespace graphics
	{

		VKCommandPool::VKCommandPool()
		{
			m_CommandPool = VK_NULL_HANDLE;
			Init();
		}

		VKCommandPool::~VKCommandPool()
		{
			Unload();
			m_CommandPool = VK_NULL_HANDLE;
		}

		bool VKCommandPool::Init()
		{
			VkResult result;
			VkCommandPoolCreateInfo cmdPoolCI{};

			cmdPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cmdPoolCI.queueFamilyIndex = VKDevice::Instance()->GetGraphicsQueueFamilyIndex();
			cmdPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

			result = vkCreateCommandPool(VKDevice::Instance()->GetDevice(), &cmdPoolCI, VK_NULL_HANDLE, &m_CommandPool);

			return result == VK_SUCCESS;
		}

		void VKCommandPool::Unload() const
		{
			vkDestroyCommandPool(VKDevice::Instance()->GetDevice(), m_CommandPool, VK_NULL_HANDLE);
		}

		VkCommandPool VKCommandPool::GetCommandPool() const
		{
			return m_CommandPool;
		}
	}
}
