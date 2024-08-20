#include "Precompiled.h"
#include "OS.h"

#if defined(LUMOS_PLATFORM_WINDOWS)
#include "Platform/Windows/WindowsOS.h"
#elif defined(LUMOS_PLATFORM_MACOS)
#include "Platform/MacOS/MacOSOS.h"
#elif defined(LUMOS_PLATFORM_IOS)
#include "Platform/iOS/iOSOS.h"
#else
#include "Platform/Unix/UnixOS.h"
#endif

namespace Lumos
{
    OS* OS::s_Instance = nullptr;

    void OS::Create()
    {
        ASSERT(!s_Instance, "OS already exists!");

#if defined(LUMOS_PLATFORM_WINDOWS)
        s_Instance = new WindowsOS();
#elif defined(LUMOS_PLATFORM_MACOS)
        s_Instance = new MacOSOS();
#elif defined(LUMOS_PLATFORM_IOS)
        s_Instance = new iOSOS();
#else
        s_Instance = new UnixOS();
#endif
    }

    void OS::Release()
    {
        delete s_Instance;
        s_Instance = nullptr;
    }

    std::string OS::PowerStateToString(PowerState state)
    {
        switch(state)
        {
        case POWERSTATE_UNKNOWN:
            return std::string("Unknown");
            break;
        case POWERSTATE_NO_BATTERY:
            return std::string("No Battery");
            break;
        case POWERSTATE_CHARGED:
            return std::string("Charged");
            break;
        case POWERSTATE_CHARGING:
            return std::string("Charging");
            break;
        case POWERSTATE_ON_BATTERY:
            return std::string("On Battery");
            break;
        default:
            return std::string("Error");
            break;
        }
    }

    std::string OS::GetCurrentWorkingDirectory()
    {
        return std::string("");
    };
    std::string OS::GetAssetPath()
    {
        return std::string("");
    };

    void OS::ConsoleWrite(const char* msg, u8 level)
    {
        #ifdef LUMOS_PLATFORM_MACOS
            const char* colour_strings[] = {"1;32", "1;30", "1;33", "1;31", "0;41"};
            printf("\033[%sm%s\033[0m", colour_strings[level], msg);
        #else
            printf("%s", msg);
        #endif
    }

}
