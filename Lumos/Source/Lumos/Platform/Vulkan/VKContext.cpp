#include "Precompiled.h"
#include "VKContext.h"
#include "VKDevice.h"
#include "VKSwapchain.h"
#include "VKCommandPool.h"
#include "VKCommandBuffer.h"
#include "Core/Version.h"

#include <imgui/imgui.h>

#define VK_LAYER_LUNARG_STANDARD_VALIDATION_NAME "VK_LAYER_LUNARG_standard_validation"
#define VK_LAYER_LUNARG_ASSISTENT_LAYER_NAME "VK_LAYER_LUNARG_assistant_layer"
#define VK_LAYER_RENDERDOC_CAPTURE_NAME "VK_LAYER_RENDERDOC_Capture"

namespace Lumos
{
    namespace Graphics
    {
        const std::vector<const char*> VKContext::GetRequiredExtensions()
        {
            std::vector<const char*> extensions;

            if(EnableValidationLayers)
            {
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }

            extensions.push_back("VK_KHR_surface");

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

            if(m_RenderDocLayer)
                layers.emplace_back(VK_LAYER_RENDERDOC_CAPTURE_NAME);

            if(m_AssistanceLayer)
                layers.emplace_back(VK_LAYER_LUNARG_ASSISTENT_LAYER_NAME);

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

        VKContext::VKContext(const WindowProperties& properties, Window* window)
            : m_VkInstance(nullptr)
        {
            m_Window = window;
            m_Width = properties.Width;
            m_Height = properties.Height;
            m_VSync = properties.VSync;
        }

        VKContext::~VKContext()
        {
            m_Swapchain.reset();
            VKDevice::Release();
            DestroyDebugReportCallbackEXT(m_VkInstance, m_DebugCallback, nullptr);
            vkDestroyInstance(m_VkInstance, nullptr);
        }

        void VKContext::Init()
        {
            LUMOS_PROFILE_FUNCTION();
            CreateInstance();

            VKDevice::Get().Init();

            m_Swapchain = CreateRef<VKSwapchain>(m_Width, m_Height);
            m_Swapchain->Init(m_VSync, m_Window);
            //m_Swapchain->AcquireNextImage();

            SetupDebugCallback();

            Maths::Matrix4::SetUpCoordSystem(false, true);
        };

        void VKContext::OnResize(uint32_t width, uint32_t height)
        {
            LUMOS_PROFILE_FUNCTION();
            m_Width = width;
            m_Height = height;

            m_Swapchain->OnResize(width, height, true, m_Window);
            //m_Swapchain->AcquireNextImage();
        }

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

        bool VKContext::CheckValidationLayerSupport(const std::vector<const char*>& validationLayers)
        {
            uint32_t layerCount;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

            m_InstanceLayers.resize(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, m_InstanceLayers.data());

            for(const char* layerName : validationLayers)
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
                    return false;
                }
            }

            return true;
        }

        bool VKContext::CheckExtensionSupport(const std::vector<const char*>& extensions)
        {
            uint32_t extensionCount;
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

            m_InstanceExtensions.resize(extensionCount);
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, m_InstanceExtensions.data());

            for(const char* extensionName : extensions)
            {
                bool layerFound = false;

                for(const auto& layerProperties : m_InstanceExtensions)
                {
                    if(strcmp(extensionName, layerProperties.extensionName) == 0)
                    {
                        layerFound = true;
                        break;
                    }
                }

                if(!layerFound)
                {
                    return false;
                }
            }

            return true;
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
            appInfo.pApplicationName = "Sandbox";
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

            if(EnableValidationLayers && !CheckValidationLayerSupport(m_InstanceLayerNames))
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

