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
}