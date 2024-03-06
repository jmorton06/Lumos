#include "Precompiled.h"

#include "Core/Application.h"
#include "Core/Version.h"
#include "Utilities/StringUtilities.h"
#include "VKUtilities.h"
#include "VKDevice.h"
#include "VKRenderer.h"
#include "VKCommandPool.h"

namespace Lumos
{
    namespace Graphics
    {
        std::string VKPhysicalDevice::PhysicalDeviceInfo::GetVendorName()
        {
            std::string name = "Unknown";

            if(VendorID == 0x10DE || StringUtilities::StringContains(Name, "Nvidia"))
            {
                name = "Nvidia";
            }
            else if(VendorID == 0x1002 || VendorID == 0x1022 || StringUtilities::StringContains(Name, "Amd"))
            {
                name = "AMD";
            }
            else if(VendorID == 0x8086 || VendorID == 0x163C || VendorID == 0x8087 || StringUtilities::StringContains(Name, "Intel"))
            {
                name = "Intel";
            }
            else if(VendorID == 0x13B5 || StringUtilities::StringContains(Name, "Arm,"))
            {
                name = "Arm";
            }
            else if(VendorID == 0x5143 || StringUtilities::StringContains(Name, "Qualcomm"))
            {
                name = "Qualcomm";
            }
            else if(VendorID == 0x106b || StringUtilities::StringContains(Name, "Apple"))
            {
                return "Apple";
            }

            return name;
        }

        std::string VKPhysicalDevice::PhysicalDeviceInfo::DecodeDriverVersion(const uint32_t version)
        {
            char buffer[256];

            if(Vendor == "Nvidia")
            {
                sprintf(
                    buffer,
                    "%d.%d.%d.%d",
                    (version >> 22) & 0x3ff,
                    (version >> 14) & 0x0ff,
                    (version >> 6) & 0x0ff,
                    (version) & 0x003f);
            }
#if LUMOS_PLATFORM_WINDOWS
            else if(Vendor == "Intel")
            {
                sprintf(
                    buffer,
                    "%d.%d",
                    (version >> 14),
                    (version) & 0x3fff);
            }
#endif
            else // Vulkan version conventions
            {
                sprintf(
                    buffer,
                    "%d.%d.%d",
                    (version >> 22),
                    (version >> 12) & 0x3ff,
                    version & 0xfff);
            }

            return buffer;
        }

        const char* PhysicalDeviceTypeToString(PhysicalDeviceType type)
        {
            switch(type)
            {
            case PhysicalDeviceType::UNKNOWN:
                return "OTHER";
            case PhysicalDeviceType::INTEGRATED:
                return "INTEGRATED GPU";
            case PhysicalDeviceType::DISCRETE:
                return "DISCRETE GPU";
            case PhysicalDeviceType::VIRTUAL:
                return "VIRTUAL GPU";
            case PhysicalDeviceType::CPU:
                return "CPU";
            default:
                return "UNKNOWN";
            }
        }

