#ifdef LUMOS_RENDER_API_VULKAN

#import <Cocoa/Cocoa.h>
#include <QuartzCore/CAMetalLayer.h>

#include "Platform/Vulkan/VKSwapchain.h"
#include "Core/Application.h"
#undef _GLFW_REQUIRE_LOADER
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

extern "C" void* GetCAMetalLayer(void* handle)
{
    NSWindow* window = (NSWindow*)handle;
    NSView* view = window.contentView;
    
    if (![view.layer isKindOfClass:[CAMetalLayer class]])
    {
        [view setLayer:[CAMetalLayer layer]];
        [view setWantsLayer:YES];
        [view.layer setContentsScale:[window backingScaleFactor]];
    }
    
    return view.layer;
}

namespace Lumos
{
	VkSurfaceKHR Graphics::VKSwapchain::CreatePlatformSurface(VkInstance vkInstance, Window* window)
	{
		VkSurfaceKHR surface;
#if defined(VK_USE_PLATFORM_MACOS_MVK)
        VkMacOSSurfaceCreateInfoMVK surfaceInfo;
        surfaceInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
        surfaceInfo.pNext = NULL;
        surfaceInfo.pView = GetCAMetalLayer((void*)glfwGetCocoaWindow(static_cast<GLFWwindow*>(window->GetHandle())));
        vkCreateMacOSSurfaceMVK(vkInstance, &surfaceInfo, nullptr, &surface);

#elif defined(VK_USE_PLATFORM_METAL_EXT)
          
        VkMetalSurfaceCreateInfoEXT surfaceInfo;
        surfaceInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
        surfaceInfo.pNext = NULL;
        surfaceInfo.flags = 0;
        surfaceInfo.pLayer = (CAMetalLayer*)GetCAMetalLayer((void*)glfwGetCocoaWindow(static_cast<GLFWwindow*>(window->GetHandle())));
        vkCreateMetalSurfaceEXT(vkInstance, &surfaceInfo, nullptr, &surface);
#endif
        
		return surface;
	}
}

#endif
