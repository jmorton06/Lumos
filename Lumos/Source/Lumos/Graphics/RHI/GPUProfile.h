#pragma once
#include "Graphics/RHI/GraphicsContext.h"

#if defined(LUMOS_RENDER_API_VULKAN)
#include "Platform/Vulkan/VKDevice.h"
#include "Platform/Vulkan/VKCommandBuffer.h"
#endif

#if LUMOS_PROFILE_GPU_TIMINGS && defined(LUMOS_RENDER_API_VULKAN) && defined(LUMOS_PROFILE) && defined(TRACY_ENABLE)
#define LUMOS_PROFILE_GPU(name)                                                                                                                                                                  \
    TracyVkZone(Lumos::Graphics::VKDevice::Get().GetTracyContext(), static_cast<Lumos::Graphics::VKCommandBuffer*>(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer())->GetHandle(), name); \
    Lumos::Graphics::VKGPUMarker GPUMarker(name)
#else
#define LUMOS_PROFILE_GPU(name) Lumos::Graphics::VKGPUMarker GPUMarker(name)
#endif
