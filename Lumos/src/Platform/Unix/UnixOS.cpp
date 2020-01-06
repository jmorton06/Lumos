#include "lmpch.h"
#include "UnixOS.h"
#include "Platform/GLFW/GLFWWindow.h"
#include "Core/CoreSystem.h"
#include "Core/OS/MemoryManager.h"
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
        GLFWWindow::MakeDefault();
    }

    SystemMemoryInfo MemoryManager::GetSystemInfo()
    {
        return SystemMemoryInfo();
    }
}
