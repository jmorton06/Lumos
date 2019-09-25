#include "lmpch.h"
#include "UnixMutex.h"

namespace Lumos
{
    void UnixMutex::Lock()
    {
        pthread_mutex_lock(&mutex);
    }
    
    void UnixMutex::Unlock()
    {
        pthread_mutex_unlock(&mutex);
    }

    bool UnixMutex::TryLock()
    {
        return (pthread_mutex_trylock(&mutex) == 0) ? true : false;
    }

    UnixMutex::UnixMutex(bool p_recursive)
    {
        pthread_mutexattr_init(&attr);
        if (p_recursive)
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&mutex, &attr);
    }

    UnixMutex::~UnixMutex() 
    {
        pthread_mutex_destroy(&mutex);
    }
    
    Mutex* UnixMutex::CreateFuncUnix(bool p_recursive)
    {
        return lmnew UnixMutex(p_recursive);
    }
    
    void UnixMutex::MakeDefault()
    {
        CreateFunc = CreateFuncUnix;
    }
}
