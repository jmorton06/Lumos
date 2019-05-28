#include "LM.h"
#include "Window.h"
#include "Graphics/API/GraphicsContext.h"

#ifdef LUMOS_PLATFORM_MOBILE
#include "Platform/GLFM/GLFMWindow.h"
#endif

#ifdef LUMOS_PLATFORM_WINDOWS
#include "Platform/Windows/WindowsWindow.h"
#endif

#if ( defined LUMOS_RENDER_API_OPENGL || defined LUMOS_RENDER_API_VULKAN && !defined LUMOS_PLATFORM_MOBILE )
#include "Platform/GLFW/GLFWWindow.h"
#endif
namespace lumos
{
	Window* Window::Create(const WindowProperties& properties)
	{
		switch (graphics::GraphicsContext::GetRenderAPI())
		{
#ifdef LUMOS_PLATFORM_MOBILE
#ifdef LUMOS_RENDER_API_OPENGL 
		case graphics::RenderAPI::OPENGL:		return new GLFMWindow(properties);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
        case graphics::RenderAPI::VULKAN:		return new GLFMWindow(properties);
#endif
#endif

#ifdef LUMOS_PLATFORM_WINDOWS
#ifdef LUMOS_RENDER_API_OPENGL
		case graphics::RenderAPI::OPENGL:		return new WindowsWindow(properties);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
		case graphics::RenderAPI::VULKAN:		return new WindowsWindow(properties);
#endif
#endif

#if((defined LUMOS_PLATFORM_MACOS || defined LUMOS_PLATFORM_LINUX ))
#ifdef LUMOS_RENDER_API_OPENGL
		case graphics::RenderAPI::OPENGL:		return new GLFWWindow(properties);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
		case graphics::RenderAPI::VULKAN:		return new GLFWWindow(properties);
#endif
#endif
		}
		return nullptr;
	}

	bool Window::Initialise(const WindowProperties& properties)
	{
		return HasInitialised();
	}
}
