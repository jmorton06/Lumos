#include "LM.h"
#include "Thread.h"

#ifdef LUMOS_PLATFORM_UNIX
#include "Platform/Unix/UnixThread.h"
#endif

#ifdef LUMOS_PLATFORM_WINDOWS
#include "Platform/Windows/WindowsThread.h"
#endif

namespace Lumos
{    
    Thread::ID (*Thread::get_thread_id_func)() = NULL;
    void (*Thread::wait_to_finish_func)(Thread *) = NULL;
    Error (*Thread::set_name_func)(const String &) = NULL;
    
    Thread *(*Thread::CreateFunc)(ThreadCreateCallback, void*, const Settings&) = NULL;

    Thread::ID Thread::s_MainThreadID = 0;

    Thread* Thread::Create(ThreadCreateCallback p_callback, void *p_user, const Settings &p_settings)
    {
        LUMOS_CORE_ASSERT(CreateFunc, "No Thread Create Function");
        
        return CreateFunc(p_callback, p_user, p_settings);
    }
    
    Thread::ID Thread::GetCallerID() 
    {
        if (get_thread_id_func)
            return get_thread_id_func();

            return 0;
    }
        
    void Thread::WaitToFinish(Thread *p_thread) 
    {
        if (wait_to_finish_func)
            wait_to_finish_func(p_thread);
    }

    Error Thread::SetName(const String &p_name) 
    {
        if (set_name_func)
            return set_name_func(p_name);

        return ERR_UNAVAILABLE;
    };
}
