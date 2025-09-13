#pragma once

#ifdef LUMOS_PLATFORM_WINDOWS
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace Lumos
{
    struct Mutex
    {
#ifdef LUMOS_PLATFORM_WINDOWS
        CRITICAL_SECTION cs;
#else
        pthread_mutex_t handle;
#endif
    };

    i32 MutexInit(Mutex* m);
    int MutexDestroy(Mutex* m);
    int MutexLock(Mutex* m);
    int MutexUnlock(Mutex* m);

    class ScopedMutex
    {
    public:
        ScopedMutex(Mutex* mutex)
            : m_Mutex(mutex)
        {
            MutexLock(m_Mutex);
        }

        ~ScopedMutex()
        {
            MutexUnlock(m_Mutex);
        }

    private:
        Mutex* m_Mutex;
    };
}
