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
    Thread::ID (*Thread::GetThreadIDFunc)() = NULL;
    void (*Thread::WaitToFinishFunc)(Thread *) = NULL;
    Error (*Thread::SetNameFunc)(const String &) = NULL;
    
    Thread *(*Thread::CreateFunc)(ThreadCreateCallback, void*, const Settings&) = NULL;

    Thread::ID Thread::s_MainThreadID = 0;

    Thread* Thread::Create(ThreadCreateCallback p_callback, void *p_user, const Settings &p_settings)
    {
        LUMOS_ASSERT(CreateFunc, "No Thread Create Function");
        
        return CreateFunc(p_callback, p_user, p_settings);
    }
    
    Thread::ID Thread::GetCallerID() 
    {
        if (GetThreadIDFunc)
            return GetThreadIDFunc();

            return 0;
    }
        
    void Thread::WaitToFinish(Thread *p_thread) 
    {
        if (WaitToFinishFunc)
            WaitToFinishFunc(p_thread);
    }

    Error Thread::SetName(const String &p_name) 
    {
        if (SetNameFunc)
            return SetNameFunc(p_name);

        return ERR_UNAVAILABLE;
    };
}
