#ifdef LUMOS_RENDER_API_VULKAN

#import <Cocoa/Cocoa.h>
#include <QuartzCore/CAMetalLayer.h>

#include "Platform/Vulkan/VKDevice.h"
#include "App/Application.h"

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

namespace Lumos
{
	VkSurfaceKHR Graphics::VKDevice::CreatePlatformSurface(VkInstance vkInstance, Window* window)
	{
		VkSurfaceKHR surface;

        VkMacOSSurfaceCreateInfoMVK surfaceInfo;
        surfaceInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
        surfaceInfo.pNext = NULL;
        surfaceInfo.pView = MakeViewMetalCompatible((void*)glfwGetCocoaWindow(static_cast<GLFWwindow*>(window->GetHandle())));
        vkCreateMacOSSurfaceMVK(vkInstance, &surfaceInfo, nullptr, &surface);

		return surface;
	}

    static const char* GetPlatformSurfaceExtension()
	{
		return VK_MVK_MACOS_SURFACE_EXTENSION_NAME;
	}
}

#endif
