#pragma once

#ifdef LUMOS_PLATFORM_WINDOWS
#include <windows.h>
#else
#include <pthread.h>
#endif

#include <time.h>
#include "Core/Profiler.h" // LUMOS_PROFILE

#if LUMOS_PROFILE
// Tracy C lock-context handle (defined in Tracy/public/tracy/TracyC.h, global namespace).
struct __tracy_lockable_context_data;
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
#if LUMOS_PROFILE
        __tracy_lockable_context_data* tracyLock = nullptr;
#endif
    };

    i32 MutexInit(Mutex* m);
    int MutexDestroy(Mutex* m);
    int MutexLock(Mutex* m);
    int MutexUnlock(Mutex* m);

    // Optional readable name for the lock in Tracy's lock list. No-op without profiling.
    void MutexSetName(Mutex* m, const char* name);

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
