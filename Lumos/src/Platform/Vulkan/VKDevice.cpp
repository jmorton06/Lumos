#include "lmpch.h"

#include "App/Application.h"
#include "Core/Version.h"

#if defined(LUMOS_PLATFORM_MACOS)
#ifndef VK_USE_PLATFORM_MACOS_MVK
#define VK_USE_PLATFORM_MACOS_MVK
#endif
#endif

#include "VKDevice.h"

namespace Lumos
{
	namespace Graphics
	{
		VKDevice::VKDevice()
		{
			m_VKContext = static_cast<VKContext*>(GraphicsContext::GetContext());

			Init();

			CreatePipelineCache();
		}
		
		VKDevice::~VKDevice()
		{
			Unload();
		}

		const char* TranslateVkPhysicalDeviceTypeToString(VkPhysicalDeviceType type)
		{
			switch (type)
			{
			case VK_PHYSICAL_DEVICE_TYPE_OTHER: return "OTHER";
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "INTEGRATED GPU";
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return "DISCRETE GPU";
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return "VIRTUAL GPU";
			case VK_PHYSICAL_DEVICE_TYPE_CPU: return "CPU";
			default: return "UNKNOWN";
			}
		}

		bool VKDevice::Init()
		{
			// GPU
			uint32_t numGPUs = 0;
			vkEnumeratePhysicalDevices(m_VKContext->GetVKInstance(), &numGPUs, VK_NULL_HANDLE);
			if (numGPUs == 0)
			{
				LUMOS_LOG_ERROR("ERROR : No GPUs found!");
				return false;
			}

			std::vector<VkPhysicalDevice> pGPUs(numGPUs);
			vkEnumeratePhysicalDevices(m_VKContext->GetVKInstance(), &numGPUs, pGPUs.data());
			m_PhysicalDevice = pGPUs[0];

			vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_PhysicalDeviceProperties);
			vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_MemoryProperties);

			LUMOS_LOG_INFO("Vulkan : {0}.{1}.{2}", VK_VERSION_MAJOR(m_PhysicalDeviceProperties.apiVersion), VK_VERSION_MINOR(m_PhysicalDeviceProperties.apiVersion), VK_VERSION_PATCH(m_PhysicalDeviceProperties.apiVersion));
			LUMOS_LOG_INFO("GPU : {0}", std::string(m_PhysicalDeviceProperties.deviceName));
			LUMOS_LOG_INFO("Vendor ID : {0}", StringFormat::ToString(m_PhysicalDeviceProperties.vendorID));
			LUMOS_LOG_INFO("Device Type : {0}", String(TranslateVkPhysicalDeviceTypeToString(m_PhysicalDeviceProperties.deviceType)));
			LUMOS_LOG_INFO("Driver Version : {0}.{1}.{2}", VK_VERSION_MAJOR(m_PhysicalDeviceProperties.driverVersion), VK_VERSION_MINOR(m_PhysicalDeviceProperties.driverVersion), VK_VERSION_PATCH(m_PhysicalDeviceProperties.driverVersion));

