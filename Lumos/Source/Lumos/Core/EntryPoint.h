#if defined(LUMOS_PLATFORM_WINDOWS)

#include "Core/CoreSystem.h"
#include "Platform/Windows/WindowsOS.h"

#pragma comment(linker, "/subsystem:windows")

#ifndef NOMINMAX
#define NOMINMAX // For windows.h
#endif

#include <windows.h>

extern Lumos::Application* Lumos::CreateApplication();

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    if(!Lumos::Internal::CoreSystem::Init(0, nullptr))
        return 0;

    auto windowsOS = new Lumos::WindowsOS();
    Lumos::OS::SetInstance(windowsOS);

    windowsOS->Init();

    Lumos::CreateApplication();

    windowsOS->Run();
    delete windowsOS;

    Lumos::Internal::CoreSystem::Shutdown();
}

#elif defined(LUMOS_PLATFORM_LINUX)

#include "Core/CoreSystem.h"
#include "Platform/Unix/UnixOS.h"

extern Lumos::Application* Lumos::CreateApplication();

int main(int argc, char** argv)
{
    if(!Lumos::Internal::CoreSystem::Init(argc, argv))
        return 0;

    auto unixOS = new Lumos::UnixOS();
    Lumos::OS::SetInstance(unixOS);
    unixOS->Init();

    Lumos::CreateApplication();

    unixOS->Run();
    delete unixOS;

    Lumos::Internal::CoreSystem::Shutdown();
}

#elif defined(LUMOS_PLATFORM_MACOS)

#include "Platform/MacOS/MacOSOS.h"

int main(int argc, char** argv)
{
    if(!Lumos::Internal::CoreSystem::Init(argc, argv))
        return 0;

    auto macOSOS = new Lumos::MacOSOS();
    Lumos::OS::SetInstance(macOSOS);
    macOSOS->Init();

    Lumos::CreateApplication();

    macOSOS->Run();
    delete macOSOS;

    Lumos::Internal::CoreSystem::Shutdown();
}

#elif defined(LUMOS_PLATFORM_IOS)

#endif
