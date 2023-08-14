#include "Precompiled.h"
#include "UnixOS.h"
#include "Platform/GLFW/GLFWWindow.h"
#include "Core/CoreSystem.h"
#include "Core/OS/MemoryManager.h"
#include "Core/Application.h"

#include <time.h>

extern Lumos::Application* Lumos::CreateApplication();

namespace Lumos
{
    void UnixOS::Run()
    {
        auto& app = Lumos::Application::Get();

        LUMOS_LOG_INFO("--------------------");
        LUMOS_LOG_INFO(" System Information ");
        LUMOS_LOG_INFO("--------------------");

        auto systemInfo = MemoryManager::Get()->GetSystemInfo();
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
        SystemMemoryInfo result = {};
        return result;
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

    void UnixOS::OpenFileLocation(const std::filesystem::path& path)
    {
#ifndef LUMOS_PLATFORM_MOBILE
        std::string command = "open -R " + path.string();
        std::system(command.c_str());
#endif
    }

    void UnixOS::OpenFileExternal(const std::filesystem::path& path)
    {
#ifndef LUMOS_PLATFORM_MOBILE
        std::string command = "open " + path.string();
        std::system(command.c_str());
#endif
    }

    void UnixOS::OpenURL(const std::string& url)
    {
#ifndef LUMOS_PLATFORM_MOBILE
        std::string command = "open " + url;
        system(command.c_str());
#endif
    }
}
