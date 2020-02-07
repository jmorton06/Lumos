#pragma once

#ifdef LUMOS_PLATFORM_IOS_TEST
#include <vulkan/vulkan.h>
#else
#include <volk/volk.h>
#endif
#define USE_VMA_ALLOCATOR
