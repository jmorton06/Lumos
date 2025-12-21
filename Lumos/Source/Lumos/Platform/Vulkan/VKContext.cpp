#include "Precompiled.h"
#include "VKContext.h"
#include "VKDevice.h"
#include "VKCommandPool.h"
#include "VKCommandBuffer.h"
#include "VKRenderer.h"
#include "VKUtilities.h"
#include "Core/Version.h"
#include "Utilities/StringUtilities.h"
#include "Maths/MathsUtilities.h"
#include "Core/CommandLine.h"
#include "Core/CoreSystem.h"

#ifndef VK_API_VERSION_1_2
#error Wrong Vulkan SDK!
#endif

#include <imgui/imgui.h>

#define VK_LAYER_LUNARG_STANDARD_VALIDATION_NAME "VK_LAYER_LUNARG_standard_validation"
#define VK_LAYER_LUNARG_ASSISTENT_LAYER_NAME "VK_LAYER_LUNARG_assistant_layer"
#define VK_LAYER_LUNARG_VALIDATION_NAME "VK_LAYER_KHRONOS_validation"

const bool EnableValidationLayers = false;

namespace Lumos
{
    namespace Graphics
    {
        VkInstance VKContext::s_VkInstance  = nullptr;
        uint32_t VKContext::m_VKVersion     = 0;
        bool VKContext::s_ValidationEnabled = false;

        const TDArray<const char*> VKContext::GetRequiredExtensions(bool enableValidationLayers)
        {
            if(m_InstanceExtensions.Empty())
            {
                uint32_t extensionCount;
                VK_CHECK_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));

