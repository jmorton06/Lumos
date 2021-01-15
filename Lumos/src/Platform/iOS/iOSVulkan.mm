#if defined(LUMOS_RENDER_API_VULKAN)

#include "Platform/Vulkan/VKSwapchain.h"
#include "Core/Application.h"
#include "iOSOS.h"
#include "iOSWindow.h"

#if 0
#include <MoltenVK/vk_mvk_moltenvk.h>
#endif

namespace Lumos
{
	VkSurfaceKHR Graphics::VKSwapchain::CreatePlatformSurface(VkInstance vkInstance, Window* window)
	{
		VkSurfaceKHR surface;

        auto iosView = window->GetHandle();

        VkIOSSurfaceCreateInfoMVK surfaceCreateInfo = {};
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK;
        surfaceCreateInfo.pNext = NULL;
        surfaceCreateInfo.pView = iosView;
        vkCreateIOSSurfaceMVK(vkInstance, &surfaceCreateInfo, nullptr, &surface);
        
#if 0
        MVKConfiguration mvkConfig;
        size_t pConfigurationSize = sizeof(MVKConfiguration);
        vkGetMoltenVKConfigurationMVK(vkInstance, &mvkConfig, &pConfigurationSize);
        mvkConfig.debugMode = true;
        mvkConfig.synchronousQueueSubmits = false;
        mvkConfig.presentWithCommandBuffer = false;
        mvkConfig.prefillMetalCommandBuffers = true;
        vkSetMoltenVKConfigurationMVK(vkInstance, &mvkConfig, &pConfigurationSize);
#endif
		return surface;
	}

#if 0
    static const char* GetPlatformSurfaceExtension()
	{
		return VK_MVK_IOS_SURFACE_EXTENSION_NAME;
	}
#endif

}

#endif
