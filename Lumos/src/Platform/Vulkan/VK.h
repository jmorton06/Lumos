#pragma once

#ifdef LUMOS_PLATFORM_IOS
#include <vulkan/vulkan.h>
#else
#include <volk/volk.h>
#endif
#define USE_VMA_ALLOCATOR
