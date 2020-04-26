#pragma once

#ifdef LUMOS_VOLK
#define VK_NO_PROTOTYPES
#include <volk/volk.h>
#else
#include <vulkan/vulkan.h>
#endif