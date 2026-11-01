// The editor doubles as the command-line exporter (`LumosEditor export ...`): define
// LUMOS_EXPORT_CLI before including this to handle those commands before any window opens.
#ifdef LUMOS_EXPORT_CLI
#include "Export/ExportCLI.h"
#define LUMOS_EXPORT_CLI_QUIET(argc, argv)  \
    if(Lumos::Export::IsCommand(argc, argv)) \
        Lumos::Debug::Log::SetQuiet(true);
#define LUMOS_RUN_EXPORT_CLI(argc, argv)                                 \
    {                                                                    \
        int cliExitCode = 0;                                             \
        if(Lumos::Export::RunCommandLine(argc, argv, cliExitCode))       \
        {                                                                \
            Lumos::Internal::CoreSystem::Shutdown();                     \
            return cliExitCode;                                          \
        }                                                                \
    }
#else
#define LUMOS_EXPORT_CLI_QUIET(argc, argv)
#define LUMOS_RUN_EXPORT_CLI(argc, argv)
#endif

#if defined(LUMOS_PLATFORM_WINDOWS)

#include "Core/CoreSystem.h"
#include "Core/Application.h"
#include "Platform/Windows/WindowsOS.h"

#ifndef NOMINMAX
#define NOMINMAX // For windows.h
#endif

#include <windows.h>

extern Lumos::Application* Lumos::CreateApplication();

#ifdef LUMOS_PRODUCTION
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
    LUMOS_EXPORT_CLI_QUIET(__argc, __argv)
    if(!Lumos::Internal::CoreSystem::Init(__argc, __argv))
        return 0;
    LUMOS_RUN_EXPORT_CLI(__argc, __argv)
#else
#pragma comment(linker, "/subsystem:console")

int main(int argc, char** argv)
{
    LUMOS_EXPORT_CLI_QUIET(argc, argv)
    if(!Lumos::Internal::CoreSystem::Init(argc, argv))
        return 0;
    LUMOS_RUN_EXPORT_CLI(argc, argv)
#endif

    auto windowsOS = new Lumos::WindowsOS();
    Lumos::OS::SetInstance(windowsOS);

    windowsOS->Init();

    Lumos::CreateApplication();

    windowsOS->Run();
    delete windowsOS;

    Lumos::Internal::CoreSystem::Shutdown();
    return Lumos::Application::GetExitCode();
}

#elif defined(LUMOS_PLATFORM_LINUX)

#include "Core/CoreSystem.h"
#include "Core/Application.h"
#include "Platform/Unix/UnixOS.h"

extern Lumos::Application* Lumos::CreateApplication();

int main(int argc, char** argv)
{
    LUMOS_EXPORT_CLI_QUIET(argc, argv)
    if(!Lumos::Internal::CoreSystem::Init(argc, argv))
        return 0;
    LUMOS_RUN_EXPORT_CLI(argc, argv)

    auto unixOS = new Lumos::UnixOS();
    Lumos::OS::SetInstance(unixOS);
    unixOS->Init();

    Lumos::CreateApplication();

    unixOS->Run();
    delete unixOS;

    Lumos::Internal::CoreSystem::Shutdown();
    return Lumos::Application::GetExitCode();
}

#elif defined(LUMOS_PLATFORM_MACOS)

#include "Core/CoreSystem.h"
#include "Core/Application.h"
#include "Platform/MacOS/MacOSOS.h"

int main(int argc, char** argv)
{
    LUMOS_EXPORT_CLI_QUIET(argc, argv)
    if(!Lumos::Internal::CoreSystem::Init(argc, argv))
        return 0;
    LUMOS_RUN_EXPORT_CLI(argc, argv)

    auto macOSOS = new Lumos::MacOSOS();
    Lumos::OS::SetInstance(macOSOS);
    macOSOS->Init();

    Lumos::CreateApplication();

    macOSOS->Run();
    delete macOSOS;

    Lumos::Internal::CoreSystem::Shutdown();
    return Lumos::Application::GetExitCode();
}

#elif defined(LUMOS_PLATFORM_IOS)

#endif
