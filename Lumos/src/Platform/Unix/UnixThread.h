#pragma once

#include "LM.h"
#include "Core/OS/Thread.h"

#include <pthread.h>
#include <sys/types.h>

namespace Lumos
{
    class UnixThread : public Thread
    {
        friend class Thread;
    public:
        ~UnixThread();

        virtual ID GetID() const;

        static void MakeDefault();

    protected:
        UnixThread(ThreadCreateCallback p_callback, void *, const Settings &);

        static pthread_key_t s_ThreadIDKey;
        static ID s_NextThreadID;

        pthread_t m_PThread;
        pthread_attr_t m_PThreadAttr;
        ThreadCreateCallback m_Callback;
        void* m_User;
        ID m_ID;

        static void *ThreadCallback(void *userdata);

        static ID GetThreadIDFuncUnix();
        static void WaitToFinishFuncUnix(Thread *p_thread);

        static Error SetNameFuncUnix(const String &p_name);
    };
}
