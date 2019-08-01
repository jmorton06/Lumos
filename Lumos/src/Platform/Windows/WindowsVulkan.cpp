#include "LM.h"

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
	vk::SurfaceKHR Graphics::VKDevice::CreatePlatformSurface(vk::Instance vkInstance, Window* window)
	{
		vk::SurfaceKHR surface;

#ifdef LUMOS_USE_GLFW_WINDOWS
		glfwCreateWindowSurface(vkInstance, static_cast<GLFWwindow*>(window->GetHandle()), nullptr, (VkSurfaceKHR*)&surface);
#else
		vk::Win32SurfaceCreateInfoKHR surfaceInfo;
		surfaceInfo.pNext = NULL;
		surfaceInfo.hwnd = (HWND)window->GetHandle();
		surfaceInfo.hinstance = ((WindowsWindow*)window)->GetHInstance();
		surface = m_VKContext->GetVKInstance().createWin32SurfaceKHR(surfaceInfo);
#endif

		return surface;
	}

	static const char* GetPlatformSurfaceExtension()
	{
		return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
	}
}

#endif