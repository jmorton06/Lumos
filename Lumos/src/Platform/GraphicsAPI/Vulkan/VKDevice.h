#pragma once
#include "Utilities/TSingleton.h"
#include "Dependencies/vulkan/vulkan.h"
#include "VKContext.h"

namespace jm
{
	namespace graphics
	{
		struct QueueFamilyIndices
		{
			int graphicsFamily = -1;
			int presentFamily = -1;

			bool isComplete()
			{
				return graphicsFamily >= 0 && presentFamily >= 0;
			}
		};

		class VKDevice : public TSingleton<VKDevice>
		{
			friend class TSingleton<VKDevice>;

		public:
			VKDevice();
			~VKDevice();

			bool Init();
			void Unload();
			bool MemoryTypeFromProperties(uint32_t typeBits, VkFlags reqMask, uint32_t* typeIndex);
			void CreatePipelineCache();

			QueueFamilyIndices FindQueueFamilies() const;

			VkDevice GetDevice()							const { return m_Device; };
			VkPhysicalDevice GetGPU()						const { return m_PhysicalDevice; };
			VkQueue GetGraphicsQueue()						const { return m_GraphicsQueue; };
			VkQueue GetPresentQueue()						const { return m_PresentQueue; };
			uint32_t GetGraphicsQueueFamilyIndex()			const { return m_GraphicsQueueFamilyIndex; };
			VkSurfaceKHR GetSurface()						const { return m_Surface; };
			VkFormat GetFormat()							const { return m_Format; };
			VkPhysicalDeviceProperties GetGPUProperties()	const { return m_PhysicalDeviceProperties; };
			VkPipelineCache GetPipelineCache() 				const { return m_PipelineCache; }
			VKContext* GetVKContext() 						const { return m_VKContext; }

			uint m_SwapChainSize = 0;

		private:

			VkDevice m_Device;
			VkPhysicalDevice m_PhysicalDevice;
			VkPhysicalDeviceProperties m_PhysicalDeviceProperties;
			VkPhysicalDeviceMemoryProperties m_MemoryProperties;
			std::vector<VkQueueFamilyProperties> m_QueueFamiliyProperties;
			VkSurfaceKHR m_Surface;
			uint32_t m_GraphicsQueueFamilyIndex;
			VkFormat m_Format;
			VkQueue m_GraphicsQueue;
			VkQueue m_PresentQueue;
			VkPipelineCache m_PipelineCache;
			VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;

			VKContext* m_VKContext;
		};
	}
}
