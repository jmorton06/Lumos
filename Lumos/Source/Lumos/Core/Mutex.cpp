#include "Precompiled.h"
#include "Mutex.h"

#if LUMOS_PROFILE
#include <Tracy/public/tracy/TracyC.h>
#include <cstring>

#define LUMOS_MUTEX_ANNOUNCE(m)                                                                                         \
    do                                                                                                                 \
    {                                                                                                                  \
        static const ___tracy_source_location_data srcloc = { "Mutex", __FUNCTION__, __FILE__, (uint32_t)__LINE__, 0 }; \
        (m)->tracyLock                                     = ___tracy_announce_lockable_ctx(&srcloc);                  \
    } while(0)
#endif

namespace Lumos
{
    void MutexSetName(Mutex* m, const char* name)
    {
#if LUMOS_PROFILE
        if(m->tracyLock && name)
            ___tracy_custom_name_lockable_ctx(m->tracyLock, name, strlen(name));
#else
        (void)m;
        (void)name;
#endif
    }

#ifdef LUMOS_PLATFORM_WINDOWS

    i32 MutexInit(Mutex* m)
    {
        InitializeCriticalSection(&m->cs);
#if LUMOS_PROFILE
        LUMOS_MUTEX_ANNOUNCE(m);
#endif
        return 0;
    }

    i32 MutexDestroy(Mutex* m)
    {
#if LUMOS_PROFILE
        if(m->tracyLock)
        {
            ___tracy_terminate_lockable_ctx(m->tracyLock);
            m->tracyLock = nullptr;
        }
#endif
        DeleteCriticalSection(&m->cs);
        return 0;
    }

    i32 MutexLock(Mutex* m)
    {
#if LUMOS_PROFILE
        const int32_t runAfter = m->tracyLock ? ___tracy_before_lock_lockable_ctx(m->tracyLock) : 0;
#endif
        EnterCriticalSection(&m->cs);
#if LUMOS_PROFILE
        if(runAfter)
            ___tracy_after_lock_lockable_ctx(m->tracyLock);
#endif
        return 0;
    }

    i32 MutexUnlock(Mutex* m)
    {
        LeaveCriticalSection(&m->cs);
#if LUMOS_PROFILE
        if(m->tracyLock)
            ___tracy_after_unlock_lockable_ctx(m->tracyLock);
#endif
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
#if LUMOS_PROFILE
        if(m->tracyLock)
            ___tracy_after_unlock_lockable_ctx(m->tracyLock);
#endif
        BOOL ok = SleepConditionVariableCS(cv, &m->cs, INFINITE);
#if LUMOS_PROFILE
        if(m->tracyLock)
            ___tracy_after_lock_lockable_ctx(m->tracyLock);
#endif
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
        i32 r = pthread_mutex_init(&m->handle, NULL);
#if LUMOS_PROFILE
        LUMOS_MUTEX_ANNOUNCE(m);
#endif
        return r;
    }

    i32 MutexDestroy(Mutex* m)
    {
#if LUMOS_PROFILE
        if(m->tracyLock)
        {
            ___tracy_terminate_lockable_ctx(m->tracyLock);
            m->tracyLock = nullptr;
        }
#endif
        return pthread_mutex_destroy(&m->handle);
    }

    i32 MutexLock(Mutex* m)
    {
#if LUMOS_PROFILE
        const int32_t runAfter = m->tracyLock ? ___tracy_before_lock_lockable_ctx(m->tracyLock) : 0;
#endif
        i32 r = pthread_mutex_lock(&m->handle);
#if LUMOS_PROFILE
        if(runAfter)
            ___tracy_after_lock_lockable_ctx(m->tracyLock);
#endif
        return r;
    }

    i32 MutexUnlock(Mutex* m)
    {
        i32 r = pthread_mutex_unlock(&m->handle);
#if LUMOS_PROFILE
        if(m->tracyLock)
            ___tracy_after_unlock_lockable_ctx(m->tracyLock);
#endif
        return r;
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
#if LUMOS_PROFILE
        if(m->tracyLock)
            ___tracy_after_unlock_lockable_ctx(m->tracyLock);
#endif
        i32 r = pthread_cond_wait(cv, &m->handle);
#if LUMOS_PROFILE
        if(m->tracyLock)
            ___tracy_after_lock_lockable_ctx(m->tracyLock);
#endif
        return r;
    }
    i32 ConditionTimedWait(ConditionVar* cv, Mutex* m, const timespec* abstime)
    {
#if LUMOS_PROFILE
        if(m->tracyLock)
            ___tracy_after_unlock_lockable_ctx(m->tracyLock);
#endif
        i32 r = pthread_cond_timedwait(cv, &m->handle, abstime);
#if LUMOS_PROFILE
        if(m->tracyLock)
            ___tracy_after_lock_lockable_ctx(m->tracyLock);
#endif
        return r;
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
