#pragma once

#include "LM.h"
#include "Core/OS/Thread.h"

#include <windows.h>

namespace Lumos
{
    class WindowsThread : public Thread
    {
        friend class Thread;
    public:
        ~WindowsThread();

        virtual ID GetID() const;

        static void MakeDefault();

    protected:
        WindowsThread(ThreadCreateCallback p_callback, void *, const Settings &);

        HANDLE m_Handle;
        ThreadCreateCallback m_Callback;
        void* m_User;
        ID m_ID;

        static DWORD WINAPI ThreadCallback(LPVOID userdata);

        static ID GetThreadIDFuncWindows();
        static void WaitToFinishFuncWindows(Thread *p_thread);
    };
}
