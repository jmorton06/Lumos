#include "LM.h"
#include "macOSOS.h"
#include "System/CoreSystem.h"
#include "App/Application.h"

extern Lumos::Application* Lumos::CreateApplication();

namespace Lumos
{
    void macOSOS::Run()
    {
        auto app = Lumos::Application::Instance();
        app->Init();
        app->Run();
        delete app;
    }
}
