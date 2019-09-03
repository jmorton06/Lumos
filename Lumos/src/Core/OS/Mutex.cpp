#include "LM.h"
#include "Mutex.h"
#include <stddef.h>

#ifdef LUMOS_PLATFORM_UNIX
#include "Platform/Unix/UnixMutex.h"
#endif

#ifdef LUMOS_PLATFORM_WINDOWS
#include "Platform/Windows/WindowsMutex.h"
#endif

namespace Lumos
{
    Mutex*(*Mutex::CreateFunc)(bool) = 0;
    
    Mutex *Mutex::Create(bool p_recursive)
    {
        LUMOS_ASSERT(CreateFunc, "No Mutex Create Function");
        
        return CreateFunc(p_recursive);
    }
    
    Mutex::~Mutex()
    {
    }
    
    Mutex *_global_mutex = NULL;
    
    void _global_lock()
    {
        if (_global_mutex)
            _global_mutex->Lock();
    }
    
    void _global_unlock()
    {
        if (_global_mutex)
            _global_mutex->Unlock();
    }
}
