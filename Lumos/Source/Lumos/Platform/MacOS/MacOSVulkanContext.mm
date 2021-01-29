#ifdef LUMOS_RENDER_API_VULKAN

#import <Cocoa/Cocoa.h>
#include <QuartzCore/CAMetalLayer.h>

#include "Platform/Vulkan/VKSwapchain.h"
#include "Core/Application.h"

#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

extern "C" void* MakeViewMetalCompatible(void* handle)
{
    NSWindow* window = (NSWindow*)handle;
    NSView* view = window.contentView;
    
    if (![view.layer isKindOfClass:[CAMetalLayer class]])
    {
        [view setLayer:[CAMetalLayer layer]];
        [view setWantsLayer:YES];
    }
    
    return (void*)view;
}

extern "C" void* GetCAMetalLayer(void* handle)
{
    NSWindow* window = (NSWindow*)handle;
    NSView* view = window.contentView;
    
    if (![view.layer isKindOfClass:[CAMetalLayer class]])
    {
        [view setLayer:[CAMetalLayer layer]];
        [view setWantsLayer:YES];
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
        surfaceInfo.pView = MakeViewMetalCompatible((void*)glfwGetCocoaWindow(static_cast<GLFWwindow*>(window->GetHandle())));
        vkCreateMacOSSurfaceMVK(vkInstance, &surfaceInfo, nullptr, &surface);

#elif defined(VK_USE_PLATFORM_METAL_EXT)
        VkMetalSurfaceCreateInfoEXT surfaceInfo;
        surfaceInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
        surfaceInfo.pNext = NULL;
        surfaceInfo.pLayer = (CAMetalLayer*)GetCAMetalLayer((void*)glfwGetCocoaWindow(static_cast<GLFWwindow*>(window->GetHandle())));
        vkCreateMetalSurfaceEXT(vkInstance, &surfaceInfo, nullptr, &surface);
#endif
		return surface;
	}
}

#endif
