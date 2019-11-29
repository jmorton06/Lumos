#pragma once

#include "Core/OS/Mutex.h"

#include <pthread.h>

namespace Lumos
{
    class UnixMutex : public Mutex 
    {
    public:
        virtual void Lock() override;
        virtual void Unlock() override;
        virtual bool TryLock() override;

		explicit UnixMutex(bool p_recursive);
        ~UnixMutex();
        
        static void MakeDefault();
    protected:
        static Mutex* CreateFuncUnix(bool p_recursive);

    private:
        pthread_mutexattr_t attr;
        pthread_mutex_t mutex;
    };
}
