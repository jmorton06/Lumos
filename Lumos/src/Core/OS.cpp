#include "LM.h"
#include "OS.h"

#if defined(LUMOS_PLATFORM_WINDOWS)
#include "Platform/Windows/WindowsOS.h"
#elif defined(LUMOS_PLATFORM_MACOS)
#include "Platform/macOS/macOSOS.h"
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
        LUMOS_ASSERT(!s_Instance, "OS already exists!");

        #if defined(LUMOS_PLATFORM_WINDOWS)
        s_Instance = lmnew WindowsOS();
        #elif defined(LUMOS_PLATFORM_MACOS)
        s_Instance = lmnew macOSOS();
        #elif defined(LUMOS_PLATFORM_IOS)
        s_Instance = lmnew iOSOS();
        #else
        s_Instance = lmnew UnixOS();
        #endif
    }

    void OS::Release()
    {
        lmdel s_Instance;
    }
    
    String OS::PowerStateToString(PowerState state)
    {
        switch (state)
        {
            case POWERSTATE_UNKNOWN:
                return String("Unknown");
                break;
            case POWERSTATE_NO_BATTERY:
                return String("No Battery");
                break;
            case POWERSTATE_CHARGED:
                return String("Charged");
                break;
            case POWERSTATE_CHARGING:
                return String("Charging");
                break;
            case POWERSTATE_ON_BATTERY:
                return String("On Battery");
                break;
            default:
                return String("Error");
                break;
        }
    }
}
