#if defined(LUMOS_RENDER_API_VULKAN) && !defined(LUMOS_PLATFORM_MACOS) && !defined(LUMOS_PLATFORM_IOS)

#	include "Platform/Vulkan/VKSwapchain.h"
#	include "Core/Application.h"

#	include <GLFW/glfw3.h>
#	include <GLFW/glfw3native.h>

namespace Lumos
{
	VkSurfaceKHR Graphics::VKSwapchain::CreatePlatformSurface(VkInstance vkInstance, Window* window)
	{
		VkSurfaceKHR surface;

		glfwCreateWindowSurface(vkInstance, static_cast<GLFWwindow*>(window->GetHandle()), nullptr, (VkSurfaceKHR*)&surface);

		return surface;
	}

	static const char* GetPlatformSurfaceExtension()
	{
		return VK_KHR_XCB_SURFACE_EXTENSION_NAME;
	}
}

#endif
