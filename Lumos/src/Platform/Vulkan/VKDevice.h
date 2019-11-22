#pragma once
#include "Utilities/TSingleton.h"
#include "VK.h"
#include "VKContext.h"

#ifdef USE_VMA_ALLOCATOR
#ifdef LUMOS_DEBUG
#define VMA_DEBUG_MARGIN 16
#define VMA_DEBUG_DETECT_CORRUPTION 1
#endif
#include <vulkan/vk_mem_alloc.h>
#endif

namespace Lumos
{
	namespace Graphics
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

		class LUMOS_EXPORT VKDevice : public TSingleton<VKDevice>
		{
			friend class TSingleton<VKDevice>;

		public:
			VKDevice();
			~VKDevice();

			bool Init();
			void Unload();
			bool MemoryTypeFromProperties(uint32_t typeBits, VkMemoryPropertyFlags reqMask, uint32_t* typeIndex);
			void CreatePipelineCache();

			VkDevice GetDevice()							const { return m_Device; };
			VkPhysicalDevice GetGPU()						const { return m_PhysicalDevice; };
			VkQueue GetGraphicsQueue()					    const { return m_GraphicsQueue; };
			VkQueue GetPresentQueue()						const { return m_PresentQueue; };
			uint32_t GetGraphicsQueueFamilyIndex()			const { return m_GraphicsQueueFamilyIndex; };
			VkSurfaceKHR GetSurface()						const { return m_Surface; };
			VkFormat GetFormat()							const { return m_Format; };
            VkColorSpaceKHR GetColourSpace()                const { return m_ColourSpace; };

			VkPhysicalDeviceProperties GetGPUProperties()	const { return m_PhysicalDeviceProperties; };
			VkPipelineCache GetPipelineCache() 			    const { return m_PipelineCache; }

			VKContext* GetVKContext() 						const { return m_VKContext; }
            
#ifdef USE_VMA_ALLOCATOR
            VmaAllocator GetAllocator()                     const { return m_Allocator; }
#endif

			VkSurfaceKHR CreatePlatformSurface(VkInstance vkInstance, Window* window);
            
			static VkDevice Device() { return VKDevice::Instance()->GetDevice(); }
		private:
			VkDevice m_Device;
			VkPhysicalDevice m_PhysicalDevice;
			VkPhysicalDeviceProperties m_PhysicalDeviceProperties;
			VkPhysicalDeviceMemoryProperties m_MemoryProperties;
			std::vector<VkQueueFamilyProperties> m_QueueFamiliyProperties;
			VkSurfaceKHR m_Surface;
			uint32_t m_GraphicsQueueFamilyIndex;
			VkFormat m_Format;
            VkColorSpaceKHR m_ColourSpace;
			VkQueue m_GraphicsQueue;
			VkQueue m_PresentQueue;
			VkPipelineCache m_PipelineCache;
			VkDescriptorPool m_DescriptorPool;

			VKContext* m_VKContext;
            
#ifdef USE_VMA_ALLOCATOR
            VmaAllocator m_Allocator;
#endif
		};
	}
}
