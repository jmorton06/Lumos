#include "lmpch.h"
#include "macOSOS.h"
#include "macOSPower.h"
#include "Platform/GLFW/GLFWWindow.h"
#include "Core/CoreSystem.h"
#include "App/Application.h"

#include <mach-o/dyld.h>

extern Lumos::Application* Lumos::CreateApplication();

namespace Lumos
{
    void macOSOS::Run()
    {
        auto power = macOSPower();
        auto percentage = power.GetPowerPercentageLeft();
        auto secondsLeft = power.GetPowerSecondsLeft();
        auto state = power.GetPowerState();

        LUMOS_CORE_INFO("Battery Info - Percentage : {0} , Time Left {1}s , State : {2}", percentage, secondsLeft, PowerStateToString(state));
        
        const String root = ROOT_DIR;
		VFS::Get()->Mount("Meshes", root + "/Assets/meshes");
		VFS::Get()->Mount("Textures", root + "/Assets/textures");
		VFS::Get()->Mount("Sounds", root + "/Assets/sounds");
        
        auto app = Lumos::Application::Instance();
        app->Init();
        app->Run();
        delete app;
    }

    void macOSOS::Init()
    {
        GLFWWindow::MakeDefault();
    }

    String macOSOS::GetExecutablePath()
    {
        std::string result;

        uint32_t size = 0;
        _NSGetExecutablePath(nullptr, &size);

        std::vector<char> buffer;
        buffer.resize(size + 1);

        _NSGetExecutablePath(buffer.data(), &size);
        buffer[size] = '\0';

        if (!strrchr(buffer.data(), '/'))
        {
            return "";
        }
        return buffer.data();
    }

    PowerState macOSOS::GetPowerState()
    {
        auto power = macOSPower();
        auto state = power.GetPowerState();

        return state;
    }

    int macOSOS::BatteryPercentage()
    {
        auto power = macOSPower();
        auto percentage = power.GetPowerPercentageLeft();

        return percentage;
    }

    int macOSOS::BatterySecondsLeft()
    {
        auto power = macOSPower();
        auto seconds = power.GetPowerSecondsLeft();

        return seconds;
    }
}