                m_InstanceExtensions.Resize(extensionCount);
                VK_CHECK_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, m_InstanceExtensions.Data()));
            }

            auto CheckExtension = [&](const char* extensionName)
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
                    LWARN("[VULKAN] Extension not supported - %s", extensionName);
                }

                return extensionFound;
            };

            TDArray<const char*> extensions;

            if(enableValidationLayers)
            {
                LINFO("Vulkan : Enabled Validation Layers");
                if(CheckExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
                    extensions.PushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                if(CheckExtension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
                    extensions.PushBack(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
            }

            if(CheckExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
                extensions.PushBack(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
            if(CheckExtension(VK_KHR_SURFACE_EXTENSION_NAME))
                extensions.PushBack(VK_KHR_SURFACE_EXTENSION_NAME);
            if(CheckExtension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
                extensions.PushBack(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

#if 0
#if defined(TRACY_ENABLE) && defined(LUMOS_PLATFORM_WINDOWS)
            if (CheckExtension("VK_EXT_calibrated_timestamps"))
                extensions.PushBack("VK_EXT_calibrated_timestamps");
#endif
#endif
            const char* platformLayerName = nullptr;
#if defined(_WIN32)
            platformLayerName = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
            platformLayerName = VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
#elif defined(_DIRECT2DISPLAY)
            platformLayerName = VK_KHR_DISPLAY_EXTENSION_NAME;
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
            platformLayerName = VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
#elif defined(VK_USE_PLATFORM_XCB_KHR)
            platformLayerName = VK_KHR_XCB_SURFACE_EXTENSION_NAME;
#elif defined(VK_USE_PLATFORM_IOS_MVK)
            platformLayerName = VK_MVK_IOS_SURFACE_EXTENSION_NAME; //"VK_EXT_metal_surface";
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
            platformLayerName = VK_MVK_MACOS_SURFACE_EXTENSION_NAME;
#elif defined(VK_USE_PLATFORM_METAL_EXT)
            platformLayerName = "VK_EXT_metal_surface";
#endif
            if(CheckExtension(platformLayerName))
                extensions.PushBack(platformLayerName);

            return extensions;
        }

        const TDArray<const char*> VKContext::GetRequiredLayers(bool enableValidationLayers)
        {
            if(m_InstanceLayers.Empty())
            {
                uint32_t layerCount;
                VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

                m_InstanceLayers.Resize(layerCount);
                VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&layerCount, m_InstanceLayers.Data()));
            }

            auto CheckLayer = [&](const char* layerName)
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
                    LWARN("[VULKAN] Layer not supported - %s", layerName);
                }

                return layerFound;
            };

            TDArray<const char*> layers;
            if(enableValidationLayers)
            {
                if(CheckLayer(VK_LAYER_LUNARG_VALIDATION_NAME))
                    layers.EmplaceBack(VK_LAYER_LUNARG_VALIDATION_NAME);
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
            VKRenderer::FlushDeletionQueues();
            VKRenderer::GetRenderer()->ReleaseDescriptorPools();

            if(m_DebugCallback)
                vkDestroyDebugReportCallbackEXT(s_VkInstance, m_DebugCallback, VK_NULL_HANDLE);

            VKDevice::Release();
            vkDestroyInstance(s_VkInstance, nullptr);
        }

        void VKContext::Init()
        {
            LUMOS_PROFILE_FUNCTION();
            CreateInstance();

            Mat4::SetUpCoordSystem(false, true);
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
                LERROR("[VULKAN] - ERROR : [%s] Code %i  : %s", pLayerPrefix, msgCode, pMsg);
            }
            // Warnings may hint at unexpected / non-spec API usage
            if(flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
            {
                LWARN("[VULKAN] - WARNING : [%s] Code %i  : %s", pLayerPrefix, msgCode, pMsg);
            }
            // May indicate sub-optimal usage of the API
            if(flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
            {
                LWARN("[VULKAN] - PERFORMANCE : [%s] Code %i : %s", pLayerPrefix, msgCode, pMsg);
            }
            // Informal messages that may become handy during debugging
            if(flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
            {
                LINFO("[VULKAN] - INFO : [%s] Code %i : %s", pLayerPrefix, msgCode, pMsg);
            }
            // Diagnostic info from the Vulkan loader and layers
            // Usually not helpful in terms of API usage, but may help to debug layer and loader problems
            if(flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
            {
                LINFO("[VULKAN] - DEBUG : [%s] Code %i : %s", pLayerPrefix, msgCode, pMsg);
            }

            return VK_FALSE;
        }

        size_t VKContext::GetMinUniformBufferOffsetAlignment() const
        {
            return Graphics::VKDevice::Get().GetPhysicalDevice()->GetProperties().limits.minUniformBufferOffsetAlignment;
        }

        void VKContext::CreateInstance()
        {
            LUMOS_PROFILE_FUNCTION();
#ifndef LUMOS_PLATFORM_IOS
            VK_CHECK_RESULT(volkInitialize());

            if(volkGetInstanceVersion() == 0)
            {
                ASSERT(false, "Could not find loader");
            }
#endif
            bool enableValidation = EnableValidationLayers;
            CommandLine* cmdline  = Internal::CoreSystem::GetCmdLine();
            if(cmdline->OptionBool(Str8Lit("EnableVulkanValidation")))
            {
                enableValidation = true;
            }

            s_ValidationEnabled = enableValidation;

            m_InstanceLayerNames     = GetRequiredLayers(enableValidation);
            m_InstanceExtensionNames = GetRequiredExtensions(enableValidation);

            VkApplicationInfo appInfo = {};
            uint32_t sdkVersion       = VK_HEADER_VERSION_COMPLETE;
            uint32_t loaderVersion    = 0;

            auto enumerateInstanceVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
                vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));

            if(enumerateInstanceVersion)
            {
                enumerateInstanceVersion(&loaderVersion);
            }
            else
            {
                loaderVersion = VK_API_VERSION_1_0;
            }

            appInfo.apiVersion = Maths::Min(sdkVersion, loaderVersion);
            m_VKVersion        = appInfo.apiVersion;

            if(sdkVersion > loaderVersion)
            {
                auto VersionToStr = [](uint32_t version) -> std::string
                {
                    return std::to_string(VK_API_VERSION_MAJOR(version)) + "." + std::to_string(VK_API_VERSION_MINOR(version)) + "." + std::to_string(VK_API_VERSION_PATCH(version));
                };

                LWARN("Using Vulkan %s. Please update your graphics drivers to support Vulkan %s.",
                      VersionToStr(loaderVersion).c_str(),
                      VersionToStr(sdkVersion).c_str());
            }

            appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName   = "Runtime";
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName        = "Lumos";
            appInfo.engineVersion      = VK_MAKE_VERSION(LumosVersion.major, LumosVersion.minor, LumosVersion.patch);

            VkInstanceCreateInfo createInfo    = {};
            createInfo.pApplicationInfo        = &appInfo;
            createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.enabledExtensionCount   = static_cast<uint32_t>(m_InstanceExtensionNames.Size());
            createInfo.ppEnabledExtensionNames = m_InstanceExtensionNames.Data();
            createInfo.enabledLayerCount       = static_cast<uint32_t>(m_InstanceLayerNames.Size());
            createInfo.ppEnabledLayerNames     = m_InstanceLayerNames.Data();
            createInfo.flags                   = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

            const bool enableFeatureValidation = false;

            if(enableFeatureValidation)
            {
                TDArray<VkValidationFeatureEnableEXT> validation_extensions = {};
                validation_extensions.EmplaceBack(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT);
                validation_extensions.EmplaceBack(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT);
                validation_extensions.EmplaceBack(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT);

                VkValidationFeaturesEXT validation_features       = {};
                validation_features.sType                         = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
                validation_features.enabledValidationFeatureCount = static_cast<uint32_t>(validation_extensions.Size());
                validation_features.pEnabledValidationFeatures    = validation_extensions.Data();
                createInfo.pNext                                  = &validation_features;
            }

            VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &s_VkInstance));
#ifndef LUMOS_PLATFORM_IOS
            volkLoadInstance(s_VkInstance);
#endif
            VKUtilities::Init();

            VKDevice::Get().Init();

            if(enableValidation)
                SetupDebugCallback();
        }

        void VKContext::SetupDebugCallback()
        {
            LUMOS_PROFILE_FUNCTION();

            VkDebugReportCallbackCreateInfoEXT createInfo = {};
            createInfo.sType                              = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
            createInfo.flags                              = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
            createInfo.pfnCallback                        = reinterpret_cast<PFN_vkDebugReportCallbackEXT>(DebugCallback);

            VK_CHECK_RESULT(CreateDebugReportCallbackEXT(s_VkInstance, &createInfo, nullptr, &m_DebugCallback));
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

        void VKContext::OnImGui()
        {
#ifdef USE_VMA_ALLOCATOR
            const auto& memoryProps = VKDevice::Get().GetPhysicalDevice()->GetMemoryProperties();
            TDArray<VmaBudget> budgets(memoryProps.memoryHeapCount);
            vmaGetHeapBudgets(VKDevice::Get().GetAllocator(), budgets.Data());

            for(VmaBudget& b : budgets)
            {
                ImGui::Text("VmaBudget.allocationBytes = %s", StringUtilities::BytesToString(b.statistics.allocationBytes).c_str());
                ImGui::Text("VmaBudget.blockBytes = %s", StringUtilities::BytesToString(b.statistics.blockBytes).c_str());
                ImGui::Text("VmaBudget.usage = %s", StringUtilities::BytesToString(b.usage).c_str());
                ImGui::Text("VmaBudget.budget = %s", StringUtilities::BytesToString(b.budget).c_str());
            }
#endif
        }
    }
}
