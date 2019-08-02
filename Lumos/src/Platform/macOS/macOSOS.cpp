#include "LM.h"
#include "macOSOS.h"
#include "macOSPower.h"
#include "System/CoreSystem.h"
#include "App/Application.h"

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
        auto app = Lumos::Application::Instance();
        app->Init();
        app->Run();
        delete app;
    }
}
