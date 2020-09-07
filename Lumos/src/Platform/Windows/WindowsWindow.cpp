#include "Precompiled.h"
#define NOMINMAX
#undef NOGDI
#include <Windows.h>
#include <Windowsx.h>
#define NOGDI

#ifndef GET_WHEEL_DELTA_WPARAM
#	define GET_WHEEL_DELTA_WPARAM(wParam) ((short)HIWORD(wParam))
#endif

#include "WindowsWindow.h"
#include "WindowsKeyCodes.h"
#include "Graphics/API/GraphicsContext.h"
#include "Core/Application.h"
#include "Core/OS/Input.h"
#include "Utilities/LoadImage.h"
#include "Events/ApplicationEvent.h"

#include <imgui/imgui.h>

extern "C" {
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

namespace Lumos
{

	EXTERN_C IMAGE_DOS_HEADER __ImageBase;

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#ifndef HID_USAGE_PAGE_GENERIC
#	define HID_USAGE_PAGE_GENERIC ((USHORT)0x01)
#endif

#ifndef HID_USAGE_GENERIC_MOUSE
#	define HID_USAGE_GENERIC_MOUSE ((USHORT)0x02)
#endif

#ifndef HID_USAGE_GENERIC_KEYBOARD
#	define HID_USAGE_GENERIC_KEYBOARD ((USHORT)0x06)
#endif

	WindowsWindow::WindowsWindow(const WindowProperties& properties)
		: hWnd(nullptr)
	{
		m_Init = false;
		m_VSync = properties.VSync;
		SetHasResized(true);
		m_Data.m_RenderAPI = static_cast<Graphics::RenderAPI>(properties.RenderAPI);

		m_Init = Init(properties);
		
		Graphics::GraphicsContext::SetRenderAPI(static_cast<Graphics::RenderAPI>(properties.RenderAPI));
		Graphics::GraphicsContext::Create(properties, this);
		Graphics::GraphicsContext::GetContext()->Init();
	}

	WindowsWindow::~WindowsWindow()
	{
		Graphics::GraphicsContext::Release();
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

	static HICON createIcon(unsigned char* image, int width, int height, int xhot, int yhot, bool icon)
	{
		int i;
		HDC dc;
		HICON handle;
		HBITMAP color, mask;
		BITMAPV5HEADER bi;
		ICONINFO ii;
		unsigned char* target = NULL;
		unsigned char* source = image;

		ZeroMemory(&bi, sizeof(bi));
		bi.bV5Size = sizeof(bi);
		bi.bV5Width = width;
		bi.bV5Height = -height;
		bi.bV5Planes = 1;
		bi.bV5BitCount = 32;
		bi.bV5Compression = BI_BITFIELDS;
		bi.bV5RedMask = 0x00ff0000;
		bi.bV5GreenMask = 0x0000ff00;
		bi.bV5BlueMask = 0x000000ff;
		bi.bV5AlphaMask = 0xff000000;

		dc = GetDC(NULL);
		color = CreateDIBSection(dc,
			(BITMAPINFO*)&bi,
			DIB_RGB_COLORS,
			(void**)&target,
			NULL,
			(DWORD)0);
		ReleaseDC(NULL, dc);

		if(!color)
		{
			Debug::Log::Error("Win32: Failed to create RGBA bitmap");
			return NULL;
		}

		mask = CreateBitmap(width, height, 1, 1, NULL);
		if(!mask)
		{
			Debug::Log::Error("Win32: Failed to create mask bitmap");
			DeleteObject(color);
			return NULL;
		}

		for(i = 0; i < width * height; i++)
		{
			target[0] = source[2];
			target[1] = source[1];
			target[2] = source[0];
			target[3] = source[3];
			target += 4;
			source += 4;
		}

		ZeroMemory(&ii, sizeof(ii));
		ii.fIcon = icon;
		ii.xHotspot = xhot;
		ii.yHotspot = yhot;
		ii.hbmMask = mask;
		ii.hbmColor = color;

		handle = CreateIconIndirect(&ii);

		DeleteObject(color);
		DeleteObject(mask);

		if(!handle)
		{
			if(icon)
			{
				Debug::Log::Error("Win32: Failed to create icon");
			}
			else
			{
				Debug::Log::Error("Win32: Failed to create cursor");
			}
		}

		return handle;
	}

	bool WindowsWindow::Init(const WindowProperties& properties)
	{
		m_Data.Title = properties.Title;
		m_Data.Width = properties.Width;
		m_Data.Height = properties.Height;
		m_Data.Exit = false;

		hInstance = reinterpret_cast<HINSTANCE>(&__ImageBase);

		WNDCLASSEXA winClass = {};
		winClass.cbSize = sizeof(WNDCLASSEX);
		winClass.hInstance = hInstance;
		winClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		winClass.lpfnWndProc = static_cast<WNDPROC>(WndProc);
		winClass.lpszClassName = properties.Title.c_str();

		winClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		winClass.hCursor = LoadCursorA(nullptr, IDC_ARROW);
		winClass.hIcon = winClass.hIconSm = LoadIconA(nullptr, IDI_APPLICATION);
		winClass.lpszMenuName = nullptr;
		winClass.style = CS_VREDRAW | CS_HREDRAW;

		DWORD style = WS_POPUP;
		if(!properties.Fullscreen)
			style = WS_SYSMENU | WS_CAPTION | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX;

		if(!properties.Borderless)
			style |= WS_BORDER;

		if(!RegisterClassExA(&winClass))
		{
			LUMOS_LOG_CRITICAL("Could not register Win32 class!");
			return false;
		}

		RECT size = {0, 0, (LONG)properties.Width, (LONG)properties.Height};
		AdjustWindowRectEx(&size, style, false, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);

		m_Data.Width = size.right - size.left;
		m_Data.Height = size.bottom - size.top;

		int windowLeft = (GetSystemMetrics(SM_CXSCREEN) - m_Data.Width) / 2;
		int windowTop = (GetSystemMetrics(SM_CYSCREEN) - m_Data.Height) / 2;

		if(properties.Fullscreen)
		{
			windowLeft = 0;
			windowTop = 0;
		}

		hWnd = CreateWindow(winClass.lpszClassName, properties.Title.c_str(), style, windowLeft, windowTop, m_Data.Width, m_Data.Height, NULL, NULL, hInstance, NULL);

		if(!hWnd)
		{
			LUMOS_LOG_CRITICAL("Could not create window!");
			return false;
		}

		hDc = GetDC(hWnd);
		PIXELFORMATDESCRIPTOR pfd = GetPixelFormat();
		i32 pixelFormat = ChoosePixelFormat(hDc, &pfd);
		if(pixelFormat)
		{
			if(!SetPixelFormat(hDc, pixelFormat, &pfd))
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

		SetIcon("/Textures/icon.png", "/Textures/icon32.png");

		ShowWindow(hWnd, SW_SHOW);
		SetFocus(hWnd);
		SetWindowTitle(properties.Title);

		RECT clientRect;
		GetClientRect(hWnd, &clientRect);

		UpdateWindow(hWnd);

		MoveWindow(hWnd, windowLeft, windowTop, m_Data.Width, m_Data.Height, TRUE);

		GetClientRect(hWnd, &clientRect);
		int w = clientRect.right - clientRect.left;
		int h = clientRect.bottom - clientRect.top;

		m_Data.Height = h;
		m_Data.Width = w;

		if(!properties.ShowConsole)
		{
			HWND consoleWindow = GetConsoleWindow();

			ShowWindow(consoleWindow, SW_HIDE);

			SetActiveWindow(hWnd);
		}
		else
		{
			HWND consoleWindow = GetConsoleWindow();

			ShowWindow(consoleWindow, SW_SHOW);
		}

		if(properties.Fullscreen)
		{
			DEVMODE dm;
			memset(&dm, 0, sizeof(dm));
			dm.dmSize = sizeof(dm);
			// use default values from current setting
			EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);
			/*m_data->m_oldScreenWidth = dm.dmPelsWidth;
			m_data->m_oldHeight = dm.dmPelsHeight;
			m_data->m_oldBitsPerPel = dm.dmBitsPerPel;*/

			dm.dmPelsWidth = m_Data.Width;
			dm.dmPelsHeight = m_Data.Height;
			/*if (colorBitsPerPixel)
			{
				dm.dmBitsPerPel = colorBitsPerPixel;
			}*/
			dm.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

			LONG res = ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
			if(res != DISP_CHANGE_SUCCESSFUL)
			{ // try again without forcing display frequency
				dm.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
				res = ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
			}
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
		WindowsWindow::WindowData& data = static_cast<WindowsWindow*>(window)->m_Data;

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

		while(PeekMessage(&message, NULL, NULL, NULL, PM_REMOVE) > 0)
		{
			if(message.message == WM_QUIT)
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

	void WindowsWindow::SetWindowTitle(const std::string& title)
	{
		SetWindowText(hWnd, title.c_str());
	}

	void WindowsWindow::ToggleVSync()
	{
		if(m_VSync)
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
		WindowsWindow::WindowData& data = static_cast<WindowsWindow*>(window)->m_Data;
		HWND hWnd = static_cast<WindowsWindow*>(window)->GetHWND();

		bool down = false;
		Lumos::InputCode::MouseKey mouseKey; 
		switch(button)
		{
		case WM_LBUTTONDOWN:
			SetCapture(hWnd);
			mouseKey = Lumos::InputCode::MouseKey::ButtonLeft;
			down = true;
			break;
		case WM_LBUTTONUP:
			ReleaseCapture();
			mouseKey = Lumos::InputCode::MouseKey::ButtonLeft;
			down = false;
			break;
		case WM_RBUTTONDOWN:
			SetCapture(hWnd);
			mouseKey = Lumos::InputCode::MouseKey::ButtonRight;
			down = true;
			break;
		case WM_RBUTTONUP:
			ReleaseCapture();
			mouseKey = Lumos::InputCode::MouseKey::ButtonRight;
			down = false;
			break;
		case WM_MBUTTONDOWN:
			SetCapture(hWnd);
			mouseKey = Lumos::InputCode::MouseKey::ButtonMiddle;
			down = true;
			break;
		case WM_MBUTTONUP:
			ReleaseCapture();
			mouseKey = Lumos::InputCode::MouseKey::ButtonMiddle;
			down = false;
			break;
		}

		if(down)
		{
			MouseButtonPressedEvent event(mouseKey);
			data.EventCallback(event);
		}
		else
		{
			MouseButtonReleasedEvent event(mouseKey);
			data.EventCallback(event);
		}
	}

	void MouseScrollCallback(Window* window, int inSw, WPARAM wParam, LPARAM lParam)
	{
		WindowsWindow::WindowData& data = static_cast<WindowsWindow*>(window)->m_Data;

		MouseScrolledEvent event(0.0f, static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA));
		data.EventCallback(event);
	}

	void MouseMoveCallback(Window* window, i32 x, i32 y)
	{
		WindowsWindow::WindowData& data = static_cast<WindowsWindow*>(window)->m_Data;
		MouseMovedEvent event(static_cast<float>(x), static_cast<float>(y));
		data.EventCallback(event);
	}

	void CharCallback(Window* window, i32 key, i32 flags, UINT message)
	{
		WindowsWindow::WindowData& data = static_cast<WindowsWindow*>(window)->m_Data;

		KeyTypedEvent event(Lumos::InputCode::Key{} , char(key));
		data.EventCallback(event);
	}

	void KeyCallback(Window* window, i32 key, i32 flags, UINT message)
	{
		bool pressed = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
		bool repeat = (flags >> 30) & 1;

		WindowsWindow::WindowData& data = static_cast<WindowsWindow*>(window)->m_Data;

		if(pressed)
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

	static void ImGuiUpdateMousePos(HWND hWnd)
	{
		ImGuiIO& io = ImGui::GetIO();

		// Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
		if(io.WantSetMousePos)
		{
			POINT pos = {(int)io.MousePos.x, (int)io.MousePos.y};
			::ClientToScreen(hWnd, &pos);
			::SetCursorPos(pos.x, pos.y);
		}

		// Set mouse position
		io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
		POINT pos;
		if(HWND active_window = ::GetForegroundWindow())
			if(active_window == hWnd || ::IsChild(active_window, hWnd))
				if(::GetCursorPos(&pos) && ::ScreenToClient(hWnd, &pos))
					io.MousePos = ImVec2((float)pos.x, (float)pos.y);
	}

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		LRESULT result = NULL;
		Window* window = Application::Get().GetWindow();
		if(window == nullptr)
			return DefWindowProc(hWnd, message, wParam, lParam);

		switch(message)
		{
		case WM_ACTIVATE: {
			if(!HIWORD(wParam)) // Is minimized
			{
				// active
			}
			else
			{
				// inactive
			}

			return 0;
		}
		case WM_SYSCOMMAND: {
			switch(wParam)
			{
			case SC_SCREENSAVE:
			case SC_MONITORPOWER:
				return 0;
			}
			result = DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
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
			break;
		case WM_SETCURSOR:
			ImGuiUpdateMousePos(hWnd);
			break;
		default:
			result = DefWindowProc(hWnd, message, wParam, lParam);
			break;
		}
		return result;
	}

	void WindowsWindow::UpdateCursorImGui()
	{
		//ImGuiUpdateMousePos(hWnd);

		ImGuiIO& io = ImGui::GetIO();
		ImGuiMouseCursor imgui_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();

		if(io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
			return;

		if(imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
		{
			// Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
			::SetCursor(NULL);
		}
		else
		{
			// Show OS mouse cursor
			LPTSTR win32_cursor = IDC_ARROW;
			switch(imgui_cursor)
			{
			case ImGuiMouseCursor_Arrow:
				win32_cursor = IDC_ARROW;
				break;
			case ImGuiMouseCursor_TextInput:
				win32_cursor = IDC_IBEAM;
				break;
			case ImGuiMouseCursor_ResizeAll:
				win32_cursor = IDC_SIZEALL;
				break;
			case ImGuiMouseCursor_ResizeEW:
				win32_cursor = IDC_SIZEWE;
				break;
			case ImGuiMouseCursor_ResizeNS:
				win32_cursor = IDC_SIZENS;
				break;
			case ImGuiMouseCursor_ResizeNESW:
				win32_cursor = IDC_SIZENESW;
				break;
			case ImGuiMouseCursor_ResizeNWSE:
				win32_cursor = IDC_SIZENWSE;
				break;
			case ImGuiMouseCursor_Hand:
				win32_cursor = IDC_HAND;
				break;
			}
			::SetCursor(::LoadCursor(NULL, win32_cursor));
		}
	}

	void WindowsWindow::SetIcon(const std::string& filePath, const std::string& smallIconFilePath)
	{
		HICON bigIcon = NULL;
		HICON smallIcon = NULL;

		if(filePath != "")
		{
			u32 width, height;
			u8* pixels = Lumos::LoadImageFromFile(filePath, &width, &height, nullptr, nullptr, true);

			bigIcon = createIcon(pixels, int(width), int(height), 0, 0, true);
			delete[] pixels;
		}

		if(smallIconFilePath != "")
		{
			u32 width, height;
			u8* pixels = Lumos::LoadImageFromFile(smallIconFilePath, &width, &height, nullptr, nullptr, true);

			auto smallIcon = createIcon(pixels, int(width), int(height), 0, 0, true);
			delete[] pixels;
		}

		if(!smallIcon)
			smallIcon = bigIcon;

		SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)bigIcon);
		SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)smallIcon);

		if(m_BigIcon)
			DestroyIcon(m_BigIcon);

		if(m_SmallIcon)
			DestroyIcon(m_SmallIcon);

		m_BigIcon = bigIcon;
		m_SmallIcon = smallIcon;
	}

	void WindowsWindow::MakeDefault()
	{
		CreateFunc = CreateFuncWindows;
	}

	Window* WindowsWindow::CreateFuncWindows(const WindowProperties& properties)
	{
		return new WindowsWindow(properties);
	}
}
