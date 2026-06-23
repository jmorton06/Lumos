#include "Precompiled.h"
#include "WindowsOS.h"
#include "WindowsPower.h"
#include "WindowsWindow.h"
#include "Core/CoreSystem.h"
#include "Core/OS/MemoryManager.h"
#include "Core/Application.h"
#include "Maths/Vector4.h"
#include <Windows.h>

#ifdef LUMOS_USE_GLFW_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32

#include "Platform/GLFW/GLFWWindow.h"
#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>
#endif

#include <filesystem>
#include <shellapi.h>
#include <dwmapi.h>
#include <winuser.h>

extern Lumos::Application* Lumos::CreateApplication();

namespace Lumos
{
    void WindowsOS::Run()
    {
        auto power       = WindowsPower();
        auto percentage  = power.GetPowerPercentageLeft();
        auto secondsLeft = power.GetPowerSecondsLeft();
        auto state       = power.GetPowerState();

        LINFO("--------------------");
        LINFO(" System Information ");
        LINFO("--------------------");

        if(state != PowerState::POWERSTATE_NO_BATTERY)
            LINFO("Battery Info - Percentage : %d , Time Left %ds , State : %s", percentage, secondsLeft, PowerStateToString(state).c_str());
        else
            LINFO("Power - Outlet");

        auto systemInfo = MemoryManager::Get().GetSystemInfo();
        systemInfo.Log();

        auto& app = Lumos::Application::Get();
        app.Init();
        app.Run();
        app.Release();
    }

    void WindowsOS::Init()
    {
#ifdef LUMOS_USE_GLFW_WINDOWS
        GLFWWindow::MakeDefault();
#else
        WindowsWindow::MakeDefault();
#endif
    }

    SystemMemoryInfo MemoryManager::GetSystemInfo()
    {
        MEMORYSTATUSEX status;
        status.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&status);

        SystemMemoryInfo result = {
            (int64_t)status.ullAvailPhys,
            (int64_t)status.ullTotalPhys,

            (int64_t)status.ullAvailVirtual,
            (int64_t)status.ullTotalVirtual
        };
        return result;
    }

    std::string WindowsOS::GetExecutablePath()
    {
        WCHAR path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);

        std::string convertedString = std::filesystem::path(path).string();
        std::replace(convertedString.begin(), convertedString.end(), '\\', '/');

        return convertedString;
    }

    void WindowsOS::OpenFileLocation(const std::string& path)
    {
        std::filesystem::path fsPath = path;
        ShellExecuteA(NULL, "open", std::filesystem::is_directory(fsPath) ? path.c_str() : fsPath.parent_path().string().c_str(), NULL, NULL, SW_SHOWNORMAL);
    }

    void WindowsOS::OpenFileExternal(const std::string& path)
    {
        ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }

    void WindowsOS::OpenURL(const std::string& url)
    {
        ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }

#include <glfw/glfw3native.h>

    void WindowsOS::SetTitleBarColour(const Vec4& colour, bool dark)
    {
#if WINVER >= 0x0A00
        auto& app = Lumos::Application::Get();
        HWND hwnd = glfwGetWin32Window((GLFWwindow*)static_cast<GLFWwindow*>(app.GetWindow()->GetHandle()));

        COLORREF col = RGB(colour.x * 255, colour.y * 255, colour.z * 255);

        COLORREF CAPTION_COLOR = col;
        COLORREF BORDER_COLOR  = 0x201e1e;

        DwmSetWindowAttribute(hwnd, 34 /*DWMWINDOWATTRIBUTE::DWMWA_BORDER_COLOR*/, &BORDER_COLOR, sizeof(BORDER_COLOR));
        DwmSetWindowAttribute(hwnd, 35 /*DWMWINDOWATTRIBUTE::DWMWA_CAPTION_COLOR*/, &CAPTION_COLOR, sizeof(CAPTION_COLOR));
        SetWindowPos(hwnd, NULL, NULL, NULL, NULL, NULL, SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE);
#endif
    }

    void WindowsOS::SetWindowDecorations(bool decorated)
    {
#ifdef LUMOS_USE_GLFW_WINDOWS
        auto& app = Lumos::Application::Get();
        GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow()->GetHandle());
        glfwSetWindowAttrib(window, GLFW_DECORATED, decorated ? GLFW_TRUE : GLFW_FALSE);
#endif
    }

#ifdef LUMOS_USE_GLFW_WINDOWS
    static int s_DragStartCursorX = 0;
    static int s_DragStartCursorY = 0;
    static int s_DragStartWindowX = 0;
    static int s_DragStartWindowY = 0;
