#pragma once
#include "Graphics/RHI/GraphicsContext.h"

#if defined(LUMOS_RENDER_API_VULKAN)
#include "Platform/Vulkan/VKDevice.h"
#include "Platform/Vulkan/VKCommandBuffer.h"
#include <Tracy/public/tracy/TracyVulkan.hpp>
#endif

#if LUMOS_VULKAN_MARKERS && defined(LUMOS_RENDER_API_VULKAN)
#define LUMOS_VULKAN_GPU_MARKER(name) Lumos::Graphics::VKGPUMarker GPUMarker(name)
#else
#define LUMOS_VULKAN_GPU_MARKER(name) (void(0))
#endif

#if LUMOS_PROFILE_GPU_TIMINGS && defined(LUMOS_RENDER_API_VULKAN) && defined(LUMOS_PROFILE) && defined(TRACY_ENABLE)
#define LUMOS_PROFILE_GPU(name)                                                                                                                                                                  \
    TracyVkZone(Lumos::Graphics::VKDevice::Get().GetTracyContext(), static_cast<Lumos::Graphics::VKCommandBuffer*>(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer())->GetHandle(), name); \
    LUMOS_VULKAN_GPU_MARKER(name)
#else
#define LUMOS_PROFILE_GPU(name) LUMOS_VULKAN_GPU_MARKER(name)
#endif
