#include "Precompiled.h"
#include "WindowsOS.h"
#include "WindowsPower.h"
#include "WindowsWindow.h"
#include "Core/CoreSystem.h"
#include "Core/OS/MemoryManager.h"
#include "Core/Application.h"

#ifdef LUMOS_USE_GLFW_WINDOWS
#include "Platform/GLFW/GLFWWindow.h"
#endif

#include <Windows.h>

extern Lumos::Application* Lumos::CreateApplication();

namespace Lumos
{
    void WindowsOS::Run()
    {
        auto power = WindowsPower();
        auto percentage = power.GetPowerPercentageLeft();
        auto secondsLeft = power.GetPowerSecondsLeft();
        auto state = power.GetPowerState();

        LUMOS_LOG_INFO("--------------------");
        LUMOS_LOG_INFO(" System Information ");
        LUMOS_LOG_INFO("--------------------");

        if(state != PowerState::POWERSTATE_NO_BATTERY)
            LUMOS_LOG_INFO("Battery Info - Percentage : {0} , Time Left {1}s , State : {2}", percentage, secondsLeft, PowerStateToString(state));
        else
            LUMOS_LOG_INFO("Power - Outlet");

        auto systemInfo = MemoryManager::Get()->GetSystemInfo();
        systemInfo.Log();

        const std::string root = ROOT_DIR;
        VFS::Get()->Mount("Meshes", root + "/Assets/meshes");
        VFS::Get()->Mount("Textures", root + "/Assets/textures");
        VFS::Get()->Mount("Sounds", root + "/Assets/sounds");

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
}
