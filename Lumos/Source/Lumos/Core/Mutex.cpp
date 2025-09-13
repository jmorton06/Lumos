#include "Precompiled.h"
#include "Mutex.h"

namespace Lumos
{
#ifdef LUMOS_PLATFORM_WINDOWS

    i32 MutexInit(Mutex* m)
    {
        InitializeCriticalSection(&m->cs);
        return 0;
    }

    i32 MutexDestroy(Mutex* m)
    {
        DeleteCriticalSection(&m->cs);
        return 0;
    }

    i32 MutexLock(Mutex* m)
    {
        EnterCriticalSection(&m->cs);
        return 0;
    }

    i32 MutexUnlock(Mutex* m)
    {
        LeaveCriticalSection(&m->cs);
        return 0;
    }

#else // POSIX

    i32 MutexInit(Mutex* m)
    {
        return pthread_mutex_init(&m->handle, NULL);
    }

    i32 MutexDestroy(Mutex* m)
    {
        return pthread_mutex_destroy(&m->handle);
    }

    i32 MutexLock(Mutex* m)
    {
        return pthread_mutex_lock(&m->handle);
    }

    i32 MutexUnlock(Mutex* m)
    {
        return pthread_mutex_unlock(&m->handle);
    }

#endif
}
