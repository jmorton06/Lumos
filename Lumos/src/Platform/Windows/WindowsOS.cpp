#include "lmpch.h"
#include "WindowsOS.h"
#include "WindowsPower.h"
#include "WindowsWindow.h"
#include "Core/CoreSystem.h"
#include "Core/OS/MemoryManager.h"
#include "App/Application.h"

#ifdef LUMOS_USE_GLFW_WINDOWS
#include "Platform/GLFW/GLFWWindow.h"
#endif

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

	if (state != PowerState::POWERSTATE_NO_BATTERY)
		LUMOS_LOG_INFO("Battery Info - Percentage : {0} , Time Left {1}s , State : {2}", percentage, secondsLeft, PowerStateToString(state));
	else
		LUMOS_LOG_INFO("Power - Outlet");

	auto systemInfo = MemoryManager::Get()->GetSystemInfo();
	systemInfo.Log();
	    
	const String root = ROOT_DIR;
	VFS::Get()->Mount("Meshes", root + "/Sandbox/res/meshes");
	VFS::Get()->Mount("Textures", root + "/Sandbox/res/textures");
	VFS::Get()->Mount("Sounds", root + "/Sandbox/res/sounds");

        auto app = Lumos::Application::Instance();
        app->Init();
        app->Run();
        delete app;
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

		SystemMemoryInfo result =
		{
			(i64)status.ullAvailPhys,
			(i64)status.ullTotalPhys,

			(i64)status.ullAvailVirtual,
			(i64)status.ullTotalVirtual
		};
		return result;
	}
}
