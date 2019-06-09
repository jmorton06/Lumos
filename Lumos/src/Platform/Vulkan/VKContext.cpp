#include "LM.h"
#include "VKContext.h"
#include "VKDevice.h"
#include "VKCommandPool.h"
#include "VKCommandBuffer.h"
#include "Maths/Matrix4.h"

namespace Lumos
{
	namespace Graphics
	{
		std::vector<const char*> GetRequiredExtensions()
		{
			std::vector<const char*> extensions;

			if (EnableValidationLayers)
			{
				extensions.push_back("VK_EXT_debug_report");
				extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}

			extensions.push_back("VK_KHR_surface");

	#if defined(_WIN32)
			 extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
	#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
			 extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
	#elif defined(_DIRECT2DISPLAY)
			 extensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
	#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
			 extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
	#elif defined(VK_USE_PLATFORM_XCB_KHR)
			 extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
	#elif defined(VK_USE_PLATFORM_IOS_MVK)
			 extensions.push_back(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
	#elif defined(VK_USE_PLATFORM_MACOS_MVK)
			 extensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
	#endif

			return extensions;
		}

		VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
		{
			auto func = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));

			if (func != nullptr)
			{
				return func(instance, pCreateInfo, pAllocator, pCallback);
			}
			else
			{
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		}

		void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator)
		{
			auto func = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(
				instance, "vkDestroyDebugReportCallbackEXT"));

			if (func != nullptr)
			{
				func(instance, callback, pAllocator);
			}
		}

        VKContext::VKContext(const WindowProperties& properties, void* deviceContext) : m_CommandPool(nullptr)
		{
			m_WindowContext = deviceContext;
			CreateInstance();
			SetupDebugCallback();

			Maths::Matrix4::SetUpCoordSystem(false, true);
		}

		VKContext::~VKContext()
		{
			delete m_CommandPool;
            
			DestroyDebugReportCallbackEXT(m_VkInstance, callback, nullptr);
			vkDestroyInstance(m_VkInstance, nullptr);
		}

		void VKContext::Init()
		{
            if(m_CommandPool != nullptr)
                delete m_CommandPool;
            
			m_CommandPool = new VKCommandPool();
		};

		void VKContext::Present()
		{

		}

		void VKContext::Unload()
		{
			delete m_CommandPool;
			m_CommandPool = nullptr;
		}

		VkBool32 VKContext::DebugCallback(vk::DebugReportFlagsEXT flags, vk::DebugReportObjectTypeEXT objType, uint64_t obj,
		                                  size_t location, int32_t code, const char* layerPrefix, const char* msg,
		                                  void* userData)
		{
			// Select prefix depending on flags passed to the callback
			// Note that multiple flags may be set for a single validation message
			// Error that may result in undefined behaviour

			LUMOS_CORE_WARN("[VULKAN] : [{0}] Code {1}  : {2}", layerPrefix, code, msg);

			if (flags & vk::DebugReportFlagBitsEXT::eError)
			{
				LUMOS_CORE_WARN("[VULKAN] - ERROR : [{0}] Code {1}  : {2}", layerPrefix, code, msg);
			};
			// Warnings may hint at unexpected / non-spec API usage
			if (flags & vk::DebugReportFlagBitsEXT::eWarning)
			{
				LUMOS_CORE_WARN("[VULKAN] - WARNING : [{0}] Code {1}  : {2}", layerPrefix, code, msg);
			};
			// May indicate sub-optimal usage of the API
			if (flags & vk::DebugReportFlagBitsEXT::ePerformanceWarning)
			{
				LUMOS_CORE_INFO("[VULKAN] - PERFORMANCE : [{0}] Code {1}  : {2}", layerPrefix, code, msg);
			};
			// Informal messages that may become handy during debugging
			if (flags & vk::DebugReportFlagBitsEXT::eInformation)
			{
				LUMOS_CORE_INFO("[VULKAN] - INFO : [{0}] Code {1}  : {2}", layerPrefix, code, msg);
			}
			// Diagnostic info from the Vulkan loader and layers
			// Usually not helpful in terms of API usage, but may help to debug layer and loader problems 
			if (flags & vk::DebugReportFlagBitsEXT::eDebug)
			{
				LUMOS_CORE_INFO("[VULKAN] - DEBUG : [{0}] Code {1}  : {2}", layerPrefix, code, msg);
			}

			return VK_FALSE;
		}

		bool CheckValidationLayerSupport()
		{
			uint32_t layerCount;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

			std::vector<vk::LayerProperties> availableLayers(layerCount);
			vk::enumerateInstanceLayerProperties(&layerCount, availableLayers.data());

			for (const char* layerName : validationLayers)
			{
				bool layerFound = false;

				for (const auto& layerProperties : availableLayers)
				{
					if (strcmp(layerName, layerProperties.layerName) == 0)
					{
						layerFound = true;
						break;
					}
				}

				if (!layerFound)
				{
					return false;
				}
			}

			return true;
		}

		size_t VKContext::GetMinUniformBufferOffsetAlignment() const
		{
			return Graphics::VKDevice::Instance()->GetGPUProperties().limits.minUniformBufferOffsetAlignment;
		}

		void VKContext::CreateInstance()
		{
			if (volkInitialize() != VK_SUCCESS)
			{
				LUMOS_CORE_ERROR("volkInitialize failed");
			}

			if (volkGetInstanceVersion() == 0)
			{
				LUMOS_CORE_ERROR("Could not find loader");
			}

			if (EnableValidationLayers && !CheckValidationLayerSupport())
			{
				throw std::runtime_error("validation layers requested, but not available!");
			}

			vk::ApplicationInfo appInfo = {};
			appInfo.pApplicationName = "Sandbox";
			appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
			appInfo.pEngineName = "Lumos";
			appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
			appInfo.apiVersion = VK_API_VERSION_1_0;

			vk::InstanceCreateInfo createInfo = {};
			createInfo.pApplicationInfo = &appInfo;

			auto extensions = GetRequiredExtensions();
			createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
			createInfo.ppEnabledExtensionNames = extensions.data();

			if (EnableValidationLayers)
			{
				createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
				createInfo.ppEnabledLayerNames = validationLayers.data();
			}
			else
			{
				createInfo.enabledLayerCount = 0;
			}

			m_VkInstance = vk::createInstance(createInfo, nullptr);
			if (!m_VkInstance)
			{
				LUMOS_CORE_ERROR("failed to create instance!");
			}

			volkLoadInstance(m_VkInstance);
		}

		void VKContext::SetupDebugCallback()
		{
			if (!EnableValidationLayers) return;

			vk::DebugReportCallbackCreateInfoEXT createInfo = {};
			createInfo.flags =	vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning |
								vk::DebugReportFlagBitsEXT::eInformation | vk::DebugReportFlagBitsEXT::eDebug | vk::DebugReportFlagBitsEXT::ePerformanceWarning;

			createInfo.pfnCallback = reinterpret_cast<PFN_vkDebugReportCallbackEXT>(DebugCallback);

			callback = m_VkInstance.createDebugReportCallbackEXT(createInfo);
			if (!callback)
			{
				throw std::runtime_error("failed to set up debug callback!");
			}
		}
	}
}
