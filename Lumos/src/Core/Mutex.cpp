#include "LM.h"
#include "Mutex.h"
#include <stddef.h>

namespace Lumos
{
    Mutex *(*Mutex::create_func)(bool) = 0;
    
    Mutex *Mutex::Create(bool p_recursive)
    {
        LUMOS_CORE_ASSERT(!create_func, 0);
        
        return create_func(p_recursive);
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
