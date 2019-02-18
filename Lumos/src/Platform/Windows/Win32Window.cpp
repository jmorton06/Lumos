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
		
		//Input
		rid.usUsagePage = HID_USAGE_PAGE_GENERIC;
		rid.usUsage = HID_USAGE_GENERIC_KEYBOARD;
		rid.dwFlags = RIDEV_INPUTSINK;
		rid.hwndTarget = hWnd;
		RegisterRawInputDevices(&rid, 1, sizeof(rid));

		return true;
	}

	void Win32Window::OnUpdate()
	{
		MSG message;
	/*	while (PeekMessage(&message, NULL, NULL, NULL, PM_REMOVE) > 0)
		{
			if (message.message == WM_QUIT)
			{
				SetExit(true);
				return;
			}
			TranslateMessage(&message);
			DispatchMessage(&message);
		}*/

		//UINT dwSize;
		//GetRawInputData((HRAWINPUT)message.lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
		//BYTE *lpb = new BYTE[dwSize];

		//GetRawInputData((HRAWINPUT)message.lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));
		//RAWINPUT *raw = (RAWINPUT *)lpb;

		//RAWINPUT* rw = (RAWINPUT*)raw;
		//DWORD key = (DWORD)rw->data.keyboard.VKey;

		//// We should do bounds checking!
		//if (key < 0 || key > 1048)
		//{
		//	return;
		//}

		//m_KeyPressed[Win32KeyToLumos(key)] = !(rw->data.keyboard.Flags & RI_KEY_BREAK);

		::SwapBuffers(hDc);
	}

	void Win32Window::SetWindowTitle(const String& title)
	{
		SetWindowText(hWnd, title.c_str());
	}

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		LRESULT result = NULL;
		Window* window = Application::Instance()->GetWindow();
		if (window == nullptr)
			return DefWindowProc(hWnd, message, wParam, lParam);
		else
			return result;
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
}