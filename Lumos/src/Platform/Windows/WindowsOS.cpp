#include "lmpch.h"
#include "WindowsOS.h"
#include "WindowsPower.h"
#include "WindowsMutex.h"
#include "WindowsThread.h"
#include "WindowsWindow.h"
#include "Core/CoreSystem.h"
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

		if (state != PowerState::POWERSTATE_NO_BATTERY)
			LUMOS_LOG_INFO("Battery Info - Percentage : {0} , Time Left {1}s , State : {2}", percentage, secondsLeft, PowerStateToString(state));

        auto app = Lumos::Application::Instance();
        app->Init();
        app->Run();
        delete app;
    }

    void WindowsOS::Init()
    {
        WindowsThread::MakeDefault();
        WindowsMutex::MakeDefault();


#ifdef LUMOS_USE_GLFW_WINDOWS
        GLFWWindow::MakeDefault();
#else
        WindowsWindow::MakeDefault();
#endif
    }
}
