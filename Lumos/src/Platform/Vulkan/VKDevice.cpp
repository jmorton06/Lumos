#include "LM.h"

#include "App/Application.h"
#include "Core/Version.h"

#if defined(LUMOS_PLATFORM_MACOS)
#ifndef VK_USE_PLATFORM_MACOS_MVK
#define VK_USE_PLATFORM_MACOS_MVK
#endif
#define GLFW_EXPOSE_NATIVE_COCOA
#endif

#include "VKDevice.h"

#ifndef LUMOS_PLATFORM_IOS
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#endif

#ifdef LUMOS_PLATFORM_WINDOWS
#include "Platform/Windows/WindowsWindow.h"
#endif

#ifdef LUMOS_PLATFORM_MACOS
extern "C" {
    void* makeViewMetalCompatible(void* handle);
}
#endif

#ifdef LUMOS_PLATFORM_IOS
void* Lumos::Graphics::VKDevice::m_IOSView = nullptr;
#endif

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

		const char* TranslateVkPhysicalDeviceTypeToString(vk::PhysicalDeviceType type)
		{
			switch (type)
			{
			case vk::PhysicalDeviceType::eOther: return "OTHER";
			case vk::PhysicalDeviceType::eIntegratedGpu: return "INTEGRATED GPU";
			case vk::PhysicalDeviceType::eDiscreteGpu: return "DISCRETE GPU";
			case vk::PhysicalDeviceType::eVirtualGpu: return "VIRTUAL GPU";
			case vk::PhysicalDeviceType::eCpu: return "CPU";
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
				LUMOS_CORE_ERROR("[VULKAN] No GPUs found!");
				return false;
			}

			std::vector<vk::PhysicalDevice> pGPUs(numGPUs);
			m_VKContext->GetVKInstance().enumeratePhysicalDevices(&numGPUs, pGPUs.data());
			m_PhysicalDevice = pGPUs[0];
			m_PhysicalDeviceProperties = m_PhysicalDevice.getProperties();
			m_MemoryProperties = m_PhysicalDevice.getMemoryProperties();

			LUMOS_CORE_INFO("Vulkan : {0}.{1}.{2}", VK_VERSION_MAJOR(m_PhysicalDeviceProperties.apiVersion), VK_VERSION_MINOR(m_PhysicalDeviceProperties.apiVersion), VK_VERSION_PATCH(m_PhysicalDeviceProperties.apiVersion));
			LUMOS_CORE_INFO("GPU : {0}", std::string(m_PhysicalDeviceProperties.deviceName));
			LUMOS_CORE_INFO("Vendor ID : {0}", StringFormat::ToString(m_PhysicalDeviceProperties.vendorID));
			LUMOS_CORE_INFO("Device Type : {0}", String(TranslateVkPhysicalDeviceTypeToString(m_PhysicalDeviceProperties.deviceType)));
			LUMOS_CORE_INFO("Driver Version : {0}.{1}.{2}", VK_VERSION_MAJOR(m_PhysicalDeviceProperties.driverVersion), VK_VERSION_MINOR(m_PhysicalDeviceProperties.driverVersion), VK_VERSION_PATCH(m_PhysicalDeviceProperties.driverVersion));

			// Queue family
			m_QueueFamiliyProperties = m_PhysicalDevice.getQueueFamilyProperties();
			if (m_QueueFamiliyProperties.size() == 0)
			{
				LUMOS_CORE_ERROR("[VULKAN] No Queue Families were found!");
				return false;
			}

#ifdef LUMOS_PLATFORM_IOS
			vk::IOSSurfaceCreateInfoMVK surfaceCreateInfo = {};
			surfaceCreateInfo.pNext = NULL;
			surfaceCreateInfo.pView = m_IOSView;
			m_Surface = m_VKContext->GetVKInstance().createIOSSurfaceMVK(surfaceCreateInfo);
#endif

#ifdef LUMOS_PLATFORM_MACOS
            vk::MacOSSurfaceCreateInfoMVK surfaceInfo;
            surfaceInfo.pNext = NULL;
            surfaceInfo.pView = makeViewMetalCompatible((void*)glfwGetCocoaWindow(static_cast<GLFWwindow*>(m_VKContext->GetWindowContext())));
			m_Surface = m_VKContext->GetVKInstance().createMacOSSurfaceMVK(surfaceInfo);
#endif

#ifdef LUMOS_PLATFORM_WINDOWS
#ifdef LUMOS_USE_GLFW_WINDOWS
			glfwCreateWindowSurface(m_VKContext->GetVKInstance(), static_cast<GLFWwindow*>(m_VKContext->GetWindowContext()), nullptr, (VkSurfaceKHR*)&m_Surface);
#else
			vk::Win32SurfaceCreateInfoKHR surfaceInfo;
			surfaceInfo.pNext = NULL;
			surfaceInfo.hwnd = (HWND)VKContext::Get()->GetWindowContext();
			surfaceInfo.hinstance = ((WindowsWindow*)Application::Instance()->GetWindow())->GetHInstance();
			m_Surface = m_VKContext->GetVKInstance().createWin32SurfaceKHR(surfaceInfo);
#endif
#endif

#ifdef LUMOS_PLATFORM_LINUX
			glfwCreateWindowSurface(m_VKContext->GetVKInstance(), static_cast<GLFWwindow*>(m_VKContext->GetWindowContext()), nullptr, (VkSurfaceKHR*)&m_Surface);

#endif
			if(!m_Surface)
			{
				LUMOS_CORE_ERROR("[VULKAN] Failed to create window surface!");
			}

			VkBool32 * supportsPresent = new VkBool32[m_QueueFamiliyProperties.size()];
			for (uint32_t i = 0; i < m_QueueFamiliyProperties.size(); i++)
				supportsPresent[i] = m_PhysicalDevice.getSurfaceSupportKHR(i, m_Surface);

			m_GraphicsQueueFamilyIndex = UINT32_MAX;
			for (uint32_t i = 0; i < m_QueueFamiliyProperties.size(); i++)
			{
				if ((m_QueueFamiliyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics))
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
				LUMOS_CORE_ERROR("[VULKAN] Couldn't find a Graphics queue family index!");
				return false;
			}

			std::vector<vk::SurfaceFormatKHR> formats = m_PhysicalDevice.getSurfaceFormatsKHR(m_Surface);

			if (formats.empty())
			{
				LUMOS_CORE_ERROR("[VULKAN] Couldn't get surface formats!");
				return false;
			}

			if (formats.size() == 1 && formats[0].format == vk::Format::eUndefined)
				m_Format = vk::Format::eB8G8R8A8Unorm;
			else
				m_Format = formats[0].format;

			// Device queue

			float pQueuePriorities[] = { 1.0f };
			vk::DeviceQueueCreateInfo deviceQueueCI{};
			deviceQueueCI.queueCount = 1;
			deviceQueueCI.queueFamilyIndex = m_GraphicsQueueFamilyIndex;
			deviceQueueCI.pQueuePriorities = pQueuePriorities;

			vk::PhysicalDeviceFeatures deviceFeatures{};
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
			vk::DeviceCreateInfo deviceCI{};
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
			
			m_Device = m_PhysicalDevice.createDevice(deviceCI);

			if (!m_Device)
			{
				LUMOS_CORE_ERROR("[VULKAN] vkCreateDevice() failed!");
				return false;
			}

			m_GraphicsQueue = m_Device.getQueue(m_GraphicsQueueFamilyIndex, 0);
			m_PresentQueue = m_Device.getQueue(m_GraphicsQueueFamilyIndex, 0);
            
            
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
            fn.vkCreateImage                       = (PFN_vkCreateImage)vkCreateImage;
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
                LUMOS_CORE_ERROR("[VULKAN] Failed to create VMA allocator");
            }
#endif

			return VK_SUCCESS;
		}

		void VKDevice::Unload()
		{
			VKDevice::Instance()->GetDevice().destroyPipelineCache(m_PipelineCache);

#ifdef USE_VMA_ALLOCATOR
			vmaDestroyAllocator(m_Allocator);
#endif

			m_Device.destroy();
			m_VKContext->GetVKInstance().destroySurfaceKHR(m_Surface);

		}

		bool VKDevice::MemoryTypeFromProperties(uint32_t typeBits, vk::MemoryPropertyFlags reqMask, uint32_t * typeIndex)
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
			vk::PipelineCacheCreateInfo pipelineCacheCI{};
			pipelineCacheCI.pNext = NULL;
			m_PipelineCache = m_Device.createPipelineCache(pipelineCacheCI);
		}
	}
}
