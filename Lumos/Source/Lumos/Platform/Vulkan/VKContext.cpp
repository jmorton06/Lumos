#include "Precompiled.h"
#include "VKContext.h"
#include "VKDevice.h"
#include "VKCommandPool.h"
#include "VKCommandBuffer.h"
#include "VKRenderer.h"
#include "Core/Version.h"
#include "Core/StringUtilities.h"

#include <imgui/imgui.h>

#define VK_LAYER_LUNARG_STANDARD_VALIDATION_NAME "VK_LAYER_LUNARG_standard_validation"
#define VK_LAYER_LUNARG_ASSISTENT_LAYER_NAME "VK_LAYER_LUNARG_assistant_layer"
#define VK_LAYER_LUNARG_VALIDATION_NAME "VK_LAYER_KHRONOS_validation"

namespace Lumos
{
    namespace Graphics
    {
        VkInstance VKContext::s_VkInstance = nullptr;
        const std::vector<const char*> VKContext::GetRequiredExtensions()
        {
            std::vector<const char*> extensions;

            if(EnableValidationLayers)
            {
                LUMOS_LOG_INFO("Vulkan : Enabled Validation Layers");
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
                extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
            }

            extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#if 0
#if defined(TRACY_ENABLE) && defined(LUMOS_PLATFORM_WINDOWS)
            extensions.push_back("VK_EXT_calibrated_timestamps");
#endif
#endif

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
            extensions.push_back("VK_EXT_metal_surface");
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
            extensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_METAL_EXT)
            extensions.push_back("VK_EXT_metal_surface");
#endif

            return extensions;
        }

        const std::vector<const char*> VKContext::GetRequiredLayers() const
        {
            std::vector<const char*> layers;

            if(m_StandardValidationLayer)
                layers.emplace_back(VK_LAYER_LUNARG_STANDARD_VALIDATION_NAME);

            if(m_AssistanceLayer)
                layers.emplace_back(VK_LAYER_LUNARG_ASSISTENT_LAYER_NAME);

            if(EnableValidationLayers)
            {
                layers.emplace_back(VK_LAYER_LUNARG_VALIDATION_NAME);
            }

            return layers;
        }

        VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
        {
            auto func = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));

            if(func != nullptr)
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

            if(func != nullptr)
            {
                func(instance, callback, pAllocator);
            }
        }

        VKContext::VKContext()
        {
        }

        VKContext::~VKContext()
        {
            for(int i = 0; i < 3; i++)
            {
                VKRenderer::GetDeletionQueue(i).Flush();
            }

            vkDestroyDescriptorPool(VKDevice::Get().GetDevice(), VKRenderer::GetDescriptorPool(), VK_NULL_HANDLE);

            if(m_DebugCallback)
                vkDestroyDebugReportCallbackEXT(s_VkInstance, m_DebugCallback, VK_NULL_HANDLE);

            VKDevice::Release();
            vkDestroyInstance(s_VkInstance, nullptr);
        }

        void VKContext::Init()
        {
            LUMOS_PROFILE_FUNCTION();
            CreateInstance();
            VKDevice::Get().Init();

            SetupDebugCallback();
        };

        void VKContext::Present()
        {
        }

        VkBool32 VKContext::DebugCallback(VkDebugReportFlagsEXT flags,
            VkDebugReportObjectTypeEXT objType,
            uint64_t sourceObj,
            size_t location,
            int32_t msgCode,
            const char* pLayerPrefix,
            const char* pMsg,
            void* userData)
        {
            // Select prefix depending on flags passed to the callback
            // Note that multiple flags may be set for a single validation message
            // Error that may result in undefined behaviour

            if(!flags)
                return VK_FALSE;

            if(flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
            {
                LUMOS_LOG_WARN("[VULKAN] - ERROR : [{0}] Code {1}  : {2}", pLayerPrefix, msgCode, pMsg);
            };
            // Warnings may hint at unexpected / non-spec API usage
            if(flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
            {
                LUMOS_LOG_WARN("[VULKAN] - WARNING : [{0}] Code {1}  : {2}", pLayerPrefix, msgCode, pMsg);
            };
            // May indicate sub-optimal usage of the API
            if(flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
            {
                LUMOS_LOG_WARN("[VULKAN] - PERFORMANCE : [{0}] Code {1}  : {2}", pLayerPrefix, msgCode, pMsg);
            };
            // Informal messages that may become handy during debugging
            if(flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
            {
                LUMOS_LOG_WARN("[VULKAN] - INFO : [{0}] Code {1}  : {2}", pLayerPrefix, msgCode, pMsg);
            }
            // Diagnostic info from the Vulkan loader and layers
            // Usually not helpful in terms of API usage, but may help to debug layer and loader problems
            if(flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
            {
                LUMOS_LOG_INFO("[VULKAN] - DEBUG : [{0}] Code {1}  : {2}", pLayerPrefix, msgCode, pMsg);
            }

            return VK_FALSE;
        }

        bool VKContext::CheckValidationLayerSupport(std::vector<const char*>& validationLayers)
        {
            uint32_t layerCount;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

            m_InstanceLayers.resize(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, m_InstanceLayers.data());
            bool removedLayer = false;

            validationLayers.erase(
                std::remove_if(
                    validationLayers.begin(),
                    validationLayers.end(),
                    [&](const char* layerName)
                    {
                        bool layerFound = false;

                        for(const auto& layerProperties : m_InstanceLayers)
                        {
                            if(strcmp(layerName, layerProperties.layerName) == 0)
                            {
                                layerFound = true;
                                break;
                            }
                        }

                        if(!layerFound)
                        {
                            removedLayer = true;
                            LUMOS_LOG_WARN("[VULKAN] Layer not supported - {0}", layerName);
                        }

                        return !layerFound;
                    }),
                validationLayers.end());

            return !removedLayer;
        }

        bool VKContext::CheckExtensionSupport(std::vector<const char*>& extensions)
        {
            uint32_t extensionCount;
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

            m_InstanceExtensions.resize(extensionCount);
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, m_InstanceExtensions.data());

            bool removedExtension = false;

            extensions.erase(
                std::remove_if(
                    extensions.begin(),
                    extensions.end(),
                    [&](const char* extensionName)
                    {
                        bool extensionFound = false;

                        for(const auto& extensionProperties : m_InstanceExtensions)
                        {
                            if(strcmp(extensionName, extensionProperties.extensionName) == 0)
                            {
                                extensionFound = true;
                                break;
                            }
                        }

                        if(!extensionFound)
                        {
                            removedExtension = true;
                            LUMOS_LOG_WARN("[VULKAN] Extension not supported - {0}", extensionName);
                        }

                        return !extensionFound;
                    }),
                extensions.end());

            return !removedExtension;
        }

        size_t VKContext::GetMinUniformBufferOffsetAlignment() const
        {
            return Graphics::VKDevice::Get().GetPhysicalDevice()->GetProperties().limits.minUniformBufferOffsetAlignment;
        }

        void VKContext::CreateInstance()
        {
            LUMOS_PROFILE_FUNCTION();
#ifndef LUMOS_PLATFORM_IOS
            if(volkInitialize() != VK_SUCCESS)
            {
                LUMOS_LOG_CRITICAL("volkInitialize failed");
            }

            if(volkGetInstanceVersion() == 0)
            {
                LUMOS_LOG_CRITICAL("Could not find loader");
            }
#endif

            VkApplicationInfo appInfo = {};
            appInfo.pApplicationName = "Runtime";
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = "Lumos";
            appInfo.engineVersion = VK_MAKE_VERSION(LumosVersion.major, LumosVersion.minor, LumosVersion.patch);
#if defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_IOS)
            appInfo.apiVersion = VK_API_VERSION_1_1;
#else
            appInfo.apiVersion = VK_API_VERSION_1_1;
#endif

            VkInstanceCreateInfo createInfo = {};
            createInfo.pApplicationInfo = &appInfo;

            m_InstanceLayerNames = GetRequiredLayers();
            m_InstanceExtensionNames = GetRequiredExtensions();

            if(!CheckValidationLayerSupport(m_InstanceLayerNames))
            {
                LUMOS_LOG_CRITICAL("[VULKAN] Validation layers requested, but not available!");
            }

            if(!CheckExtensionSupport(m_InstanceExtensionNames))
            {
                LUMOS_LOG_CRITICAL("[VULKAN] Extensions requested are not available!");
            }

            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

            createInfo.enabledExtensionCount = static_cast<uint32_t>(m_InstanceExtensionNames.size());
            createInfo.ppEnabledExtensionNames = m_InstanceExtensionNames.data();

            createInfo.enabledLayerCount = static_cast<uint32_t>(m_InstanceLayerNames.size());
            createInfo.ppEnabledLayerNames = m_InstanceLayerNames.data();

            VkResult createResult = vkCreateInstance(&createInfo, nullptr, &s_VkInstance);
            if(createResult != VK_SUCCESS)
            {
                LUMOS_LOG_CRITICAL("[VULKAN] Failed to create instance!");
            }
#ifndef LUMOS_PLATFORM_IOS
            volkLoadInstance(s_VkInstance);
#endif
        }

        void VKContext::SetupDebugCallback()
        {
            LUMOS_PROFILE_FUNCTION();
            if(!EnableValidationLayers)
                return;

            VkDebugReportCallbackCreateInfoEXT createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
            createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;

            createInfo.pfnCallback = reinterpret_cast<PFN_vkDebugReportCallbackEXT>(DebugCallback);

            VkResult result = CreateDebugReportCallbackEXT(s_VkInstance, &createInfo, nullptr, &m_DebugCallback);
            if(result != VK_SUCCESS)
            {
                LUMOS_LOG_CRITICAL("[VULKAN] Failed to set up debug callback!");
            }
        }

        void VKContext::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
        }

        GraphicsContext* VKContext::CreateFuncVulkan()
        {
            return new VKContext();
        }

        void VKContext::WaitIdle() const
        {
            vkDeviceWaitIdle(VKDevice::Get().GetDevice());
        }
    }
}