#endif

    void WindowsOS::BeginWindowDrag()
    {
#ifdef LUMOS_USE_GLFW_WINDOWS
        auto& app          = Lumos::Application::Get();
        GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow()->GetHandle());
        double cx, cy;
        glfwGetCursorPos(window, &cx, &cy);
        glfwGetWindowPos(window, &s_DragStartWindowX, &s_DragStartWindowY);
        s_DragStartCursorX = s_DragStartWindowX + (int)cx;
        s_DragStartCursorY = s_DragStartWindowY + (int)cy;
#endif
    }

    void WindowsOS::UpdateWindowDrag()
    {
#ifdef LUMOS_USE_GLFW_WINDOWS
        auto& app          = Lumos::Application::Get();
        GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow()->GetHandle());
        int wx, wy;
        double cx, cy;
        glfwGetWindowPos(window, &wx, &wy);
        glfwGetCursorPos(window, &cx, &cy);
        const int curScreenX = wx + (int)cx;
        const int curScreenY = wy + (int)cy;
        glfwSetWindowPos(window,
                         s_DragStartWindowX + (curScreenX - s_DragStartCursorX),
                         s_DragStartWindowY + (curScreenY - s_DragStartCursorY));
#endif
    }

    bool WindowsOS::IsWindowMaximised() const
    {
#ifdef LUMOS_USE_GLFW_WINDOWS
        auto& app          = Lumos::Application::Get();
        GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow()->GetHandle());
        return glfwGetWindowAttrib(window, GLFW_MAXIMIZED) == GLFW_TRUE;
#else
        return false;
#endif
    }

    void WindowsOS::RestoreWindow()
    {
#ifdef LUMOS_USE_GLFW_WINDOWS
        auto& app          = Lumos::Application::Get();
        GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow()->GetHandle());
        glfwRestoreWindow(window);
#endif
    }

    void WindowsOS::IconifyWindow()
    {
#ifdef LUMOS_USE_GLFW_WINDOWS
        auto& app          = Lumos::Application::Get();
        GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow()->GetHandle());
        glfwIconifyWindow(window);
#endif
    }

#ifdef LUMOS_USE_GLFW_WINDOWS
    static int s_ResizeEdgeMask    = 0;
    static int s_ResizeStartMouseX = 0;
    static int s_ResizeStartMouseY = 0;
    static int s_ResizeStartWinX   = 0;
    static int s_ResizeStartWinY   = 0;
    static int s_ResizeStartW      = 0;
    static int s_ResizeStartH      = 0;
    static const int kMinW         = 320;
    static const int kMinH         = 240;
#endif

    void WindowsOS::BeginWindowResize(int edgeMask)
    {
#ifdef LUMOS_USE_GLFW_WINDOWS
        auto& app          = Lumos::Application::Get();
        GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow()->GetHandle());
        s_ResizeEdgeMask   = edgeMask;
        double cx, cy;
        glfwGetCursorPos(window, &cx, &cy);
        glfwGetWindowPos(window, &s_ResizeStartWinX, &s_ResizeStartWinY);
        glfwGetWindowSize(window, &s_ResizeStartW, &s_ResizeStartH);
        s_ResizeStartMouseX = s_ResizeStartWinX + (int)cx;
        s_ResizeStartMouseY = s_ResizeStartWinY + (int)cy;
#endif
    }

    void WindowsOS::UpdateWindowResize()
    {
#ifdef LUMOS_USE_GLFW_WINDOWS
        if(!s_ResizeEdgeMask) return;
        auto& app          = Lumos::Application::Get();
        GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow()->GetHandle());
        int wx, wy;
        double cx, cy;
        glfwGetWindowPos(window, &wx, &wy);
        glfwGetCursorPos(window, &cx, &cy);
        const int curX = wx + (int)cx;
        const int curY = wy + (int)cy;
        const int dX   = curX - s_ResizeStartMouseX;
        const int dY   = curY - s_ResizeStartMouseY;

        int newX = s_ResizeStartWinX;
        int newY = s_ResizeStartWinY;
        int newW = s_ResizeStartW;
        int newH = s_ResizeStartH;

        if(s_ResizeEdgeMask & 4)
        {
            newW = s_ResizeStartW - dX;
            if(newW < kMinW) newW = kMinW;
            newX = s_ResizeStartWinX + (s_ResizeStartW - newW);
        }
        if(s_ResizeEdgeMask & 8)
        {
            newW = s_ResizeStartW + dX;
            if(newW < kMinW) newW = kMinW;
        }
        if(s_ResizeEdgeMask & 1)
        {
            newH = s_ResizeStartH - dY;
            if(newH < kMinH) newH = kMinH;
            newY = s_ResizeStartWinY + (s_ResizeStartH - newH);
        }
        if(s_ResizeEdgeMask & 2)
        {
            newH = s_ResizeStartH + dY;
            if(newH < kMinH) newH = kMinH;
        }

        if(newX != wx || newY != wy)
            glfwSetWindowPos(window, newX, newY);
        glfwSetWindowSize(window, newW, newH);
#endif
    }
}
