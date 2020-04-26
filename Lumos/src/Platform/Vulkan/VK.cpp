#include "lmpch.h"

#ifdef USE_VMA_ALLOCATOR
#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 1
#include <vulkan/vk_mem_alloc.h>
#endif