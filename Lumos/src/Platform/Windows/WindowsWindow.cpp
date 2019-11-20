#include "lmpch.h"																									
#define NOMINMAX
#undef NOGDI
#include <Windows.h>
#include <Windowsx.h>
#define NOGDI

#ifndef GET_WHEEL_DELTA_WPARAM
#define GET_WHEEL_DELTA_WPARAM(wParam)  ((short)HIWORD(wParam))
#endif

#include "WindowsWindow.h"
#include "WindowsKeyCodes.h"
#include "Graphics/API/GraphicsContext.h"
#include "App/Application.h"
#include "Core/OS/Input.h"
#include "Events/ApplicationEvent.h"

#include <imgui/imgui.h>

extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

namespace Lumos
{

	EXTERN_C IMAGE_DOS_HEADER __ImageBase;

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC ((USHORT)0x01)
#endif

#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE ((USHORT)0x02)
#endif

#ifndef HID_USAGE_GENERIC_KEYBOARD
#define HID_USAGE_GENERIC_KEYBOARD ((USHORT)0x06)
#endif

	static ImGuiMouseCursor g_LastMouseCursor = ImGuiMouseCursor_COUNT;

	WindowsWindow::WindowsWindow(const WindowProperties& properties) : hWnd(nullptr)
	{
		m_Init = false;
		m_VSync = properties.VSync;
		SetHasResized(true);
		m_Data.m_RenderAPI = static_cast<Graphics::RenderAPI>(properties.RenderAPI);

		m_Init = Init(properties);

		Graphics::GraphicsContext::Create(properties, hWnd);
	}

	WindowsWindow::~WindowsWindow()
	{
	}

	static PIXELFORMATDESCRIPTOR GetPixelFormat()
	{
		PIXELFORMATDESCRIPTOR result = {};
		result.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		result.nVersion = 1;
		result.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		result.iPixelType = PFD_TYPE_RGBA;
		result.cColorBits = 32;
		result.cDepthBits = 24;
		result.cStencilBits = 8;
		result.cAuxBuffers = 0;
		result.iLayerType = PFD_MAIN_PLANE;
		return result;
	}

	bool WindowsWindow::Init(const WindowProperties& properties)
	{
		m_Data.Title = properties.Title;
		m_Data.Width = properties.Width;
		m_Data.Height = properties.Height;
		m_Data.Exit = false;

		hInstance = reinterpret_cast<HINSTANCE>(&__ImageBase);

		WNDCLASS winClass = {};
		winClass.hInstance = hInstance; // GetModuleHandle(0);
		winClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		winClass.lpfnWndProc = static_cast<WNDPROC>(WndProc);
		winClass.lpszClassName = properties.Title.c_str();
		winClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		winClass.hIcon = LoadIcon(NULL, IDI_WINLOGO);

		if (!RegisterClassA(&winClass))
		{
			LUMOS_LOG_CRITICAL("Could not register Win32 class!");
			return false;
		}

		RECT size = { 0, 0, (LONG)properties.Width, (LONG)properties.Height };
		AdjustWindowRectEx(&size, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, false, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);

		hWnd = CreateWindowExA(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
			winClass.lpszClassName, properties.Title.c_str(),
			WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			GetSystemMetrics(SM_CXSCREEN) / 2 - properties.Width / 2,
			GetSystemMetrics(SM_CYSCREEN) / 2 - properties.Height / 2,

			size.right + (-size.left), size.bottom + (-size.top), NULL, NULL, hInstance, NULL);

		if (!hWnd)
		{
			LUMOS_LOG_CRITICAL("Could not create window!");
			return false;
		}

		hDc = GetDC(hWnd);
		PIXELFORMATDESCRIPTOR pfd = GetPixelFormat();
		i32 pixelFormat = ChoosePixelFormat(hDc, &pfd);
		if (pixelFormat)
		{
			if (!SetPixelFormat(hDc, pixelFormat, &pfd))
			{
				LUMOS_LOG_CRITICAL("Failed setting pixel format!");
				return false;
			}
		}
		else
		{
			LUMOS_LOG_CRITICAL("Failed choosing pixel format!");
			return false;
		}

		ShowWindow(hWnd, SW_SHOW);
		SetFocus(hWnd);
		SetWindowTitle(properties.Title);

		if(!properties.ShowConsole)
		{
			HWND consoleWindow = GetConsoleWindow();

			ShowWindow(consoleWindow, SW_HIDE);

			SetActiveWindow(hWnd);
		}
		
		//Input
		rid.usUsagePage = HID_USAGE_PAGE_GENERIC;
		rid.usUsage = HID_USAGE_GENERIC_KEYBOARD;
		rid.dwFlags = RIDEV_INPUTSINK;
		rid.hwndTarget = hWnd;
		RegisterRawInputDevices(&rid, 1, sizeof(rid));

		return true;
	}