			// Queue family
			uint32_t numQueueFamily = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &numQueueFamily, VK_NULL_HANDLE);
			if (numQueueFamily == 0)
			{
				LUMOS_LOG_ERROR("ERROR : No Queue Families were found!");
				return false;
			}

			m_QueueFamiliyProperties.resize(numQueueFamily);
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &numQueueFamily, m_QueueFamiliyProperties.data());

			m_Surface = CreatePlatformSurface(m_VKContext->GetVKInstance(), Application::Instance()->GetWindow());

			if(!m_Surface)
			{
				LUMOS_LOG_CRITICAL("[VULKAN] Failed to create window surface!");
			}

			VkBool32* supportsPresent = lmnew VkBool32[m_QueueFamiliyProperties.size()];
			for (uint32_t i = 0; i < m_QueueFamiliyProperties.size(); i++)
				vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice,i, m_Surface,&supportsPresent[i]);

			m_GraphicsQueueFamilyIndex = UINT32_MAX;
			for (uint32_t i = 0; i < m_QueueFamiliyProperties.size(); i++)
			{
				if ((m_QueueFamiliyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
				{
					if (supportsPresent[i] == VK_TRUE)
					{
						m_GraphicsQueueFamilyIndex = i;
						break;
					}
				}
			}

			delete[] supportsPresent;

			if (m_GraphicsQueueFamilyIndex == UINT32_MAX)
			{
				LUMOS_LOG_CRITICAL("[VULKAN] Couldn't find a Graphics queue family index!");
				return false;
			}

			uint32_t numFormats;
			auto result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &numFormats, VK_NULL_HANDLE);
			if (result != VK_SUCCESS)
			{
				LUMOS_LOG_CRITICAL("ERROR : Couldn't get surface formats!");
				return false;
			}

			VkSurfaceFormatKHR * pSurfaceFormats = new VkSurfaceFormatKHR[numFormats];
			result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &numFormats, pSurfaceFormats);

			if (numFormats == 1 && pSurfaceFormats[0].format == VK_FORMAT_UNDEFINED)
				m_Format = VK_FORMAT_B8G8R8A8_UNORM;
			else
				m_Format = pSurfaceFormats[0].format;

			// Device queue

			float pQueuePriorities[] = { 1.0f };
			VkDeviceQueueCreateInfo deviceQueueCI{};
			deviceQueueCI.queueCount = 1;
			deviceQueueCI.queueFamilyIndex = m_GraphicsQueueFamilyIndex;
			deviceQueueCI.pQueuePriorities = pQueuePriorities;

			VkPhysicalDeviceFeatures deviceFeatures{};
			deviceFeatures.shaderClipDistance = VK_TRUE;
			deviceFeatures.shaderCullDistance = VK_TRUE;
			deviceFeatures.geometryShader = VK_FALSE;
			deviceFeatures.shaderTessellationAndGeometryPointSize = VK_TRUE;
			deviceFeatures.fillModeNonSolid = VK_TRUE;
			deviceFeatures.samplerAnisotropy = VK_TRUE;

			//auto extensions = m_VKContext->GetExtensionNames();
			auto layers		= m_VKContext->GetLayerNames();

			const std::vector<const char*> deviceExtensions = 
			{
				VK_KHR_SWAPCHAIN_EXTENSION_NAME
			};

			// Device
			VkDeviceCreateInfo deviceCI{};
			deviceCI.queueCreateInfoCount = 1;
			deviceCI.pQueueCreateInfos = &deviceQueueCI;
			deviceCI.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
			deviceCI.ppEnabledExtensionNames = deviceExtensions.data();
			deviceCI.pEnabledFeatures = &deviceFeatures;

			if (EnableValidationLayers)
			{
				deviceCI.enabledLayerCount = static_cast<uint32_t>(layers.size());
				deviceCI.ppEnabledLayerNames = layers.data();
			}
			else
			{
				deviceCI.enabledLayerCount = 0;
			}
			
			result = vkCreateDevice(m_PhysicalDevice, &deviceCI, VK_NULL_HANDLE, &m_Device);
			if (result != VK_SUCCESS)
			{
				LUMOS_LOG_CRITICAL("[VULKAN] VKDevice::Instance()->GetDevice() vkCreateDevice() failed!");
				return false;
			}

			vkGetDeviceQueue(m_Device, m_GraphicsQueueFamilyIndex, 0, &m_GraphicsQueue);
			vkGetDeviceQueue(m_Device, m_GraphicsQueueFamilyIndex, 0, &m_PresentQueue);
            
            
