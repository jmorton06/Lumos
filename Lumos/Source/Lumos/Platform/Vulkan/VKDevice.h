#pragma once
#include "Utilities/TSingleton.h"
#include "VK.h"
#include "VKContext.h"
#include "VKCommandPool.h"

#ifdef USE_VMA_ALLOCATOR
#ifdef LUMOS_DEBUG
#define VMA_DEBUG_MARGIN 16
#define VMA_DEBUG_DETECT_CORRUPTION 1
#endif
#include <vulkan/vk_mem_alloc.h>
#endif

#include <unordered_set>

#if defined(LUMOS_PROFILE) && defined(TRACY_ENABLE)
#include <Tracy/TracyVulkan.hpp>
#endif

namespace Lumos
{
    namespace Graphics
    {
        class VKPhysicalDevice
        {
        public:
            VKPhysicalDevice();
            ~VKPhysicalDevice();

            bool IsExtensionSupported(const std::string& extensionName) const;
            uint32_t GetMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const;

            VkPhysicalDevice GetVulkanPhysicalDevice() const
            {
                return m_PhysicalDevice;
            }
            int32_t GetGraphicsQueueFamilyIndex()
            {
                return m_QueueFamilyIndices.Graphics;
            }
            VkPhysicalDeviceProperties GetProperties() const
            {
                return m_PhysicalDeviceProperties;
            };

        private:
            struct QueueFamilyIndices
            {
                int32_t Graphics = -1;
                int32_t Compute = -1;
                int32_t Transfer = -1;
            };
            QueueFamilyIndices GetQueueFamilyIndices(int queueFlags);

        private:
            QueueFamilyIndices m_QueueFamilyIndices;

            std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties;
            std::unordered_set<std::string> m_SupportedExtensions;
            std::vector<VkDeviceQueueCreateInfo> m_QueueCreateInfos;

            VkPhysicalDevice m_PhysicalDevice;
            VkPhysicalDeviceFeatures m_Features;
            VkPhysicalDeviceProperties m_PhysicalDeviceProperties;
            VkPhysicalDeviceMemoryProperties m_MemoryProperties;

            friend class VKDevice;
        };

        class VKDevice : public ThreadSafeSingleton<VKDevice>
        {
            friend class TSingleton<VKDevice>;

        public:
            VKDevice();
            ~VKDevice();

            bool Init();
            void CreatePipelineCache();
            void CreateTracyContext();

            VkDevice GetDevice() const
            {
                return m_Device;
            };
            VkPhysicalDevice GetGPU() const
            {
                return m_PhysicalDevice->GetVulkanPhysicalDevice();
            };
            const Ref<VKPhysicalDevice>& GetPhysicalDevice() const
            {
                return m_PhysicalDevice;
            }

            VkQueue GetGraphicsQueue() const
            {
                return m_GraphicsQueue;
            };
            VkQueue GetPresentQueue() const
            {
                return m_PresentQueue;
            };

            const Ref<VKCommandPool>& GetCommandPool() const
            {
                return m_CommandPool;
            }

            VkPipelineCache GetPipelineCache() const
            {
                return m_PipelineCache;
            }

#if defined(LUMOS_PROFILE) && defined(TRACY_ENABLE)
            tracy::VkCtx* GetTracyContext()
            {
                return m_TracyContext;
            }
#endif

#ifdef USE_VMA_ALLOCATOR
            VmaAllocator GetAllocator() const
            {
                return m_Allocator;
            }
#endif

            static VkDevice GetHandle()
            {
                return VKDevice::Get().GetDevice();
            }

        private:
            VkDevice m_Device;

            VkQueue m_GraphicsQueue;
            VkQueue m_PresentQueue;
            VkPipelineCache m_PipelineCache;
            VkDescriptorPool m_DescriptorPool;
            VkPhysicalDeviceFeatures m_EnabledFeatures;

            Ref<VKCommandPool> m_CommandPool;
            Ref<VKPhysicalDevice> m_PhysicalDevice;

            bool m_EnableDebugMarkers = false;

            static uint32_t s_GraphicsQueueFamilyIndex;

#if defined(LUMOS_PROFILE) && defined(TRACY_ENABLE)
            tracy::VkCtx* m_TracyContext = nullptr;
#endif

#ifdef USE_VMA_ALLOCATOR
            VmaAllocator m_Allocator {};
#endif
        };
    }
}