	void ResizeCallback(Window* window, i32 width, i32 height)
	{
		WindowsWindow::WindowData data = static_cast<WindowsWindow*>(window)->m_Data;

		data.Width = width;
		data.Height = height;

		WindowResizeEvent event(width, height);
		data.EventCallback(event);
	}

	void FocusCallback(Window* window, bool focused)
	{
		Input::GetInput()->SetWindowFocus(focused);
	}

	void WindowsWindow::OnUpdate()
	{
		MSG message;

		ZeroMemory(&message, sizeof(MSG));

		while (PeekMessage(&message, NULL, NULL, NULL, PM_REMOVE) > 0)
		{
			if (message.message == WM_QUIT)
			{
				WindowCloseEvent event;
				m_Data.EventCallback(event);
				m_Data.Exit = true;
				return;
			}
			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		::SwapBuffers(hDc);
	}

	void WindowsWindow::SetWindowTitle(const String& title)
	{
		SetWindowText(hWnd, title.c_str());
	}

	void WindowsWindow::ToggleVSync()
	{
		if (m_VSync)
		{
			m_VSync = false;
			//glfwSwapInterval(0);
		}
		else
		{
			m_VSync = true;
			//glfwSwapInterval(1);
		}
	}

	void WindowsWindow::SetBorderlessWindow(bool borderless)
	{

	}

	void MouseButtonCallback(Window* window, i32 button, i32 x, i32 y)
	{
		WindowsWindow::WindowData data = static_cast<WindowsWindow*>(window)->m_Data;
		HWND hWnd = static_cast<WindowsWindow*>(window)->GetHWND();

		bool down = false;
		switch (button)
		{
		case WM_LBUTTONDOWN:
			SetCapture(hWnd);
			button = LUMOS_MOUSE_LEFT;
			down = true;
			break;
		case WM_LBUTTONUP:
			ReleaseCapture();
			button = LUMOS_MOUSE_LEFT;
			down = false;
			break;
		case WM_RBUTTONDOWN:
			SetCapture(hWnd);
			button = LUMOS_MOUSE_RIGHT;
			down = true;
			break;
		case WM_RBUTTONUP:
			ReleaseCapture();
			button = LUMOS_MOUSE_RIGHT;
			down = false;
			break;
		case WM_MBUTTONDOWN:
			SetCapture(hWnd);
			button = LUMOS_MOUSE_MIDDLE;
			down = true;
			break;
		case WM_MBUTTONUP:
			ReleaseCapture();
			button = LUMOS_MOUSE_MIDDLE;
			down = false;
			break;
		}

		if (down)
		{
			MouseButtonPressedEvent event(button);
			data.EventCallback(event);
		}
		else
		{
			MouseButtonReleasedEvent event(button);
			data.EventCallback(event);
		}
	}

	void MouseScrollCallback(Window* window, int inSw, WPARAM wParam, LPARAM lParam)
	{
		WindowsWindow::WindowData data = static_cast<WindowsWindow*>(window)->m_Data;

		MouseScrolledEvent event(0.0f, static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA));
		data.EventCallback(event);
	}

	void MouseMoveCallback(Window* window, i32 x, i32 y)
	{
		WindowsWindow::WindowData data = static_cast<WindowsWindow*>(window)->m_Data;
		MouseMovedEvent event(static_cast<float>(x), static_cast<float>(y));
		data.EventCallback(event);
	}

	void CharCallback(Window* window, i32 key, i32 flags, UINT message)
	{
		WindowsWindow::WindowData data = static_cast<WindowsWindow*>(window)->m_Data;

		KeyTypedEvent event(key);// WindowsKeyCodes::WindowsKeyToLumos(key)); //TODO : FIX
		data.EventCallback(event);
	}

