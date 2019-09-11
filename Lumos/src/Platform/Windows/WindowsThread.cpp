#include "lmpch.h"
#include "WindowsThread.h"

#ifdef PTHREAD_BSD_SET_NAME
#include <pthread_np.h>
#endif

namespace Lumos
{
    WindowsThread::WindowsThread()
    {
		m_Handle = NULL;
    }   

    WindowsThread::~WindowsThread() 
    {
    }

    Thread::ID WindowsThread::GetID() const 
    {
        return m_ID;
    }

    DWORD WindowsThread::ThreadCallback(LPVOID userdata)
    {
        WindowsThread* t = reinterpret_cast<WindowsThread*>(userdata);
       
		t->m_ID = (ID)GetCurrentThreadId();
        t->m_Callback(t->m_User);
        SetEvent(t->m_Handle);

        return 0;
    }

    Thread::ID WindowsThread::GetThreadIDFuncWindows()
    {
        return (ID)GetCurrentThreadId();
    }

    void WindowsThread::WaitToFinishFuncWindows(Thread* p_thread)
    {
        WindowsThread* tp = static_cast<WindowsThread*>(p_thread);
        LUMOS_ASSERT(tp, "Thread nullptr");
        WaitForSingleObject(tp->m_Handle, INFINITE);
        CloseHandle(tp->m_Handle);
    }

    void WindowsThread::MakeDefault() 
    {
        GetThreadIDFunc = GetThreadIDFuncWindows;
        WaitToFinishFunc = WaitToFinishFuncWindows;
        CreateFunc = CreateFuncWindows;
    }

    Thread* WindowsThread::CreateFuncWindows(ThreadCreateCallback p_callback, void* p_user, const Settings& p_settings)
    {
		WindowsThread* tr = lmnew WindowsThread();
		tr->m_Callback = p_callback;
		tr->m_User = p_user;
		tr->m_Handle = CreateEvent(NULL, TRUE, FALSE, NULL);

		QueueUserWorkItem(ThreadCallback, tr, WT_EXECUTELONGFUNCTION);
        return tr;
    }
}
