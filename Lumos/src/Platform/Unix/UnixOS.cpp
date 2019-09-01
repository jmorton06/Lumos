#include "LM.h"
#include "UnixOS.h"
#include "Platform/Unix/UnixThread.h"
#include "Platform/Unix/UnixMutex.h"
#include "Core/CoreSystem.h"
#include "App/Application.h"

extern Lumos::Application* Lumos::CreateApplication();

namespace Lumos
{
    void UnixOS::Run()
    {
        auto app = Lumos::Application::Instance();
        app->Init();
        app->Run();
        delete app;
    }
    
    void UnixOS::Init()
    {
        UnixThread::MakeDefault();
        UnixMutex::MakeDefault();
    }
}
