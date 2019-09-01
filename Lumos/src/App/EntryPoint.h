#if defined(LUMOS_PLATFORM_WINDOWS)

#include "Core/CoreSystem.h"
#include "Platform/Windows/WindowsOS.h"

extern Lumos::Application* Lumos::CreateApplication();

int main(int argc, char** argv)
{
	Lumos::Internal::CoreSystem::Init();

    auto windowsOS = new Lumos::WindowsOS();
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
	Lumos::Internal::CoreSystem::Init();
    
    auto unixOS = new Lumos::UnixOS();
    unixOS->Init();
    
    Lumos::CreateApplication();

    unixOS->Run();
    delete unixOS;

	Lumos::Internal::CoreSystem::Shutdown();
}


#elif defined(LUMOS_PLATFORM_MACOS)

#include "Platform/macOS/macOSOS.h"

int main(int argc, char** argv)
{
    Lumos::Internal::CoreSystem::Init();

    auto macOSOS = new Lumos::macOSOS();
    macOSOS->Init();
    
    Lumos::CreateApplication();

    macOSOS->Run();
    delete macOSOS;

    Lumos::Internal::CoreSystem::Shutdown();
}

#elif defined(LUMOS_PLATFORM_IOS)


#endif
