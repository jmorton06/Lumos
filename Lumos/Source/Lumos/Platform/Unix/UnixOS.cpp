#include "Precompiled.h"
#include "UnixOS.h"
#include "Platform/GLFW/GLFWWindow.h"
#include "Core/CoreSystem.h"
#include "Core/OS/MemoryManager.h"
#include "Core/Application.h"

#include <time.h>
#ifdef LUMOS_PLATFORM_MACOS
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <GLFW/glfw3.h>
#endif
#ifdef LUMOS_PLATFORM_LINUX
#include <linux/limits.h>
#include <unistd.h>
#include <GLFW/glfw3.h>
#endif

extern Lumos::Application* Lumos::CreateApplication();

namespace Lumos
{
    void UnixOS::Run()
    {
        auto& app = Lumos::Application::Get();

        LINFO("--------------------");
        LINFO(" System Information ");
        LINFO("--------------------");

        auto systemInfo = MemoryManager::Get().GetSystemInfo();
        systemInfo.Log();

        app.Init();
        app.Run();
        app.Release();
    }

    void UnixOS::Init()
    {
        GLFWWindow::MakeDefault();
    }

    SystemMemoryInfo MemoryManager::GetSystemInfo()
    {

#ifdef LUMOS_PLATFORM_MACOS
        int64_t total_physical;
        size_t len = sizeof(total_physical);
        sysctlbyname("hw.memsize", &total_physical, &len, NULL, 0);

        mach_port_t mach_port = mach_host_self();
        vm_size_t page_size;
        mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
        vm_statistics64_data_t vm_stats;

        if(host_page_size(mach_port, &page_size) != KERN_SUCCESS)
        {
            perror("Failed to get page size");
            exit(EXIT_FAILURE);
        }

        if(host_statistics64(mach_port, HOST_VM_INFO, (host_info64_t)&vm_stats, &count) != KERN_SUCCESS)
        {
            perror("Failed to get VM statistics");
            exit(EXIT_FAILURE);
        }

        int64_t available_physical = (int64_t)vm_stats.free_count * (int64_t)page_size;
        int64_t total_virtual      = (int64_t)(vm_stats.wire_count + vm_stats.active_count + vm_stats.inactive_count + vm_stats.free_count) * (int64_t)page_size;
        int64_t available_virtual  = (int64_t)vm_stats.free_count * (int64_t)page_size;

        SystemMemoryInfo result = {
            available_physical,
            total_physical,
            available_virtual,
            total_virtual
        };

        return result;
#else
        SystemMemoryInfo result = {};
        return result;
#endif
    }

    void UnixOS::Delay(uint32_t usec)
    {
        struct timespec requested = { static_cast<time_t>(usec / 1000000), (static_cast<long>(usec) % 1000000) * 1000 };
        struct timespec remaining;
        while(nanosleep(&requested, &remaining) == -1)
        {
            requested.tv_sec  = remaining.tv_sec;
            requested.tv_nsec = remaining.tv_nsec;
        }
    }

    void UnixOS::OpenFileLocation(const std::string& path)
    {
#ifndef LUMOS_PLATFORM_MOBILE
        std::string command = "open -R " + path;
        (void)std::system(command.c_str());
#endif
    }

    void UnixOS::OpenFileExternal(const std::string& path)
    {
#ifndef LUMOS_PLATFORM_MOBILE
        std::string command = "open " + path;
        (void)std::system(command.c_str());
#endif
    }

    void UnixOS::OpenURL(const std::string& url)
    {
#ifndef LUMOS_PLATFORM_MOBILE
        std::string command = "open " + url;
        (void)system(command.c_str());
#endif
    }

    void UnixOS::SetWindowDecorations(bool decorated)
    {
#ifndef LUMOS_PLATFORM_MOBILE
        auto& app          = Lumos::Application::Get();
        GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow()->GetHandle());
        glfwSetWindowAttrib(window, GLFW_DECORATED, decorated ? GLFW_TRUE : GLFW_FALSE);
#endif
    }

#ifndef LUMOS_PLATFORM_MOBILE
    static int s_DragStartCursorX = 0;
    static int s_DragStartCursorY = 0;
    static int s_DragStartWindowX = 0;
    static int s_DragStartWindowY = 0;
#endif

    void UnixOS::BeginWindowDrag()
    {
#ifndef LUMOS_PLATFORM_MOBILE
        auto& app          = Lumos::Application::Get();
        GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow()->GetHandle());
        double cx, cy;
        glfwGetCursorPos(window, &cx, &cy);
        glfwGetWindowPos(window, &s_DragStartWindowX, &s_DragStartWindowY);
        s_DragStartCursorX = s_DragStartWindowX + (int)cx;
        s_DragStartCursorY = s_DragStartWindowY + (int)cy;
#endif
    }

    void UnixOS::UpdateWindowDrag()
    {
#ifndef LUMOS_PLATFORM_MOBILE
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

    bool UnixOS::IsWindowMaximised() const
    {
#ifndef LUMOS_PLATFORM_MOBILE
        auto& app          = Lumos::Application::Get();
        GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow()->GetHandle());
        return glfwGetWindowAttrib(window, GLFW_MAXIMIZED) == GLFW_TRUE;
#else
        return false;
#endif
    }

    void UnixOS::RestoreWindow()
    {
#ifndef LUMOS_PLATFORM_MOBILE
        auto& app          = Lumos::Application::Get();
        GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow()->GetHandle());
        glfwRestoreWindow(window);
#endif
    }

    void UnixOS::IconifyWindow()
    {
#ifndef LUMOS_PLATFORM_MOBILE
        auto& app          = Lumos::Application::Get();
        GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow()->GetHandle());
        glfwIconifyWindow(window);
#endif
    }

#ifndef LUMOS_PLATFORM_MOBILE
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

    void UnixOS::BeginWindowResize(int edgeMask)
    {
#ifndef LUMOS_PLATFORM_MOBILE
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

    void UnixOS::UpdateWindowResize()
    {
#ifndef LUMOS_PLATFORM_MOBILE
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

    std::string UnixOS::GetExecutablePath()
    {
#ifdef LUMOS_PLATFORM_LINUX
        char result[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        if(count != -1)
        {
            return std::string(result, count);
        }
        else
        {
            return std::string();
        }
#else
        return std::string();
#endif
    }
}