            VkResult createResult = vkCreateInstance(&createInfo, nullptr, &m_VkInstance);
            if(createResult != VK_SUCCESS)
            {
                LUMOS_LOG_CRITICAL("[VULKAN] Failed to create instance!");
            }
#ifndef LUMOS_PLATFORM_IOS
            volkLoadInstance(m_VkInstance);
#endif
        }

        void VKContext::SetupDebugCallback()
        {
            LUMOS_PROFILE_FUNCTION();
            if(!EnableValidationLayers)
                return;

            VkDebugReportCallbackCreateInfoEXT createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
            createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;

            createInfo.pfnCallback = reinterpret_cast<PFN_vkDebugReportCallbackEXT>(DebugCallback);

            VkResult result = CreateDebugReportCallbackEXT(m_VkInstance, &createInfo, nullptr, &m_DebugCallback);
            if(result != VK_SUCCESS)
            {
                LUMOS_LOG_CRITICAL("[VULKAN] Failed to set up debug callback!");
            }
        }

        void VKContext::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
        }

        GraphicsContext* VKContext::CreateFuncVulkan(const WindowProperties& properties, Window* win)
        {
            return new VKContext(properties, win);
        }

        void VKContext::WaitIdle() const
        {
            vkDeviceWaitIdle(VKDevice::Get().GetDevice());
        }

        float VKContext::GetGPUMemoryUsed()
        {
#ifdef USE_VMA_ALLOCATOR
            VmaAllocator allocator = VKDevice::Get().GetAllocator();
            VmaStats stats;
            vmaCalculateStats(allocator, &stats);
            auto info = stats.total;
            return static_cast<float>(info.usedBytes);
#else
            return 0.0f;
#endif
        }

        float VKContext::GetTotalGPUMemory()
        {
#ifdef USE_VMA_ALLOCATOR
            VmaAllocator allocator = VKDevice::Get().GetAllocator();
            VmaStats stats;
            vmaCalculateStats(allocator, &stats);
            auto info = stats.total;
            return static_cast<float>(info.unusedBytes + info.usedBytes);
#else
            return 0.0f;
#endif
        }

        //Debug ImGui Windows
        static auto const readOnlyFlag = ImGuiInputTextFlags_ReadOnly;

        void VKContext::OnImGui()
        {
            LUMOS_PROFILE_FUNCTION();
            ImGui::TextUnformatted("Vulkan Info");

            if(ImGui::TreeNode("Instance"))
            {

                ImGui::TextUnformatted("Extensions :");

                auto globalExtensions = m_InstanceExtensions;
                for(auto const& extension : globalExtensions)
                    ImGui::BulletText("%s (%d)", extension.extensionName, VK_VERSION_PATCH(extension.specVersion));

                ImGui::TreePop();
            }

            if(ImGui::TreeNode("Layers"))
            {
                auto layerProperties = m_InstanceLayers;

                if(layerProperties.empty())
                    ImGui::TextUnformatted("No Layers");

                for(auto const& layer : layerProperties)
                {
                    if(ImGui::TreeNode(layer.layerName))
                    {
                        ImGui::BulletText("Description : %s", layer.description);
                        ImGui::BulletText("Version : %u/%d", layer.specVersion, layer.implementationVersion);

                        ImGui::TreePop();
                    }
                }

                ImGui::TreePop();
            }

            if(ImGui::TreeNode("Physical Devices"))
            {
                auto physicalDevice = VKDevice::Get().GetGPU();
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(physicalDevice, &properties);

                if(ImGui::TreeNode(properties.deviceName))
                {

                    ImGui::BulletText("API Version : %u - Driver Version : %u",
                        properties.apiVersion,
                        properties.driverVersion);
                    ImGui::BulletText("VendorID : %u - DeviceID : %u", properties.vendorID, properties.deviceID);

                    uint32_t extensionCount = 0;

                    vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extensionCount, NULL);

                    std::vector<VkExtensionProperties> extensionProperties;

                    if(extensionCount > 0)
                    {
                        extensionProperties.resize(extensionCount);
                        vkEnumerateDeviceExtensionProperties(
                            physicalDevice, NULL, &extensionCount, extensionProperties.data());
                    }

                    if(ImGui::TreeNode("##extensionProperties", "Extension Properties (%lu)", extensionProperties.size()))
                    {

                        for(auto const& extensionProperty : extensionProperties)
                            ImGui::BulletText("%s (%d)", extensionProperty.extensionName, VK_VERSION_PATCH(extensionProperty.specVersion));

                        ImGui::TreePop();
                    }
                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }

            ImGui::NewLine();

#ifdef USE_VMA_ALLOCATOR
            VmaAllocator allocator = VKDevice::Get().GetAllocator();

            VmaStats stats;
            vmaCalculateStats(allocator, &stats);

            if(ImGui::CollapsingHeader("Total"))
            {
                DebugDrawVmaMemory(stats.total);
            }

            VkPhysicalDeviceMemoryProperties const* memoryProperties = nullptr;
            vmaGetMemoryProperties(allocator, &memoryProperties);

            for(auto heapIndex = 0u; heapIndex < memoryProperties->memoryHeapCount; ++heapIndex)
            {
                if(stats.memoryHeap[heapIndex].blockCount == 0)
                    continue;

                std::string heapName = fmt::format("Heap {} | Size: {}", heapIndex, memoryProperties->memoryHeaps[heapIndex].size);

                if((memoryProperties->memoryHeaps[heapIndex].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0)
                {
                    heapName += " | DEVICE_LOCAL";
                }

                if(ImGui::CollapsingHeader(heapName.c_str()))
                {
                    DebugDrawVmaMemory(stats.memoryHeap[heapIndex]);
                }

                for(auto typeIndex = 0u; typeIndex < memoryProperties->memoryTypeCount; ++typeIndex)
                {
                    if(memoryProperties->memoryTypes[typeIndex].heapIndex != heapIndex)
                        continue;

                    if(stats.memoryType[typeIndex].blockCount == 0)
                        continue;

                    std::string typeName = fmt::format("Type {}", typeIndex);

                    VkMemoryPropertyFlags propertyFlags = memoryProperties->memoryTypes[typeIndex].propertyFlags;

                    std::string flags;

                    if((propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0)
                    {
                        flags += "DEVICE_LOCAL ";
                    }
                    if((propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0)
                    {
                        flags += "HOST_VISIBLE ";
                    }
                    if((propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0)
                    {
                        flags += "HOST_COHERENT ";
                    }
                    if((propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) != 0)
                    {
                        flags += "HOST_CACHED ";
                    }
                    if((propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) != 0)
                    {
                        flags += "LAZILY_ALLOCATED ";
                    }

                    if(!flags.empty())
                        typeName += " | " + flags;

                    if(ImGui::CollapsingHeader(typeName.c_str()))
                    {
                        DebugDrawVmaMemory(stats.memoryType[typeIndex]);
                    }
                }
            }
#endif
        }

#ifdef USE_VMA_ALLOCATOR
        void VKContext::DebugDrawVmaMemory(VmaStatInfo& info, bool indent)
        {
            LUMOS_PROFILE_FUNCTION();
            if(indent)
                ImGui::Indent();

            float bytesProgress = static_cast<float>(info.usedBytes) / static_cast<float>((info.unusedBytes + info.usedBytes));
            ImGui::ProgressBar(bytesProgress, ImVec2(0.0f, 0.0f));
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
            ImGui::TextUnformatted("Used");

            ImGui::InputScalar("Blocks", ImGuiDataType_U32, (void*)&info.blockCount, nullptr, nullptr, "%u", readOnlyFlag);

            ImGui::InputScalar("Allocations", ImGuiDataType_U32, (void*)&info.allocationCount, nullptr, nullptr, "%u", readOnlyFlag);

            ImGui::InputScalar("UnusedRanges", ImGuiDataType_U32, (void*)&info.unusedRangeCount, nullptr, nullptr, "%u", readOnlyFlag);

            ImGui::InputScalar("UsedBytes", ImGuiDataType_U32, (void*)&info.usedBytes, nullptr, nullptr, "%u", readOnlyFlag);

            ImGui::InputScalar("UnusedBytes", ImGuiDataType_U32, (void*)&info.unusedBytes, nullptr, nullptr, "%u", readOnlyFlag);

            ImGui::TextUnformatted("AllocationSize");

            ImGui::InputScalar("Min", ImGuiDataType_U32, (void*)&info.allocationSizeMin, nullptr, nullptr, "%u", readOnlyFlag);

            ImGui::InputScalar("Avg", ImGuiDataType_U32, (void*)&info.allocationSizeAvg, nullptr, nullptr, "%u", readOnlyFlag);

            ImGui::InputScalar("Max", ImGuiDataType_U32, (void*)&info.allocationSizeMax, nullptr, nullptr, "%u", readOnlyFlag);

            ImGui::TextUnformatted("UnusedRangeSize");

            ImGui::InputScalar("Min", ImGuiDataType_U32, (void*)&info.unusedRangeSizeMin, nullptr, nullptr, "%u", readOnlyFlag);

            ImGui::InputScalar("Avg", ImGuiDataType_U32, (void*)&info.unusedRangeSizeAvg, nullptr, nullptr, "%u", readOnlyFlag);

            ImGui::InputScalar("Max", ImGuiDataType_U32, (void*)&info.unusedRangeSizeMax, nullptr, nullptr, "%u", readOnlyFlag);

            if(indent)
                ImGui::Unindent();
        }
#endif
    }
}
