#pragma once
#include "Core/Core.h"

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
#define VMA_DEBUG_LOG(...)                      \
    stbsp_sprintf(VMA_LOG_BUFFER, __VA_ARGS__); \
    LINFO((const char*)VMA_LOG_BUFFER)

#define VMA_DEBUG_LOG_FORMAT(...)               \
    stbsp_sprintf(VMA_LOG_BUFFER, __VA_ARGS__); \
    LERROR((const char*)VMA_LOG_BUFFER)
#endif

#define DISABLE_VMA_ASSERT 1
#if DISABLE_VMA_ASSERT
#define VMA_ASSERT(...) \
    void(0)
#endif

#if defined(CAP_VK_VERSION_12) && (defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_IOS))
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