        VKPhysicalDevice::VKPhysicalDevice()
        {
            // GPU
            auto vkInstance = VKContext::GetVKInstance();
            vkEnumeratePhysicalDevices(vkInstance, &m_GPUCount, VK_NULL_HANDLE);
            if(m_GPUCount == 0)
            {
                LUMOS_LOG_CRITICAL("No GPUs found!");
            }

            std::vector<VkPhysicalDevice> physicalDevices(m_GPUCount);
            vkEnumeratePhysicalDevices(vkInstance, &m_GPUCount, physicalDevices.data());
            // First select the gpu at the back of the list
            m_Handle = physicalDevices.back();

            int8_t desiredGPUIndex = Application::Get().GetProjectSettings().DesiredGPUIndex;

            if(desiredGPUIndex >= 0)
            {
                // If index is greater than 0 try and use it
                if(desiredGPUIndex >= (int8_t)m_GPUCount)
                {
                    LUMOS_LOG_CRITICAL("GPU index greater than GPU count!");
                }
                else
                {
                    m_Handle     = physicalDevices[desiredGPUIndex];
                    m_DeviceInfo = GetInfo(m_Handle);
                }
            }
            else
            {
                std::vector<PhysicalDeviceInfo> deviceInfos;
                for(VkPhysicalDevice physicalDevice : physicalDevices)
                {
                    deviceInfos.push_back(GetInfo(physicalDevice));
                }

                std::sort(deviceInfos.begin(), deviceInfos.end(), [](PhysicalDeviceInfo& deviceA, PhysicalDeviceInfo& deviceB)
                          {
                    if(deviceA.Type == deviceB.Type)
                        return deviceA.Memory > deviceB.Memory;
                    
                    return deviceA.Type < deviceB.Type; });

                m_Handle     = deviceInfos[0].Handle;
                m_DeviceInfo = deviceInfos[0];
            }

            vkGetPhysicalDeviceProperties(m_Handle, &m_PhysicalDeviceProperties);
            vkGetPhysicalDeviceMemoryProperties(m_Handle, &m_MemoryProperties);

            LUMOS_LOG_INFO("Vulkan : {0}.{1}.{2}", VK_API_VERSION_MAJOR(m_PhysicalDeviceProperties.apiVersion), VK_API_VERSION_MINOR(m_PhysicalDeviceProperties.apiVersion), VK_API_VERSION_PATCH(m_PhysicalDeviceProperties.apiVersion));
            LUMOS_LOG_INFO("GPU : {0}", m_DeviceInfo.Name);
            LUMOS_LOG_INFO("Memory : {0} mb", StringUtilities::ToString(m_DeviceInfo.Memory));
            LUMOS_LOG_INFO("Vendor : {0}", m_DeviceInfo.Vendor);
            LUMOS_LOG_INFO("Vendor ID : {0}", StringUtilities::ToString(m_DeviceInfo.VendorID));
            LUMOS_LOG_INFO("Device Type : {0}", std::string(PhysicalDeviceTypeToString(m_DeviceInfo.Type)));
            LUMOS_LOG_INFO("Driver Version : {0}", m_DeviceInfo.Driver);
            LUMOS_LOG_INFO("APi Version : {0}", m_DeviceInfo.APIVersion);

            auto& caps                        = Renderer::GetCapabilities();
            caps.Vendor                       = m_DeviceInfo.Vendor;
            caps.Renderer                     = std::string(m_PhysicalDeviceProperties.deviceName);
            caps.Version                      = StringUtilities::ToString(m_PhysicalDeviceProperties.driverVersion);
            caps.MaxAnisotropy                = m_PhysicalDeviceProperties.limits.maxSamplerAnisotropy;
            caps.MaxSamples                   = m_PhysicalDeviceProperties.limits.maxSamplerAllocationCount;
            caps.MaxTextureUnits              = m_PhysicalDeviceProperties.limits.maxDescriptorSetSamplers;
            caps.UniformBufferOffsetAlignment = int(m_PhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment);
            caps.SupportCompute               = false; // true; //Need to sort descriptor set management first

            uint32_t queueFamilyCount;
            vkGetPhysicalDeviceQueueFamilyProperties(m_Handle, &queueFamilyCount, nullptr);
            LUMOS_ASSERT(queueFamilyCount > 0);
            m_QueueFamilyProperties.resize(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(m_Handle, &queueFamilyCount, m_QueueFamilyProperties.data());

            uint32_t extCount = 0;
            vkEnumerateDeviceExtensionProperties(m_Handle, nullptr, &extCount, nullptr);
            if(extCount > 0)
            {
                std::vector<VkExtensionProperties> extensions(extCount);
                if(vkEnumerateDeviceExtensionProperties(m_Handle, nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
                {
                    LUMOS_LOG_INFO("Selected physical device has {0} extensions", extensions.size());
                    for(const auto& ext : extensions)
                    {
                        m_SupportedExtensions.emplace(ext.extensionName);
                    }
                }
            }

            // Queue families
            // Desired queues need to be requested upon logical device creation
            // Due to differing queue family configurations of Vulkan implementations this can be a bit tricky, especially if the application
            // requests different queue types

            // Get queue family indices for the requested queue family types
            // Note that the indices may overlap depending on the implementation

            static const float defaultQueuePriority(0.0f);

            int requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
            m_QueueFamilyIndices    = GetQueueFamilyIndices(requestedQueueTypes);

            // Graphics queue
            if(requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
            {
                VkDeviceQueueCreateInfo queueInfo = {};
                queueInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueInfo.queueFamilyIndex        = m_QueueFamilyIndices.Graphics;
                queueInfo.queueCount              = 1;
                queueInfo.pQueuePriorities        = &defaultQueuePriority;
                m_QueueCreateInfos.push_back(queueInfo);
            }

            // Dedicated compute queue
            if(requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
            {
                if(m_QueueFamilyIndices.Compute != m_QueueFamilyIndices.Graphics)
                {
                    // If compute family index differs, we need an additional queue create info for the compute queue
                    VkDeviceQueueCreateInfo queueInfo = {};
                    queueInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                    queueInfo.queueFamilyIndex        = m_QueueFamilyIndices.Compute;
                    queueInfo.queueCount              = 1;
                    queueInfo.pQueuePriorities        = &defaultQueuePriority;
                    m_QueueCreateInfos.push_back(queueInfo);
                }
            }

            // Dedicated transfer queue
            if(requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
            {
                if((m_QueueFamilyIndices.Transfer != m_QueueFamilyIndices.Graphics) && (m_QueueFamilyIndices.Transfer != m_QueueFamilyIndices.Compute))
                {
                    // If compute family index differs, we need an additional queue create info for the compute queue
                    VkDeviceQueueCreateInfo queueInfo {};
                    queueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                    queueInfo.queueFamilyIndex = m_QueueFamilyIndices.Transfer;
                    queueInfo.queueCount       = 1;
                    queueInfo.pQueuePriorities = &defaultQueuePriority;
                    m_QueueCreateInfos.push_back(queueInfo);
                }
            }
        }

        VKPhysicalDevice::~VKPhysicalDevice()
        {
        }

        bool VKPhysicalDevice::IsExtensionSupported(const std::string& extensionName) const
        {
            return m_SupportedExtensions.find(extensionName) != m_SupportedExtensions.end();
        }

        VKPhysicalDevice::QueueFamilyIndices VKPhysicalDevice::GetQueueFamilyIndices(int flags)
        {
            QueueFamilyIndices indices;

            // Dedicated queue for compute
            // Try to find a queue family index that supports compute but not graphics
            if(flags & VK_QUEUE_COMPUTE_BIT)
            {
                for(uint32_t i = 0; i < m_QueueFamilyProperties.size(); i++)
                {
                    auto& queueFamilyProperties = m_QueueFamilyProperties[i];
                    if((queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) && ((queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
                    {
                        indices.Compute = i;
                        break;
                    }
                }
            }

            // Dedicated queue for transfer
            // Try to find a queue family index that supports transfer but not graphics and compute
            if(flags & VK_QUEUE_TRANSFER_BIT)
            {
                for(uint32_t i = 0; i < m_QueueFamilyProperties.size(); i++)
                {
                    auto& queueFamilyProperties = m_QueueFamilyProperties[i];
                    if((queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) && ((queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
                    {
                        indices.Transfer = i;
                        break;
                    }
                }
            }

            // For other queue types or if no separate compute queue is present, return the first one to support the requested flags
            for(uint32_t i = 0; i < m_QueueFamilyProperties.size(); i++)
            {
                if((flags & VK_QUEUE_TRANSFER_BIT) && indices.Transfer == -1)
                {
                    if(m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
                        indices.Transfer = i;
                }

                if((flags & VK_QUEUE_COMPUTE_BIT) && indices.Compute == -1)
                {
                    if(m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
                        indices.Compute = i;
                }

                if(flags & VK_QUEUE_GRAPHICS_BIT)
                {
                    if(m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                        indices.Graphics = i;
                }
            }

            return indices;
        }

        VKPhysicalDevice::PhysicalDeviceInfo VKPhysicalDevice::GetInfo(VkPhysicalDevice device)
        {
            VkPhysicalDeviceProperties properties = {};
            vkGetPhysicalDeviceProperties(device, &properties);

            VkPhysicalDeviceMemoryProperties memoryProperties = {};
            vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);

            PhysicalDeviceType type = PhysicalDeviceType::UNKNOWN;
            if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                type = PhysicalDeviceType::DISCRETE;
            else if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
                type = PhysicalDeviceType::INTEGRATED;
            else if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
                type = PhysicalDeviceType::VIRTUAL;
            else if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
                type = PhysicalDeviceType::CPU;

            uint64_t memory   = static_cast<uint64_t>(memoryProperties.memoryHeaps[0].size);
            uint32_t memoryMB = static_cast<uint32_t>(memory / 1024 / 1024);

            VKPhysicalDevice::PhysicalDeviceInfo info = {};
            info.Name                                 = std::string(properties.deviceName);
            info.VendorID                             = properties.vendorID;
            info.Vendor                               = info.GetVendorName();
            info.Memory                               = memoryMB;
            info.Driver                               = info.DecodeDriverVersion(uint32_t(properties.driverVersion));
            info.Type                                 = type;
            info.APIVersion                           = info.DecodeDriverVersion(uint32_t(properties.apiVersion));
            info.Handle                               = device;

            return info;
        }

        uint32_t VKPhysicalDevice::GetMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const
        {
            // Iterate over all memory types available for the device used in this example
            for(uint32_t i = 0; i < m_MemoryProperties.memoryTypeCount; i++)
            {
                if((typeBits & 1) == 1)
                {
                    if((m_MemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
                        return i;
                }
                typeBits >>= 1;
            }

            LUMOS_ASSERT(false, "Could not find a suitable memory type!");
            return UINT32_MAX;
        }

        uint32_t VKDevice::s_GraphicsQueueFamilyIndex = 0;

        VKDevice::VKDevice()
        {
        }

        VKDevice::~VKDevice()
        {
            m_CommandPool.reset();
            vkDestroyPipelineCache(m_Device, m_PipelineCache, VK_NULL_HANDLE);

#ifdef USE_VMA_ALLOCATOR
            for(auto& pool : m_SmallAllocPools)
            {
                vmaDestroyPool(m_Allocator, pool.second);
            }
            m_SmallAllocPools.clear();

            vmaDestroyAllocator(m_Allocator);
#endif
#if LUMOS_PROFILE && defined(TRACY_ENABLE)
            for(int i = 0; i < 4; i++)
                TracyVkDestroy(m_TracyContext[i]);
            TracyVkDestroy(m_PresentTracyContext);
#endif

            vkDestroyDevice(m_Device, VK_NULL_HANDLE);
        }

        bool VKDevice::Init()
        {
            m_PhysicalDevice = CreateSharedPtr<VKPhysicalDevice>();

            VkPhysicalDeviceFeatures supportedFeatures;
            memset(&supportedFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
            memset(&m_EnabledFeatures, 0, sizeof(VkPhysicalDeviceFeatures));

            vkGetPhysicalDeviceFeatures(m_PhysicalDevice->GetHandle(), &supportedFeatures);

            if(supportedFeatures.wideLines)
            {
                m_EnabledFeatures.wideLines           = true;
                Renderer::GetCapabilities().WideLines = true;
            }
            else
            {
                Renderer::GetCapabilities().WideLines = false;
            }

            if(supportedFeatures.samplerAnisotropy)
                m_EnabledFeatures.samplerAnisotropy = true;

            if(supportedFeatures.depthClamp)
                m_EnabledFeatures.depthClamp = true;

            if(supportedFeatures.depthBiasClamp)
                m_EnabledFeatures.depthBiasClamp = true;

            std::vector<const char*> deviceExtensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
            };

            if(m_PhysicalDevice->IsExtensionSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
            {
                deviceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                m_EnableDebugMarkers = true;
            }

            VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexingFeatures = {};
            indexingFeatures.sType                                         = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
            indexingFeatures.runtimeDescriptorArray                        = VK_TRUE;
            indexingFeatures.descriptorBindingPartiallyBound               = VK_TRUE;

#if defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_IOS)
            // https://vulkan.lunarg.com/doc/view/1.2.162.0/mac/1.2-extensions/vkspec.html#VUID-VkDeviceCreateInfo-pProperties-04451
            if(m_PhysicalDevice->IsExtensionSupported("VK_KHR_portability_subset"))
            {
                deviceExtensions.push_back("VK_KHR_portability_subset");
            }

            if(m_PhysicalDevice->IsExtensionSupported(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME))
            {
                deviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
            }
#endif

            // Device
            VkDeviceCreateInfo deviceCreateInfo      = {};
            deviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            deviceCreateInfo.queueCreateInfoCount    = static_cast<uint32_t>(m_PhysicalDevice->m_QueueCreateInfos.size());
            deviceCreateInfo.pQueueCreateInfos       = m_PhysicalDevice->m_QueueCreateInfos.data();
            deviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
            deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
            deviceCreateInfo.pEnabledFeatures        = &m_EnabledFeatures;
            deviceCreateInfo.enabledLayerCount       = 0;
            deviceCreateInfo.pNext                   = (void*)&indexingFeatures;

            auto result = vkCreateDevice(m_PhysicalDevice->GetHandle(), &deviceCreateInfo, VK_NULL_HANDLE, &m_Device);
            if(result != VK_SUCCESS)
            {
                LUMOS_LOG_CRITICAL("[VULKAN] vkCreateDevice() failed!");
                return false;
            }

            vkGetDeviceQueue(m_Device, m_PhysicalDevice->m_QueueFamilyIndices.Graphics, 0, &m_GraphicsQueue);
            vkGetDeviceQueue(m_Device, m_PhysicalDevice->m_QueueFamilyIndices.Graphics, 0, &m_PresentQueue);
            vkGetDeviceQueue(m_Device, m_PhysicalDevice->m_QueueFamilyIndices.Compute, 0, &m_ComputeQueue);

#ifdef USE_VMA_ALLOCATOR
            VmaAllocatorCreateInfo allocatorInfo = {};
            allocatorInfo.physicalDevice         = m_PhysicalDevice->GetHandle();
            allocatorInfo.device                 = m_Device;
            allocatorInfo.instance               = VKContext::GetVKInstance();
            allocatorInfo.vulkanApiVersion       = VKContext::GetVKVersion();

#if LUMOS_PROFILE
            VmaDeviceMemoryCallbacks device_memory_callbacks = {};
            device_memory_callbacks.pfnAllocate              = [](VmaAllocator,
                                                     uint32_t,
                                                     VkDeviceMemory VMA_NOT_NULL_NON_DISPATCHABLE memory,
                                                     VkDeviceSize size,
                                                     void*)
            {
                TracyAllocN(memory, size, "vulkan");
            };
            device_memory_callbacks.pfnFree = [](VmaAllocator,
                                                 uint32_t,
                                                 VkDeviceMemory VMA_NOT_NULL_NON_DISPATCHABLE memory,
                                                 VkDeviceSize size,
                                                 void* VMA_NULLABLE)
            {
                TracyFreeN(memory, "vulkan");
            };
            allocatorInfo.pDeviceMemoryCallbacks = &device_memory_callbacks;
#endif
            VmaVulkanFunctions fn;
            fn.vkAllocateMemory                        = (PFN_vkAllocateMemory)vkAllocateMemory;
            fn.vkBindBufferMemory                      = (PFN_vkBindBufferMemory)vkBindBufferMemory;
            fn.vkBindImageMemory                       = (PFN_vkBindImageMemory)vkBindImageMemory;
            fn.vkCmdCopyBuffer                         = (PFN_vkCmdCopyBuffer)vkCmdCopyBuffer;
            fn.vkCreateBuffer                          = (PFN_vkCreateBuffer)vkCreateBuffer;
            fn.vkCreateImage                           = (PFN_vkCreateImage)vkCreateImage;
            fn.vkDestroyBuffer                         = (PFN_vkDestroyBuffer)vkDestroyBuffer;
            fn.vkDestroyImage                          = (PFN_vkDestroyImage)vkDestroyImage;
            fn.vkFlushMappedMemoryRanges               = (PFN_vkFlushMappedMemoryRanges)vkFlushMappedMemoryRanges;
            fn.vkFreeMemory                            = (PFN_vkFreeMemory)vkFreeMemory;
            fn.vkGetBufferMemoryRequirements           = (PFN_vkGetBufferMemoryRequirements)vkGetBufferMemoryRequirements;
            fn.vkGetImageMemoryRequirements            = (PFN_vkGetImageMemoryRequirements)vkGetImageMemoryRequirements;
            fn.vkGetPhysicalDeviceMemoryProperties     = (PFN_vkGetPhysicalDeviceMemoryProperties)vkGetPhysicalDeviceMemoryProperties;
            fn.vkGetPhysicalDeviceProperties           = (PFN_vkGetPhysicalDeviceProperties)vkGetPhysicalDeviceProperties;
            fn.vkInvalidateMappedMemoryRanges          = (PFN_vkInvalidateMappedMemoryRanges)vkInvalidateMappedMemoryRanges;
            fn.vkMapMemory                             = (PFN_vkMapMemory)vkMapMemory;
            fn.vkUnmapMemory                           = (PFN_vkUnmapMemory)vkUnmapMemory;
            fn.vkGetBufferMemoryRequirements2KHR       = (PFN_vkGetBufferMemoryRequirements2KHR)vkGetBufferMemoryRequirements2KHR;
            fn.vkGetImageMemoryRequirements2KHR        = (PFN_vkGetImageMemoryRequirements2KHR)vkGetImageMemoryRequirements2KHR;
            fn.vkBindImageMemory2KHR                   = 0;
            fn.vkBindBufferMemory2KHR                  = 0;
            fn.vkGetPhysicalDeviceMemoryProperties2KHR = 0;
            fn.vkGetImageMemoryRequirements2KHR        = 0;
            fn.vkGetBufferMemoryRequirements2KHR       = 0;
            fn.vkGetInstanceProcAddr                   = (PFN_vkGetInstanceProcAddr)vkGetInstanceProcAddr;
            fn.vkGetDeviceProcAddr                     = (PFN_vkGetDeviceProcAddr)vkGetDeviceProcAddr;

#if !defined(LUMOS_PLATFORM_MACOS) && !defined(LUMOS_PLATFORM_IOS)
            fn.vkGetDeviceBufferMemoryRequirements = (PFN_vkGetDeviceBufferMemoryRequirements)vkGetDeviceBufferMemoryRequirements;
            fn.vkGetDeviceImageMemoryRequirements  = (PFN_vkGetDeviceImageMemoryRequirements)vkGetDeviceImageMemoryRequirements;
#endif
            allocatorInfo.pVulkanFunctions            = &fn;
            allocatorInfo.preferredLargeHeapBlockSize = 64 * 1024 * 1024;
            if(vmaCreateAllocator(&allocatorInfo, &m_Allocator) != VK_SUCCESS)
            {
                LUMOS_LOG_CRITICAL("[VULKAN] Failed to create VMA allocator");
            }

#endif
            m_CommandPool = CreateSharedPtr<VKCommandPool>(m_PhysicalDevice->GetGraphicsQueueFamilyIndex(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

            CreateTracyContext();
            CreatePipelineCache();
            return VK_SUCCESS;
        }

        void VKDevice::CreatePipelineCache()
        {
            VkPipelineCacheCreateInfo pipelineCacheCI = {};
            pipelineCacheCI.sType                     = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
            pipelineCacheCI.pNext                     = NULL;
            vkCreatePipelineCache(m_Device, &pipelineCacheCI, VK_NULL_HANDLE, &m_PipelineCache);
        }

        void VKDevice::CreateTracyContext()
        {
#if LUMOS_PROFILE && defined(TRACY_ENABLE)
            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool                 = m_CommandPool->GetHandle();
            allocInfo.commandBufferCount          = 1;

            VkCommandBuffer tracyBuffer;
            vkAllocateCommandBuffers(m_Device, &allocInfo, &tracyBuffer);

            m_TracyContext.resize(4);
            for(int i = 0; i < 4; i++)
                m_TracyContext[i] = TracyVkContext(m_PhysicalDevice->GetHandle(), m_Device, m_GraphicsQueue, tracyBuffer);

            m_PresentTracyContext = TracyVkContext(m_PhysicalDevice->GetHandle(), m_Device, m_PresentQueue, tracyBuffer);

            // m_TracyContext = TracyVkContextCalibrated(m_PhysicalDevice->GetHandle(), m_Device, m_GraphicsQueue, tracyBuffer, vkGetPhysicalDeviceCalibrateableTimeDomainsEXT, vkGetCalibratedTimestampsEXT);

            vkQueueWaitIdle(m_GraphicsQueue);
            vkFreeCommandBuffers(m_Device, m_CommandPool->GetHandle(), 1, &tracyBuffer);
#endif
        }

#if LUMOS_PROFILE && defined(TRACY_ENABLE)
        tracy::VkCtx* VKDevice::GetTracyContext(bool present)
        {
            if(present)
                return m_PresentTracyContext;

            return m_TracyContext[VKRenderer::GetMainSwapChain()->GetCurrentBufferIndex() + 1];
        }
#endif

#ifdef USE_VMA_ALLOCATOR
        VmaPool VKDevice::GetOrCreateSmallAllocPool(uint32_t memTypeIndex)
        {
            if(m_SmallAllocPools.find(memTypeIndex) != m_SmallAllocPools.end())
                return m_SmallAllocPools[memTypeIndex];

            LUMOS_LOG_INFO("Creating VMA small objects pool for memory type index {0}", memTypeIndex);

            VmaPoolCreateInfo pci;
            pci.memoryTypeIndex        = memTypeIndex;
            pci.flags                  = VMA_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT;
            pci.blockSize              = 0;
            pci.minBlockCount          = 0;
            pci.maxBlockCount          = SIZE_MAX;
            pci.priority               = 0.5f;
            pci.minAllocationAlignment = 0;
            pci.pMemoryAllocateNext    = nullptr;
            VmaPool pool               = VK_NULL_HANDLE;
            VK_CHECK_RESULT(vmaCreatePool(m_Allocator, &pci, &pool));
            m_SmallAllocPools[memTypeIndex] = pool;
            return pool;
        }
#endif

        void VKGPUMarker::Begin(const char* name)
        {
            VkCommandBuffer commandBuffer = static_cast<Lumos::Graphics::VKCommandBuffer*>(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer())->GetHandle();
            VkDebugUtilsLabelEXT debugLabel {};
            debugLabel.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
            debugLabel.pLabelName = name;
            fpCmdBeginDebugUtilsLabelEXT(commandBuffer, &debugLabel);
        }

        void VKGPUMarker::End()
        {
            VkCommandBuffer commandBuffer = static_cast<Lumos::Graphics::VKCommandBuffer*>(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer())->GetHandle();
            fpCmdEndDebugUtilsLabelEXT(commandBuffer);
        }
    }
}
