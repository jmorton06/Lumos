#include "LM.h"
#include "WindowsMutex.h"

namespace Lumos
{
    void WindowsMutex::Lock() 
    {
    #ifdef WINDOWS_USE_MUTEX
        WaitForSingleObject(mutex, INFINITE);
    #else
        EnterCriticalSection(&mutex);
    #endif
    }

    void WindowsMutex::Unlock()
    {
    #ifdef WINDOWS_USE_MUTEX
        ReleaseMutex(mutex);
    #else
        LeaveCriticalSection(&mutex);
    #endif
    }

    bool WindowsMutex::TryLock() 
    {
    #ifdef WINDOWS_USE_MUTEX
        return (WaitForSingleObject(mutex, 0) == WAIT_TIMEOUT) ? false : true;
    #else

        if (TryEnterCriticalSection(&mutex))
            return true;
        else
            return false;
    #endif
    }

    WindowsMutex::WindowsMutex(bool p_recursive)
    {
    #ifdef WINDOWS_USE_MUTEX
        mutex = CreateMutex(NULL, FALSE, NULL);
    #else
    #ifdef UWP_ENABLED
        InitializeCriticalSectionEx(&mutex, 0, 0);
    #else
        InitializeCriticalSection(&mutex);
    #endif
    #endif
    }

    WindowsMutex::~WindowsMutex()
    {
    #ifdef WINDOWS_USE_MUTEX
        CloseHandle(mutex);
    #else
        DeleteCriticalSection(&mutex);
    #endif
    }

    Mutex* WindowsMutex::CreateFuncWindows(bool p_recursive)
    {
        return lmnew WindowsMutex(p_recursive);
    }
    
    void WindowsMutex::MakeDefault()
    {
        CreateFunc = CreateFuncWindows;
    }
}