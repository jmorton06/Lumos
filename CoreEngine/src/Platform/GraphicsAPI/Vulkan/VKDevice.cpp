#include "JM.h"

#if defined(JM_PLATFORM_MACOS)
#ifndef VK_USE_PLATFORM_MACOS_MVK
#define VK_USE_PLATFORM_MACOS_MVK
#endif
#define GLFW_EXPOSE_NATIVE_COCOA
#endif

#include "VKDevice.h"

#include "VKInitialisers.h"
#ifndef JM_PLATFORM_IOS
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#endif

#ifdef JM_PLATFORM_MACOS
extern "C" {
    void* makeViewMetalCompatible(void* handle);
}
#endif

namespace jm
{
	namespace graphics
	{
		VKDevice::VKDevice()
		{
			m_Device  = VK_NULL_HANDLE;
			m_Surface = VK_NULL_HANDLE;

			m_VKContext = static_cast<VKContext*>(Context::GetContext());

			Init();

			CreatePipelineCache();
		}

		VKDevice::~VKDevice()
		{
			Unload();

			m_Device = VK_NULL_HANDLE;
			m_Surface = VK_NULL_HANDLE;
		}

		bool VKDevice::Init()
		{
			VkResult result;

			// GPU
			uint32_t numGPUs = 0;
			vkEnumeratePhysicalDevices(m_VKContext->GetVKInstance(), &numGPUs, VK_NULL_HANDLE);
			if (numGPUs == 0)
			{
				JM_CORE_ERROR("ERROR : No GPUs found!");
				return false;
			}

			std::vector<VkPhysicalDevice> pGPUs(numGPUs);
			vkEnumeratePhysicalDevices(m_VKContext->GetVKInstance(), &numGPUs, pGPUs.data());
			m_PhysicalDevice = pGPUs[0];

			vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_PhysicalDeviceProperties);
			vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_MemoryProperties);
			JM_CORE_INFO("Rendering with : " + std::string(m_PhysicalDeviceProperties.deviceName));

			// Queue family
			uint32_t numQueueFamily = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &numQueueFamily, VK_NULL_HANDLE);
			if (numQueueFamily == 0)
			{
				JM_CORE_ERROR("ERROR : No Queue Families were found!");
				return false;
			}

			m_QueueFamiliyProperties.resize(numQueueFamily);
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &numQueueFamily, m_QueueFamiliyProperties.data());

#ifndef JM_PLATFORM_IOS
			// Surface
#if defined(JM_PLATFORM_MACOS)
            VkMacOSSurfaceCreateInfoMVK surfaceInfo;
            surfaceInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
            surfaceInfo.pNext = NULL;
            surfaceInfo.flags = 0;
            surfaceInfo.pView = makeViewMetalCompatible((void*)glfwGetCocoaWindow(static_cast<GLFWwindow*>(m_VKContext->GetWindowContext())));
            vkCreateMacOSSurfaceMVK(m_VKContext->GetVKInstance(), &surfaceInfo, NULL, &m_Surface);
#else
            if (glfwCreateWindowSurface(m_VKContext->GetVKInstance(), static_cast<GLFWwindow*>(m_VKContext->GetWindowContext()), nullptr, &m_Surface) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create window surface!");
            }
#endif
#endif

			VkBool32 * supportsPresent = new VkBool32[m_QueueFamiliyProperties.size()];
			for (uint32_t i = 0; i < m_QueueFamiliyProperties.size(); i++)
				vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, i, m_Surface, &supportsPresent[i]);

			m_GraphicsQueueFamilyIndex = UINT32_MAX;
			for (uint32_t i = 0; i < m_QueueFamiliyProperties.size(); i++)
			{
				if ((m_QueueFamiliyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
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
				JM_CORE_ERROR("ERROR : Couldn't find a graphics queue family index!");
				return false;
			}

			uint32_t numFormats;
			result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &numFormats, VK_NULL_HANDLE);
			if (result != VK_SUCCESS)
			{
				JM_CORE_ERROR("ERROR : Couldn't get surface formats!");
				return false;
			}

			VkSurfaceFormatKHR * pSurfaceFormats = new VkSurfaceFormatKHR[numFormats];
			result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &numFormats, pSurfaceFormats);

			if (numFormats == 1 && pSurfaceFormats[0].format == VK_FORMAT_UNDEFINED)
				m_Format = VK_FORMAT_B8G8R8A8_UNORM;
			else
				m_Format = pSurfaceFormats[0].format;

            delete[] pSurfaceFormats;
			// Device queue

			float pQueuePriorities[] = { 1.0f };
			VkDeviceQueueCreateInfo deviceQueueCI{};
			deviceQueueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			deviceQueueCI.queueCount = 1;
			deviceQueueCI.queueFamilyIndex = m_GraphicsQueueFamilyIndex;
			deviceQueueCI.pQueuePriorities = pQueuePriorities;

			VkPhysicalDeviceFeatures deviceFeatures{};
			deviceFeatures.shaderClipDistance = VK_TRUE;
			deviceFeatures.shaderCullDistance = VK_TRUE;
			deviceFeatures.geometryShader = VK_TRUE;
			deviceFeatures.shaderTessellationAndGeometryPointSize = VK_TRUE;
			deviceFeatures.fillModeNonSolid = VK_TRUE;
			deviceFeatures.samplerAnisotropy = VK_TRUE;

			// Device
			VkDeviceCreateInfo deviceCI{};
			deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			deviceCI.queueCreateInfoCount = 1;
			deviceCI.pQueueCreateInfos = &deviceQueueCI;
			deviceCI.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
			deviceCI.ppEnabledExtensionNames = deviceExtensions.data();
			deviceCI.pEnabledFeatures = &deviceFeatures;

			if (enableValidationLayers)
			{
				deviceCI.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
				deviceCI.ppEnabledLayerNames = validationLayers.data();
			}
			else
			{
				deviceCI.enabledLayerCount = 0;
			}
			result = vkCreateDevice(m_PhysicalDevice, &deviceCI, VK_NULL_HANDLE, &m_Device);
			if (result != VK_SUCCESS)
			{
				JM_CORE_ERROR("ERROR : vkCreateDevice() failed!");
				return false;
			}

			vkGetDeviceQueue(m_Device, m_GraphicsQueueFamilyIndex, 0, &m_GraphicsQueue);
			vkGetDeviceQueue(m_Device, m_GraphicsQueueFamilyIndex, 0, &m_PresentQueue);

			return result == VK_SUCCESS;
		}

		void VKDevice::Unload()
		{
			vkDestroyPipelineCache(VKDevice::Instance()->GetDevice(), m_PipelineCache, VK_NULL_HANDLE);
			vkDestroyDevice(m_Device, VK_NULL_HANDLE);
			vkDestroySurfaceKHR(m_VKContext->GetVKInstance(), m_Surface, VK_NULL_HANDLE);
		}

		bool VKDevice::MemoryTypeFromProperties(uint32_t typeBits, VkFlags reqMask, uint32_t * typeIndex)
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

		QueueFamilyIndices VKDevice::FindQueueFamilies() const
		{
			QueueFamilyIndices indices;

			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilies.data());

			int i = 0;
			for (const auto& queueFamily : queueFamilies)
			{
				if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					indices.graphicsFamily = i;
				}

				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, i, VKDevice::Instance()->GetSurface(), &presentSupport);

				if (queueFamily.queueCount > 0 && presentSupport)
				{
					indices.presentFamily = i;
				}

				if (indices.isComplete())
				{
					break;
				}

				i++;
			}

			return indices;
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
