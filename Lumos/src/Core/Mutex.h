#pragma once

namespace Lumos
{
    class Mutex
    {
    public:
        virtual void Lock() = 0; ///< Lock the mutex, block if locked by someone else
        virtual void Unlock() = 0; ///< Unlock the mutex, let other threads continue
        virtual bool TryLock() = 0; ///< Attempt to lock the mutex, True on success, ERROR means it can't lock.
        
        static Mutex *Create(bool p_recursive = true); ///< Create a mutex
        
        virtual ~Mutex();
    };
    
    class MutexLock
    {
        Mutex *mutex;
        
    public:
        MutexLock(Mutex *p_mutex)
        {
            mutex = p_mutex;
            if (mutex) mutex->Lock();
        }
        ~MutexLock()
        {
            if (mutex) mutex->Unlock();
        }
    };
}