#ifdef USE_VMA_ALLOCATOR
            VmaAllocatorCreateInfo allocatorInfo = {};
            allocatorInfo.physicalDevice = m_PhysicalDevice;
            allocatorInfo.device = m_Device;
            
            VmaVulkanFunctions fn;
            fn.vkAllocateMemory                    = (PFN_vkAllocateMemory)vkAllocateMemory;
            fn.vkBindBufferMemory                  = (PFN_vkBindBufferMemory)vkBindBufferMemory;
            fn.vkBindImageMemory                   = (PFN_vkBindImageMemory)vkBindImageMemory;
            fn.vkCmdCopyBuffer                     = (PFN_vkCmdCopyBuffer)vkCmdCopyBuffer;
            fn.vkCreateBuffer                      = (PFN_vkCreateBuffer)vkCreateBuffer;
            fn. vkCreateImage                      = (PFN_vkCreateImage)vkCreateImage;
            fn.vkDestroyBuffer                     = (PFN_vkDestroyBuffer)vkDestroyBuffer;
            fn.vkDestroyImage                      = (PFN_vkDestroyImage)vkDestroyImage;
            fn.vkFlushMappedMemoryRanges           = (PFN_vkFlushMappedMemoryRanges)vkFlushMappedMemoryRanges;
            fn.vkFreeMemory                        = (PFN_vkFreeMemory)vkFreeMemory;
            fn.vkGetBufferMemoryRequirements       = (PFN_vkGetBufferMemoryRequirements)vkGetBufferMemoryRequirements;
            fn.vkGetImageMemoryRequirements        = (PFN_vkGetImageMemoryRequirements)vkGetImageMemoryRequirements;
            fn.vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)vkGetPhysicalDeviceMemoryProperties;
            fn.vkGetPhysicalDeviceProperties       = (PFN_vkGetPhysicalDeviceProperties)vkGetPhysicalDeviceProperties;
            fn.vkInvalidateMappedMemoryRanges      = (PFN_vkInvalidateMappedMemoryRanges)vkInvalidateMappedMemoryRanges;
            fn.vkMapMemory                         = (PFN_vkMapMemory)vkMapMemory;
            fn.vkUnmapMemory                       = (PFN_vkUnmapMemory)vkUnmapMemory;
            fn.vkGetBufferMemoryRequirements2KHR   = 0;  //(PFN_vkGetBufferMemoryRequirements2KHR)vkGetBufferMemoryRequirements2KHR;
            fn.vkGetImageMemoryRequirements2KHR    = 0;  //(PFN_vkGetImageMemoryRequirements2KHR)vkGetImageMemoryRequirements2KHR;
            allocatorInfo.pVulkanFunctions = &fn;

            if (vmaCreateAllocator(&allocatorInfo, &m_Allocator) != VK_SUCCESS)
            {
                LUMOS_LOG_CRITICAL("[VULKAN] Failed to create VMA allocator");
            }
#endif

			return VK_SUCCESS;
		}

		void VKDevice::Unload()
		{
			vkDestroyPipelineCache(VKDevice::Instance()->GetDevice(), m_PipelineCache, VK_NULL_HANDLE);

#ifdef USE_VMA_ALLOCATOR
			vmaDestroyAllocator(m_Allocator);
#endif

			vkDestroyDevice(m_Device, VK_NULL_HANDLE);
			vkDestroySurfaceKHR(m_VKContext->GetVKInstance(), m_Surface, VK_NULL_HANDLE);

		}

		bool VKDevice::MemoryTypeFromProperties(uint32_t typeBits, VkMemoryPropertyFlags reqMask, uint32_t * typeIndex)
		{
			for (uint32_t i = 0; i < m_MemoryProperties.memoryTypeCount; i++)
			{
				if ((typeBits & 1) == 1)
				{
					if ((m_MemoryProperties.memoryTypes[i].propertyFlags & reqMask) == reqMask)
					{
						*typeIndex = i;
						return true;
					}
				}
				typeBits >>= 1;
			}
			return false;
		}

		void VKDevice::CreatePipelineCache()
		{
			VkPipelineCacheCreateInfo pipelineCacheCI{};
			pipelineCacheCI.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
			pipelineCacheCI.pNext = NULL;
			vkCreatePipelineCache(m_Device, &pipelineCacheCI, VK_NULL_HANDLE, &m_PipelineCache);
		}
	}
}
