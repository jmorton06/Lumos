#include "LM.h"

#if defined(LUMOS_PLATFORM_MACOS)
//#define VK_USE_PLATFORM_MACOS_MVK
#define GLFW_EXPOSE_NATIVE_COCOA
#endif


#include "GLFWWindow.h"
#include "Graphics/API/Context.h"
#include "Utilities/LoadImage.h"

#include "Platform/GraphicsAPI/OpenGL/GL.h"
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#ifdef LUMOS_RENDER_API_VULKAN
#include <vulkan.h>
#include "Platform/GraphicsAPI/Vulkan/VKRenderer.h"
#endif
#include "App/Input.h"
#include "App/Application.h"

#include "Events/ApplicationEvent.h"
#include "Events/MouseEvent.h"
#include "Events/KeyEvent.h"

namespace Lumos
{
	static bool s_GLFWInitialized = false;
	static int  s_NumGLFWWindows = 0;

	static void GLFWErrorCallback(int error, const char* description)
	{
		LUMOS_CORE_ERROR("GLFW Error - {0} : {1}", error, description);
	}

	GLFWWindow::GLFWWindow(const WindowProperties& properties, const String& title, RenderAPI api)
	{
		m_Init = false;
		m_VSync = properties.VSync;
		m_Timer = new Timer();
        SetHasResized(true);
		m_RenderAPI = api;

		m_Init = Init(properties, title);

		graphics::Context::Create(properties, m_Handle);
	}

	GLFWWindow::~GLFWWindow()
	{
		if (m_Timer != nullptr)
		{
			delete m_Timer;
			m_Timer = nullptr;
		}

		glfwDestroyWindow(m_Handle);
		s_NumGLFWWindows--;

		if(s_NumGLFWWindows < 1)
		{
			glfwTerminate();
		}
	}

	bool GLFWWindow::Init(const WindowProperties& properties, const String& title)
	{
		LUMOS_CORE_INFO("Creating window - Title : {0}, Width : {1}, Height : {2}", title, properties.Width, properties.Height);

		if (!s_GLFWInitialized)
		{
			int success = glfwInit();
			LUMOS_CORE_ASSERT(success, "Could not initialize GLFW!");
			glfwSetErrorCallback(GLFWErrorCallback);

			s_GLFWInitialized = true;
			s_NumGLFWWindows++;
		}

#ifdef LUMOS_PLATFORM_MACOS
		if (m_RenderAPI == RenderAPI::OPENGL)
		{
            glfwWindowHint(GLFW_SAMPLES, 1);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
            glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GL_TRUE);
            glfwWindowHint(GLFW_COCOA_GRAPHICS_SWITCHING, GL_TRUE);
            glfwWindowHint(GLFW_STENCIL_BITS, 8); // Fixes 16 bit stencil bits in macOS.
            glfwWindowHint(GLFW_STEREO, GLFW_FALSE);
		}
#endif

		SetBorderlessWindow(properties.Borderless);

		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

		uint ScreenWidth = 0;
		uint ScreenHeight = 0;

		if (properties.Fullscreen)
		{
			ScreenWidth = mode->width;
			ScreenHeight = mode->height;
		}
		else
		{
			ScreenWidth = properties.Width;
			ScreenHeight = properties.Height;
		}

		m_Data.Title = title;
		m_Data.Width = ScreenWidth;
		m_Data.Height = ScreenHeight;
		m_Data.Exit = false;

#ifdef LUMOS_RENDER_API_VULKAN
		if(m_RenderAPI == RenderAPI::VULKAN)
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif

		m_Handle = glfwCreateWindow(ScreenWidth, ScreenHeight, title.c_str(), nullptr, nullptr);

        if(m_RenderAPI == RenderAPI::OPENGL)
            glfwMakeContextCurrent(m_Handle);

		glfwSetWindowUserPointer(m_Handle, &m_Data);

		SetIcon("/CoreTextures/icon.png");

		glfwSetWindowPos(m_Handle, mode->width / 2 - ScreenWidth / 2, mode->height / 2 - ScreenHeight / 2);
		glfwSetInputMode(m_Handle, GLFW_STICKY_KEYS, GL_TRUE);

