#pragma once

#ifdef LUMOS_VOLK
#define VK_NO_PROTOTYPES
#include <vulkan/volk/volk.h>
#else
#include <vulkan/vulkan.h>
#endif

inline PFN_vkCmdBeginDebugUtilsLabelEXT fpCmdBeginDebugUtilsLabelEXT;
inline PFN_vkCmdEndDebugUtilsLabelEXT fpCmdEndDebugUtilsLabelEXT;
inline PFN_vkSetDebugUtilsObjectNameEXT fpSetDebugUtilsObjectNameEXT;

#ifdef USE_VMA_ALLOCATOR
#ifdef LUMOS_DEBUG
#define VMA_DEBUG_MARGIN 16
#define VMA_DEBUG_DETECT_CORRUPTION 1
#endif

#define ENABLE_VMA_LOG 0

#if ENABLE_VMA_LOG
static char VMA_LOG_BUFFER[100];
#define VMA_DEBUG_LOG(...)                \
    sprintf(VMA_LOG_BUFFER, __VA_ARGS__); \
    LUMOS_LOG_INFO((const char*)VMA_LOG_BUFFER)
#endif

#if defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_IOS)
#undef VMA_VULKAN_VERSION

// Cap version to 1.2 for apple devices
// Fixes issue with vma assuming vkGetDeviceBufferMemoryRequirements is available
#if defined(VK_VERSION_1_3)
#define VMA_VULKAN_VERSION 1002000
#elif defined(VK_VERSION_1_2)
#define VMA_VULKAN_VERSION 1002000
#elif defined(VK_VERSION_1_1)
#define VMA_VULKAN_VERSION 1001000
#else
#define VMA_VULKAN_VERSION 1000000
#endif
#endif
#include <vulkan/vk_mem_alloc.h>
#endif
