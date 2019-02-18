#include "LM.h"
#include "Window.h"
#include "Graphics/API/Context.h"

#ifdef LUMOS_PLATFORM_MOBILE
#include "Platform/GLFM/GLFMWindow.h"
#endif

#ifdef LUMOS_PLATFORM_WINDOWS
#include "Platform/Windows/Win32Window.h"
#endif

#if ( defined LUMOS_RENDER_API_OPENGL || defined LUMOS_RENDER_API_VULKAN && !defined LUMOS_PLATFORM_MOBILE )
#include "Platform/GLFW/GLFWWindow.h"
#endif
namespace Lumos
{
	Window* Window::Create(const std::string& title, const WindowProperties& properties)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef LUMOS_PLATFORM_MOBILE
#ifdef LUMOS_RENDER_API_OPENGL 
		case RenderAPI::OPENGL:		return new GLFMWindow(properties, title);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
        case RenderAPI::VULKAN:		return new GLFMWindow(properties, title);
#endif
#endif

#ifdef LUMOS_PLATFORM_WINDOWS
#ifdef LUMOS_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return new Win32Window(properties, title);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
		case RenderAPI::VULKAN:		return new Win32Window(properties, title, RenderAPI::VULKAN);
#endif
#endif

#if((defined LUMOS_PLATFORM_MACOS || defined LUMOS_PLATFORM_LINUX ))
#ifdef LUMOS_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return new GLFWWindow(properties, title);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
		case RenderAPI::VULKAN:		return new GLFWWindow(properties, title, RenderAPI::VULKAN);
#endif
#endif
		}
		return nullptr;
	}

	bool Window::Initialise(const String& title, const WindowProperties& properties)
	{
		return HasInitialised();
	}
}
