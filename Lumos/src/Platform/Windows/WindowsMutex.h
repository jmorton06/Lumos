#pragma once
#include "Core/Mutex.h"
#include <windows.h>

namespace Lumos
{
    class WindowsMutex : public Mutex
    {
    public:
        virtual void Lock();
        virtual void Unlock();
        virtual bool TryLock();

        WindowsMutex();
        ~WindowsMutex();
    private:

    #ifdef WINDOWS_USE_MUTEX
        HANDLE mutex;
    #else
        CRITICAL_SECTION mutex;
    #endif

    };
}