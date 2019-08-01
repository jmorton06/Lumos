#include "LM.h"
#include "Window.h"

#ifdef LUMOS_PLATFORM_MOBILE
#include "Platform/GLFM/GLFMWindow.h"
#endif

#ifdef LUMOS_PLATFORM_WINDOWS
#include "Platform/Windows/WindowsWindow.h"
#endif

#if ( defined LUMOS_RENDER_API_OPENGL || defined LUMOS_RENDER_API_VULKAN && !defined LUMOS_PLATFORM_MOBILE )
#include "Platform/GLFW/GLFWWindow.h"
#endif
namespace Lumos
{
	Window* Window::Create(const WindowProperties& properties)
	{

#ifdef LUMOS_PLATFORM_WINDOWS
#ifdef LUMOS_USE_GLFW_WINDOWS
		return lmnew GLFWWindow(properties);
#else
		return lmnew WindowsWindow(properties);

#endif
#elif((defined LUMOS_PLATFORM_MACOS || defined LUMOS_PLATFORM_LINUX ))
		return lmnew GLFWWindow(properties);
#elif LUMOS_PLATFORM_MOBILE
		return lmnew GLFMWindow(properties);
#else
		return nullptr;
#endif
	}

	bool Window::Initialise(const WindowProperties& properties)
	{
		return HasInitialised();
	}
}