		// Set GLFW callbacks
		glfwSetWindowSizeCallback(m_Handle, [](GLFWwindow* window, int width, int height)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;

			WindowResizeEvent event(width, height);
			data.EventCallback(event);
		});

		glfwSetWindowCloseCallback(m_Handle, [](GLFWwindow* window)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			WindowCloseEvent event;
			data.EventCallback(event);
			data.Exit = true;
		});

		glfwSetWindowFocusCallback(m_Handle, [](GLFWwindow* window, int focused)
		{
			Input::GetInput().SetWindowFocus(focused);
		});

		glfwSetKeyCallback(m_Handle, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(GLFWToJMKeyboardKey(key), 0);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(GLFWToJMKeyboardKey(key));
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(GLFWToJMKeyboardKey(key), 1);
					data.EventCallback(event);
					break;
				}
			}
		});

		glfwSetMouseButtonCallback(m_Handle, [](GLFWwindow* window, int button, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(GLFWToJMMouseKey(button));
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(GLFWToJMMouseKey(button));
					data.EventCallback(event);
					break;
				}
			}
		});

		glfwSetScrollCallback(m_Handle, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseScrolledEvent event((float)xOffset, (float)yOffset);
			data.EventCallback(event);
		});

		glfwSetCursorPosCallback(m_Handle, [](GLFWwindow* window, double xPos, double yPos)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseMovedEvent event((float)xPos, (float)yPos);
			data.EventCallback(event);
		});

		glfwSetCursorEnterCallback(m_Handle, [](GLFWwindow* window, int enter)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseEnterEvent event(enter > 0);
			data.EventCallback(event);
		});

		glfwSetCharCallback(m_Handle, [](GLFWwindow* window, unsigned int keycode)
 		{
 			WindowData& data = *(WindowData*)(glfwGetWindowUserPointer(window));

  			KeyTypedEvent event(keycode);
 			data.EventCallback(event);
 		});

		LUMOS_CORE_INFO("Initialised GLFW version : {0}", glfwGetVersionString());

		return true;
	}

	void GLFWWindow::SetIcon(const String& file)
	{
		uint width, height;
		byte* pixels = Lumos::LoadImageFromFile(file, &width, &height, nullptr, true);

		GLFWimage image;
		image.height = height;
		image.width = width;
		image.pixels = static_cast<unsigned char*>(pixels);

		glfwSetWindowIcon(m_Handle, 1, &image);

		delete[] pixels;
	}

	void GLFWWindow::SetWindowTitle(const String& title)
	{
		glfwSetWindowTitle(m_Handle, title.c_str());
	}

	void GLFWWindow::ToggleVSync()
	{
		if (m_VSync)
		{
			SetVSync(false);
		}
		else
		{
			SetVSync(true);
		}
	}

	void GLFWWindow::SetVSync(bool set)
	{
		if (set)
		{
			m_VSync = true;
			glfwSwapInterval(1);
		}
		else
		{
			m_VSync = false;
			glfwSwapInterval(0);
		}
	}

	void GLFWWindow::OnUpdate()
	{
        if(m_RenderAPI == RenderAPI::OPENGL)
			glfwSwapBuffers(m_Handle);

		glfwPollEvents();
	}

	void GLFWWindow::SetBorderlessWindow(bool borderless)
	{
		if (borderless)
		{
			glfwWindowHint(GLFW_DECORATED, GL_FALSE);
		}
		else
		{
			glfwWindowHint(GLFW_DECORATED, GL_TRUE);
		}
	}

	void GLFWWindow::HideMouse(bool hide)
	{
		if (hide)
		{
			glfwSetInputMode(m_Handle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		}
		else
		{
			glfwSetInputMode(m_Handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}

	void GLFWWindow::SetMousePosition(const maths::Vector2& pos)
	{
		glfwSetCursorPos(m_Handle, pos.GetX(), pos.GetY());
	}

	int GLFWWindow::GLFWToJMKeyboardKey(int glfwKey)
	{
		switch (glfwKey)
		{
		case GLFW_KEY_A: return LUMOS_KEY_A;
		case GLFW_KEY_B: return LUMOS_KEY_B;
		case GLFW_KEY_C: return LUMOS_KEY_C;
		case GLFW_KEY_D: return LUMOS_KEY_D;
		case GLFW_KEY_E: return LUMOS_KEY_E;
		case GLFW_KEY_F: return LUMOS_KEY_F;
		case GLFW_KEY_G: return LUMOS_KEY_G;
		case GLFW_KEY_H: return LUMOS_KEY_H;
		case GLFW_KEY_I: return LUMOS_KEY_I;
		case GLFW_KEY_J: return LUMOS_KEY_J;
		case GLFW_KEY_K: return LUMOS_KEY_K;
		case GLFW_KEY_L: return LUMOS_KEY_L;
		case GLFW_KEY_M: return LUMOS_KEY_M;
		case GLFW_KEY_N: return LUMOS_KEY_N;
		case GLFW_KEY_O: return LUMOS_KEY_O;
		case GLFW_KEY_P: return LUMOS_KEY_P;
		case GLFW_KEY_Q: return LUMOS_KEY_Q;
		case GLFW_KEY_R: return LUMOS_KEY_R;
		case GLFW_KEY_S: return LUMOS_KEY_S;
		case GLFW_KEY_T: return LUMOS_KEY_T;
		case GLFW_KEY_U: return LUMOS_KEY_U;
		case GLFW_KEY_V: return LUMOS_KEY_V;
		case GLFW_KEY_W: return LUMOS_KEY_W;
		case GLFW_KEY_X: return LUMOS_KEY_X;
		case GLFW_KEY_Y: return LUMOS_KEY_Y;
		case GLFW_KEY_Z: return LUMOS_KEY_Z;

		case GLFW_KEY_0: return LUMOS_KEY_0;
		case GLFW_KEY_1: return LUMOS_KEY_1;
		case GLFW_KEY_2: return LUMOS_KEY_2;
		case GLFW_KEY_3: return LUMOS_KEY_3;
		case GLFW_KEY_4: return LUMOS_KEY_4;
		case GLFW_KEY_5: return LUMOS_KEY_5;
		case GLFW_KEY_6: return LUMOS_KEY_6;
		case GLFW_KEY_7: return LUMOS_KEY_7;
		case GLFW_KEY_8: return LUMOS_KEY_8;
		case GLFW_KEY_9: return LUMOS_KEY_9;

		case GLFW_KEY_KP_SUBTRACT: return LUMOS_KEY_SUBTRACT;
		case GLFW_KEY_DELETE: return LUMOS_KEY_DELETE;
		case GLFW_KEY_SPACE: return LUMOS_KEY_SPACE;
		case GLFW_KEY_LEFT: return LUMOS_KEY_LEFT;
		case GLFW_KEY_RIGHT: return LUMOS_KEY_RIGHT;
		case GLFW_KEY_UP: return LUMOS_KEY_UP;
		case GLFW_KEY_DOWN: return LUMOS_KEY_DOWN;
		case GLFW_KEY_LEFT_SHIFT: return LUMOS_KEY_LEFT_SHIFT;
		case GLFW_KEY_ESCAPE: return LUMOS_KEY_ESCAPE;
		case GLFW_KEY_KP_ADD: return LUMOS_KEY_ADD;
		case GLFW_KEY_COMMA: return LUMOS_KEY_COMMA;

		default: LUMOS_CORE_ERROR("Unsupported Key used : {0}", glfwKey); return 0;
		}
	}

	int GLFWWindow::GLFWToJMMouseKey(int glfwKey)
	{
		switch (glfwKey)
		{
		case GLFW_MOUSE_BUTTON_LEFT		: return LUMOS_MOUSE_LEFT;
		case GLFW_MOUSE_BUTTON_RIGHT	: return LUMOS_MOUSE_RIGHT;
		case GLFW_MOUSE_BUTTON_MIDDLE	: return LUMOS_MOUSE_MIDDLE;
		default: LUMOS_CORE_ERROR("Unsupported Key used : {0}", glfwKey); return 0;
		}
	}
}
