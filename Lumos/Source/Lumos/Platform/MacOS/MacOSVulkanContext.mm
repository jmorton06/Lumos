#ifdef LUMOS_RENDER_API_VULKAN

#include <QuartzCore/CAMetalLayer.h>

#include "Platform/Vulkan/VKSwapChain.h"
#include "Core/OS/Window.h"
#include "Core/Application.h"
#undef _GLFW_REQUIRE_LOADER
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#undef VK_NO_PROTOTYPES
#include <MoltenVK/vk_mvk_moltenvk.h>


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
	VkSurfaceKHR Graphics::VKSwapChain::CreatePlatformSurface(VkInstance vkInstance, Window* window)
	{
	    auto* cocoaWin = static_cast<GLFWwindow*>(window->GetHandle());
        void* layer = GetCAMetalLayer(glfwGetCocoaWindow(cocoaWin));

        VkSurfaceKHR surface = VK_NULL_HANDLE;
#if defined(VK_USE_PLATFORM_METAL_EXT)
        VkMetalSurfaceCreateInfoEXT info{};
        info.sType  = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
        info.pLayer = static_cast<CAMetalLayer*>(layer);
        VkResult res = vkCreateMetalSurfaceEXT(vkInstance, &info, nullptr, &surface);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
        VkMacOSSurfaceCreateInfoMVK info{};
        info.sType  = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
        info.pView  = layer;
        VkResult res = vkCreateMacOSSurfaceMVK(vkInstance, &info, nullptr, &surface);
#endif

        if (res != VK_SUCCESS || surface == VK_NULL_HANDLE)
        LFATAL("Failed to create Vulkan surface: %s",std::to_string(res).c_str());

#ifdef MVK_VERSION_STRING
        LINFO("MVK Version %s", MVK_VERSION_STRING);
#endif
		return surface;
	}
}

#endif
