#pragma once
#include "Core/OS/Mutex.h"
#include <windows.h>

namespace Lumos
{
    class WindowsMutex : public Mutex
    {
    public:
        WindowsMutex(bool p_recursive);
        ~WindowsMutex();

        virtual void Lock();
        virtual void Unlock();
        virtual bool TryLock();

    private:

    #ifdef WINDOWS_USE_MUTEX
        HANDLE mutex;
    #else
        CRITICAL_SECTION mutex;
    #endif

    };
}