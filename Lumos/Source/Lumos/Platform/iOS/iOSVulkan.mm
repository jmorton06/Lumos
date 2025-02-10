#if defined(LUMOS_RENDER_API_VULKAN)

#include "Core/Application.h"
#include "iOSOS.h"
#include "iOSWindow.h"
#include "Platform/Vulkan/VKSwapChain.h"
#include <vulkan/vulkan_metal.h>

#if 0
#include <MoltenVK/vk_mvk_moltenvk.h>
#endif

#import <MetalKit/MetalKit.h>

namespace Lumos
{
	VkSurfaceKHR Graphics::VKSwapChain::CreatePlatformSurface(VkInstance vkInstance, Window* window)
	{
		VkSurfaceKHR surface;
		CAMetalLayer* iosView = iOSOS::GetStaticLayer();// iOSOS::Get()->GetLayerPtr();

		VkMetalSurfaceCreateInfoEXT surfaceCreateInfo = {};
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
        surfaceCreateInfo.pNext = NULL;
        surfaceCreateInfo.pLayer = iosView;
		VkResult result = vkCreateMetalSurfaceEXT(vkInstance, &surfaceCreateInfo, nullptr, &surface);

		if (result != VK_SUCCESS) {
			NSLog(@"Failed to create Vulkan surface. VkResult: %d", result);
		}

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
