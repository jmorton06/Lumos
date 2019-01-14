#include "LM.h"
#include "Window.h"
#include "Graphics/API/Context.h"

#ifdef LUMOS_PLATFORM_MOBILE
#include "Platform/GLFM/GLFMWindow.h"
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

#if ( ( defined LUMOS_RENDER_API_OPENGL || defined LUMOS_RENDER_API_VULKAN ) && !defined LUMOS_PLATFORM_MOBILE )
		case RenderAPI::OPENGL:		return new GLFWWindow(properties, title);
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
