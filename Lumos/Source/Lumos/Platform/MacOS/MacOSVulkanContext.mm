#ifdef LUMOS_RENDER_API_VULKAN

#include <QuartzCore/CAMetalLayer.h>

#include "Platform/Vulkan/VKSwapChain.h"
#include "Core/OS/Window.h"
#include "Core/Application.h"
#undef _GLFW_REQUIRE_LOADER
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <dlfcn.h>

#include <MoltenVK/vk_mvk_moltenvk.h>

//Copied from newer versions of mvk
#define MVK_STRINGIFY_IMPL_COPY(val) #val
#define MVK_STRINGIFY_COPY(val)       MVK_STRINGIFY_IMPL_COPY(val)
#define MVK_VERSION_STRING_COPY       (MVK_STRINGIFY_COPY(MVK_VERSION_MAJOR) "." MVK_STRINGIFY_COPY(MVK_VERSION_MINOR) "." MVK_STRINGIFY_COPY(MVK_VERSION_PATCH))


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

        void* lib = dlopen("/usr/local/lib/libMoltenVK.dylib", RTLD_NOW | RTLD_LOCAL);
        if (!lib)
            LERROR("dlopen failed: %s", dlerror());

		auto libMoltenVK = dlopen("/usr/local/lib/libMoltenVK.dylib", RTLD_NOW | RTLD_LOCAL);
		auto getMoltenVKConfigurationMVK = (PFN_vkGetMoltenVKConfigurationMVK)
			dlsym(libMoltenVK, "vkGetMoltenVKConfigurationMVK");
		auto setMoltenVKConfigurationMVK = (PFN_vkSetMoltenVKConfigurationMVK)
			dlsym(libMoltenVK, "vkSetMoltenVKConfigurationMVK");

		if (!getMoltenVKConfigurationMVK || !setMoltenVKConfigurationMVK)
            LERROR("MoltenVK config entrypoints not found");

		MVKConfiguration mvkConfig;
        size_t pConfigurationSize = sizeof(MVKConfiguration);
        getMoltenVKConfigurationMVK(vkInstance, &mvkConfig, &pConfigurationSize);
		#ifndef LUMOS_PLATFORM_PRODUCTION
        mvkConfig.debugMode = true;
		#endif

		//mvkConfig.traceVulkanCalls = MVK_CONFIG_TRACE_VULKAN_CALLS_DURATION;
		mvkConfig.performanceTracking = false;
        mvkConfig.synchronousQueueSubmits = false;
        //mvkConfig.presentWithCommandBuffer = false;
        //mvkConfig.prefillMetalCommandBuffers = true;

		//mvkConfig.useMetalArgumentBuffers = MVKUseMetalArgumentBuffers::MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS_ALWAYS;
        mvkConfig.resumeLostDevice = true;

        setMoltenVKConfigurationMVK(vkInstance, &mvkConfig, &pConfigurationSize);
#ifdef MVK_VERSION_STRING_COPY
        LINFO("MVK Version %s", MVK_VERSION_STRING_COPY);
#endif
		return surface;
	}
}

#endif
