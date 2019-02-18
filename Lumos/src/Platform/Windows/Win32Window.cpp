#include "LM.h"																									
#define NOMINMAX
#undef NOGDI
#include <Windows.h>
#include <Windowsx.h>
#define NOGDI

#include "System/LMLog.h"
#include "Win32Window.h"
#include "Graphics/API/Context.h"
#include "App/Application.h"
#include "App/Input.h"

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

	Win32Window::Win32Window(const WindowProperties& properties, const String& title, RenderAPI api)
	{
		m_Init = false;
		m_VSync = properties.VSync;
		m_Timer = new Timer();
		SetHasResized(true);
		m_Data.m_RenderAPI = api;

		m_Init = Init(properties, title);

		graphics::Context::Create(properties, hWnd);
	}

	Win32Window::~Win32Window()
	{
		if (m_Timer != nullptr)
		{
			delete m_Timer;
			m_Timer = nullptr;
		}
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

	bool Win32Window::Init(const WindowProperties& properties, const String& title)
	{
		m_Data.Title = title;
		m_Data.Width = properties.Width;
		m_Data.Height = properties.Height;
		m_Data.Exit = false;

		hInstance = (HINSTANCE)&__ImageBase;

		WNDCLASS winClass = {};
		winClass.hInstance = hInstance; // GetModuleHandle(0);
		winClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		winClass.lpfnWndProc = (WNDPROC)WndProc;
		winClass.lpszClassName = title.c_str();
		winClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		winClass.hIcon = LoadIcon(NULL, IDI_WINLOGO);

		if (!RegisterClassA(&winClass))
		{
			LUMOS_CORE_ERROR("Could not register Win32 class!");
			return false;
		}

		RECT size = { 0, 0, (LONG)properties.Width, (LONG)properties.Height };
		AdjustWindowRectEx(&size, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, false, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);

		hWnd = CreateWindowExA(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
			winClass.lpszClassName, title.c_str(),
			WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			GetSystemMetrics(SM_CXSCREEN) / 2 - properties.Width / 2,
			GetSystemMetrics(SM_CYSCREEN) / 2 - properties.Height / 2,

			size.right + (-size.left), size.bottom + (-size.top), NULL, NULL, hInstance, NULL);

		if (!hWnd)
		{
			LUMOS_CORE_ERROR("Could not create window!");
			return false;
		}

		//RegisterWindowClass(hWnd, this);

		hDc = GetDC(hWnd);
		PIXELFORMATDESCRIPTOR pfd = GetPixelFormat();
		int32 pixelFormat = ChoosePixelFormat(hDc, &pfd);
		if (pixelFormat)
		{
			if (!SetPixelFormat(hDc, pixelFormat, &pfd))
			{
				LUMOS_CORE_ERROR("Failed setting pixel format!");
				return false;
			}
		}
		else
		{
			LUMOS_CORE_ERROR("Failed choosing pixel format!");
			return false;
		}

		ShowWindow(hWnd, SW_SHOW);
		SetFocus(hWnd);
		SetWindowTitle(title);
		
		//Input
		rid.usUsagePage = HID_USAGE_PAGE_GENERIC;
		rid.usUsage = HID_USAGE_GENERIC_KEYBOARD;
		rid.dwFlags = RIDEV_INPUTSINK;
		rid.hwndTarget = hWnd;
		RegisterRawInputDevices(&rid, 1, sizeof(rid));

		return true;
	}

	void ResizeCallback(Window* window, int32 width, int32 height)
	{
		Win32Window::WindowData data = ((Win32Window*)window)->m_Data;

		data.Width = width;
		data.Height = height;

		WindowResizeEvent event(width, height);
		data.EventCallback(event);
	}

	void FocusCallback(Window* window, bool focused)
	{
	}

	void Win32Window::OnUpdate()
	{
		MSG message;
		while (PeekMessage(&message, NULL, NULL, NULL, PM_REMOVE) > 0)
		{
			if (message.message == WM_QUIT)
			{
				SetExit(true);
				return;
			}
			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		UINT dwSize;
		GetRawInputData((HRAWINPUT)message.lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
		BYTE *lpb = new BYTE[dwSize];

		GetRawInputData((HRAWINPUT)message.lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));
		RAWINPUT *raw = (RAWINPUT *)lpb;

		RAWINPUT* rw = (RAWINPUT*)raw;
		DWORD key = (DWORD)rw->data.keyboard.VKey;

		// We should do bounds checking!
		if (key < 0 || key > 1048)
		{
			return;
		}

		//m_KeyPressed[Win32KeyToLumos(key)] = !(rw->data.keyboard.Flags & RI_KEY_BREAK);

		::SwapBuffers(hDc);
	}

	void Win32Window::SetWindowTitle(const String& title)
	{
		SetWindowText(hWnd, title.c_str());
	}

	void Win32Window::ToggleVSync()
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

	void Win32Window::SetBorderlessWindow(bool borderless)
	{

	}

	void MouseButtonCallback(Window* window, int32 button, int32 x, int32 y)
	{

	}
	void KeyCallback(Window* window, int32 flags, int32 key, uint message)
	{

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
			KeyCallback(window, lParam, wParam, message);
			break;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
			MouseButtonCallback(window, message, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			break;
		case WM_SIZE:
			ResizeCallback(window, LOWORD(lParam), HIWORD(lParam));
			break;
		default:
			result = DefWindowProc(hWnd, message, wParam, lParam);
		}
		return result;
	}
}