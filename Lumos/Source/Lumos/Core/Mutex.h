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

#ifdef LUMOS_PLATFORM_WINDOWS
    using ConditionVar = CONDITION_VARIABLE;
#else
    using ConditionVar = pthread_cond_t;
#endif

    i32 ConditionInit(ConditionVar* cv);
    i32 ConditionDestroy(ConditionVar* cv);
    i32 ConditionWait(ConditionVar* cv, Mutex* m);
    i32 ConditionTimedWait(ConditionVar* cv, Mutex* m, const timespec* abstime);
    i32 ConditionNotifyOne(ConditionVar* cv);
    i32 ConditionNotifyAll(ConditionVar* cv);
}
