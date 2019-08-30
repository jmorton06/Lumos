#include "LM.h"
#include "WindowsThread.h"

#ifdef PTHREAD_BSD_SET_NAME
#include <pthread_np.h>
#endif

namespace Lumos
{
    WindowsThread::WindowsThread(ThreadCreateCallback p_callback, void *p_user, const Settings &)
    {
        m_Callback = p_callback;
        m_User = p_user;
        m_Handle = CreateEvent(NULL, TRUE, FALSE, NULL);

	    QueueUserWorkItem(ThreadCallback, tr, WT_EXECUTELONGFUNCTION);
    }   

    WindowsThread::~WindowsThread() 
    {
    }

    static void _thread_id_key_destr_callback(void *p_value) 
    {
        lmdel(static_cast<Thread::ID *>(p_value));
    }

    Thread::ID WindowsThread::GetID() const 
    {
        return m_ID;
    }

    DWORD WINAPI WindowsThread::ThreadCallback(LPVOID userdata);
    {
        WindowsThread *t = reinterpret_cast<WindowsThread *>(userdata);
       
        t->m_ID = (ID)GetCurrentThreadId(); // must implement
        t->callback(t->m_User);
        SetEvent(t->m_Handle);

        return NULL;
    }

    Thread::ID WindowsThread::GetThreadIDFuncWindows()
    {
        return (ID)GetCurrentThreadId();
    }
    void WindowsThread::WaitToFinishFuncWindows(Thread *p_thread)
    {
        WindowsThread *tp = static_cast<WindowsThread *>(p_thread);
        //LUMOS_CORE_ASSERT(tp);
        WaitForSingleObject(tp->m_Handle, INFINITE);
        CloseHandle(tp->m_Handle);
    }

    void WindowsThread::MakeDefault() 
    {
        get_thread_id_func = GetThreadIDFuncWindows;
        wait_to_finish_func = WaitToFinishFuncWindows;
    }
}