	void KeyCallback(Window* window, i32 key, i32 flags, UINT message)
	{
		bool pressed = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
		bool repeat = (flags >> 30) & 1;

		WindowsWindow::WindowData data = static_cast<WindowsWindow*>(window)->m_Data;
	
		if (pressed)
		{
			KeyPressedEvent event(WindowsKeyCodes::WindowsKeyToLumos(key), repeat ? 1 : 0);
			data.EventCallback(event);
		}
		else
		{
			KeyReleasedEvent event(WindowsKeyCodes::WindowsKeyToLumos(key));
			data.EventCallback(event);
		}
	}

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		LRESULT result = NULL;
		Window* window = Application::Instance()->GetWindow(); //Window::GetWindowClass(hWnd);
		if (window == nullptr)
			return DefWindowProc(hWnd, message, wParam, lParam);

		switch (message)
		{
		case WM_ACTIVATE:
		{
			if (!HIWORD(wParam)) // Is minimized
			{
				// active
			}
			else
			{
				// inactive
			}

			return 0;
		}
		case WM_SYSCOMMAND:
		{
			switch (wParam)
			{
			case SC_SCREENSAVE:
			case SC_MONITORPOWER:
				return 0;
			}
			result = DefWindowProc(hWnd, message, wParam, lParam);
		} break;
		case WM_SETFOCUS:
			FocusCallback(window, true);
			break;
		case WM_KILLFOCUS:
			FocusCallback(window, false);
			break;
		case WM_CLOSE:
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
			KeyCallback(window, i32(wParam), i32(lParam), message);
			break;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
			MouseButtonCallback(window, message, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			break;
		case WM_MOUSEMOVE:
			MouseMoveCallback(window, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			break;
		case WM_MOUSEWHEEL:
			MouseScrollCallback(window, WM_VSCROLL, wParam, lParam);
			break;
		case WM_SIZE:
			ResizeCallback(window, LOWORD(lParam), HIWORD(lParam));
			break;
		case WM_CHAR:
		case WM_SYSCHAR:
		case WM_UNICHAR:
			CharCallback(window, i32(wParam), i32(lParam), message);
		default:
			result = DefWindowProc(hWnd, message, wParam, lParam);
		}
		return result;
	}

	static void ImGuiUpdateMousePos(HWND hWnd)
	{
		ImGuiIO& io = ImGui::GetIO();

		// Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
		if (io.WantSetMousePos)
		{
			POINT pos = { (int)io.MousePos.x, (int)io.MousePos.y };
			::ClientToScreen(hWnd, &pos);
			::SetCursorPos(pos.x, pos.y);
		}

		// Set mouse position
		io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
		POINT pos;
		if (HWND active_window = ::GetForegroundWindow())
			if (active_window == hWnd || ::IsChild(active_window, hWnd))
				if (::GetCursorPos(&pos) && ::ScreenToClient(hWnd, &pos))
					io.MousePos = ImVec2((float)pos.x, (float)pos.y);
	}

	void WindowsWindow::UpdateCursorImGui()
	{
		ImGuiUpdateMousePos(hWnd);

		ImGuiIO& io = ImGui::GetIO();
		ImGuiMouseCursor imgui_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
		if (g_LastMouseCursor != imgui_cursor)
		{
			g_LastMouseCursor = imgui_cursor;

			if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
				return;

			if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
			{
				// Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
				::SetCursor(NULL);
			}
			else
			{
				// Show OS mouse cursor
				LPTSTR win32_cursor = IDC_ARROW;
				switch (imgui_cursor)
				{
				case ImGuiMouseCursor_Arrow:        win32_cursor = IDC_ARROW; break;
				case ImGuiMouseCursor_TextInput:    win32_cursor = IDC_IBEAM; break;
				case ImGuiMouseCursor_ResizeAll:    win32_cursor = IDC_SIZEALL; break;
				case ImGuiMouseCursor_ResizeEW:     win32_cursor = IDC_SIZEWE; break;
				case ImGuiMouseCursor_ResizeNS:     win32_cursor = IDC_SIZENS; break;
				case ImGuiMouseCursor_ResizeNESW:   win32_cursor = IDC_SIZENESW; break;
				case ImGuiMouseCursor_ResizeNWSE:   win32_cursor = IDC_SIZENWSE; break;
				case ImGuiMouseCursor_Hand:         win32_cursor = IDC_HAND; break;
				}
				::SetCursor(::LoadCursor(NULL, win32_cursor));
			}
		}

	}

	void WindowsWindow::MakeDefault()
	{
		CreateFunc = CreateFuncWindows;
	}

	Window* WindowsWindow::CreateFuncWindows(const WindowProperties& properties)
	{
		return lmnew WindowsWindow(properties);
	}
}
