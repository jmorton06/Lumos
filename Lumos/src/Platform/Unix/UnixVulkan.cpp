#if defined(LUMOS_RENDER_API_VULKAN) && !defined(LUMOS_PLATFORM_MACOS)

#include "Platform/Vulkan/VKDevice.h"
#include "App/Application.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace Lumos
{
	vk::SurfaceKHR Graphics::VKDevice::CreatePlatformSurface(vk::Instance vkInstance, Window* window)
	{
		vk::SurfaceKHR surface;

        glfwCreateWindowSurface(vkInstance, static_cast<GLFWwindow*>(window->GetHandle()), nullptr, (VkSurfaceKHR*)&surface);

		return surface;
	}

	static const char* GetPlatformSurfaceExtension()
	{
		return VK_KHR_XCB_SURFACE_EXTENSION_NAME;
	}
}

#endif