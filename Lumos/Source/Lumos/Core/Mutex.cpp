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

    i32 ConditionInit(ConditionVar* cv)
    {
        InitializeConditionVariable(cv);
        return 0;
    }

    i32 ConditionDestroy(ConditionVar* cv)
    {
        (void)cv;
        return 0;
    }

    i32 ConditionWait(ConditionVar* cv, Mutex* m)
    {
        BOOL ok = SleepConditionVariableCS(cv, &m->cs, INFINITE);
        return ok ? 0 : GetLastError();
    }

    i32 ConditionTimedWait(ConditionVar* cv, Mutex* m, const timespec* abstime)
    {
        return 0;
    }

    i32 ConditionNotifyOne(ConditionVar* cv)
    {
        WakeConditionVariable(cv);
        return 0;
    }

    i32 ConditionNotifyAll(ConditionVar* cv)
    {
        WakeAllConditionVariable(cv);
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

    i32 ConditionInit(ConditionVar* cv)
    {
        return pthread_cond_init(cv, nullptr);
    }
    i32 ConditionDestroy(ConditionVar* cv)
    {
        return pthread_cond_destroy(cv);
    }
    i32 ConditionWait(ConditionVar* cv, Mutex* m)
    {
        return pthread_cond_wait(cv, &m->handle);
    }
    i32 ConditionTimedWait(ConditionVar* cv, Mutex* m, const timespec* abstime)
    {
        return pthread_cond_timedwait(cv, &m->handle, abstime);
    }
    i32 ConditionNotifyOne(ConditionVar* cv)
    {
        return pthread_cond_signal(cv);
    }
    i32 ConditionNotifyAll(ConditionVar* cv)
    {
        return pthread_cond_broadcast(cv);
    }

#endif
}
