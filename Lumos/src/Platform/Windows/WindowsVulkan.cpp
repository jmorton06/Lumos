#include "lmpch.h"

#ifdef LUMOS_RENDER_API_VULKAN

#include "Platform/Vulkan/VKDevice.h"
#include "App/Application.h"
#include "WindowsWindow.h"

#ifdef LUMOS_USE_GLFW_WINDOWS
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#else
#include "WindowsWindow.h"
#endif

namespace Lumos
{
	VkSurfaceKHR Graphics::VKDevice::CreatePlatformSurface(VkInstance vkInstance, Window* window)
	{
		VkSurfaceKHR surface;

#ifdef LUMOS_USE_GLFW_WINDOWS
		glfwCreateWindowSurface(vkInstance, static_cast<GLFWwindow*>(window->GetHandle()), nullptr, (VkSurfaceKHR*)&surface);
#else
		VkWin32SurfaceCreateInfoKHR surfaceInfo;
		memset(&surfaceInfo, 0, sizeof(VkWin32SurfaceCreateInfoKHR));

		surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceInfo.pNext = NULL;
		surfaceInfo.hwnd = static_cast<HWND>(window->GetHandle());
		surfaceInfo.hinstance = static_cast<WindowsWindow*>(window)->GetHInstance();
		vkCreateWin32SurfaceKHR(m_VKContext->GetVKInstance(), &surfaceInfo, nullptr, &surface);
#endif

		return surface;
	}

	static const char* GetPlatformSurfaceExtension()
	{
		return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
	}
}

#endif