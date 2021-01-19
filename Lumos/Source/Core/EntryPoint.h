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
	Lumos::Internal::CoreSystem::Init(false);

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
	Lumos::Internal::CoreSystem::Init(false);
    
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
#include "Core/CommandLineArguments.h"

int main(int argc, char** argv)
{
    std::vector<Lumos::option_t> my_options =
    {
        { "Flag with no argument", "flag", 'm', Lumos::ArgumentRequirement::NONE },
        { "Flag with a required argument", "input-file", 'f', Lumos::ArgumentRequirement::REQUIRE},
        { "Flag with an optional argument", "optimize"  , 'o', Lumos::ArgumentRequirement::OPTIONAL}
    };
    
    // Then call get_arguments
    Lumos::CommandLineArguments::Get().Init(argc, argv);
    auto cli_arguments = Lumos::CommandLineArguments::Get().GetArguments(my_options);
     
    // If the help flag was provided, print out the help message
      if(cli_arguments["help"]) {
          std::cout << "Example Help Print \n";
          Lumos::CommandLineArguments::Get().PrintHelp(my_options);
          return 0;
      }
      // You can test if an argument was provided by simply testing for the long name
      if(cli_arguments["flag"])
      {
          // std::string flag_file = cli_arguments["flag"].as_string(); // Throws a runtime_exception, since there are no arguments on '--flag'
      }
      if(cli_arguments["input-file"])
      {
          std::string input_file = cli_arguments["input-file"].AsString();
      }
      if(cli_arguments["optimize"]) {
          std::string optimize_strategy = cli_arguments["optimize"].AsString(); // Throws a runtime_exception if no flag was provided
      }
    
	Lumos::Internal::CoreSystem::Init(false);

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
