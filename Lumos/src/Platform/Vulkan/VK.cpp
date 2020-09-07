#include "Precompiled.h"

#ifdef USE_VMA_ALLOCATOR
#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#include <vulkan/vk_mem_alloc.h>
#endif
