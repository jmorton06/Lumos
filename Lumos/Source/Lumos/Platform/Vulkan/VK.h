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
