#include "LM.h"
#include "UnixThread.h"

#if defined(__FreeBSD__) || defined(__OpenBSD__)
#include <stdlib.h>
#define PTHREAD_BSD_SET_NAME
#endif
#ifdef __APPLE__
#define PTHREAD_RENAME_SELF
#endif

#ifdef PTHREAD_BSD_SET_NAME
#include <pthread_np.h>
#endif

namespace Lumos
{
    UnixThread::UnixThread(ThreadCreateCallback p_callback, void *p_user, const Settings &)
    {
	    m_PThread = 0;

        m_Callback = p_callback;
        m_User = p_user;
        pthread_attr_init(&m_PThreadAttr);
        pthread_attr_setdetachstate(&m_PThreadAttr, PTHREAD_CREATE_JOINABLE);
        pthread_attr_setstacksize(&m_PThreadAttr, 256 * 1024);

        pthread_create(&m_PThread, &m_PThreadAttr, ThreadCallback, this);

    }   

    UnixThread::~UnixThread() 
    {
    }

    static void _thread_id_key_destr_callback(void *p_value) 
    {
        lmdel(static_cast<Thread::ID *>(p_value));
    }

    static pthread_key_t _create_thread_id_key() 
    {
        pthread_key_t key;
        pthread_key_create(&key, &_thread_id_key_destr_callback);
        return key;
    }

    pthread_key_t UnixThread::s_ThreadIDKey = _create_thread_id_key();
    Thread::ID UnixThread::s_NextThreadID = 0;

    Thread::ID UnixThread::GetID() const 
    {
        return m_ID;
    }

    void *UnixThread::ThreadCallback(void *userdata)
    {
        UnixThread *t = reinterpret_cast<UnixThread *>(userdata);
        t->m_ID = atomic_increment(&s_NextThreadID);
        pthread_setspecific(s_ThreadIDKey, (void *)lmnew ID(t->m_ID));

        t->m_Callback(t->m_User);

        return NULL;
    }

    Thread::ID UnixThread::GetThreadIDFuncUnix()
    {
        void *value = pthread_getspecific(s_NextThreadID);

        if (value)
            return *static_cast<ID *>(value);

        ID new_id = atomic_increment(&s_NextThreadID);
        pthread_setspecific(s_ThreadIDKey, (void *)lmnew ID(new_id));
        return new_id;
    }
    void UnixThread::WaitToFinishFuncUnix(Thread *p_thread)
    {
        UnixThread *tp = static_cast<UnixThread *>(p_thread);
        //LUMOS_CORE_ASSERT(tp);
        //LUMOS_CORE_ASSERT(tp->pthread != 0);

        pthread_join(tp->m_PThread, NULL);
        tp->m_PThread = 0;
    }

    Error UnixThread::SetNameFuncUnix(const String &p_name) 
    {
    #ifdef PTHREAD_NO_RENAME
        return ERR_UNAVAILABLE;

    #else

    #ifdef PTHREAD_RENAME_SELF

        // check if thread is the same as caller
        int err = pthread_setname_np(p_name.c_str());

    #else
        pthread_t running_thread = pthread_self();
    #ifdef PTHREAD_BSD_SET_NAME
        pthread_set_name_np(running_thread, p_name.c_str());
        int err = 0; // Open/FreeBSD ignore errors in this function
    #else
        int err = pthread_setname_np(running_thread, p_name.c_str());
    #endif // PTHREAD_BSD_SET_NAME

    #endif // PTHREAD_RENAME_SELF

        return err == 0 ? OK : ERR_INVALID_PARAMETER;

    #endif // PTHREAD_NO_RENAME
    };
    
    Thread* UnixThread::CreateFuncUnix(ThreadCreateCallback p_callback, void *p_user, const Settings & p_settings)
    {
        return lmnew UnixThread(p_callback, p_user, p_settings);
    }

    void UnixThread::MakeDefault() 
    {
        GetThreadIDFunc = GetThreadIDFuncUnix;
        WaitToFinishFunc = WaitToFinishFuncUnix;
        SetNameFunc = SetNameFuncUnix;
        CreateFunc = CreateFuncUnix;
    }
}
