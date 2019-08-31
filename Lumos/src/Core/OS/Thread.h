#pragma once
#include "LM.h"
#include "Core/Error.h"

namespace Lumos
{
    typedef void (*ThreadCreateCallback)(void *p_userdata);

    class Thread
    {
    public:
    	enum class Priority 
        {
		    LOW,
		    NORMAL,
		    HIGH
	    };

        struct Settings 
        {
            Priority priority;
            Settings() { priority = Priority::NORMAL; }
        };

        typedef uint64_t ID;

        virtual ~Thread();

        virtual ID GetID() const = 0;

        static Error SetName(const String &p_name);
        _FORCE_INLINE_ static ID GetMainID() { return s_MainThreadID; } ///< get the ID of the main thread
        static ID GetCallerID(); ///< get the ID of the caller function ID
        static void WaitToFinish(Thread *p_thread); ///< waits until thread is finished, and deallocates it.
        static Thread* Create(ThreadCreateCallback p_callback, void *p_user, const Settings &p_settings = Settings()); 

    protected:
        
        Thread();
        static ID (*get_thread_id_func)();
	    static void (*wait_to_finish_func)(Thread *);
	    static Error (*set_name_func)(const String &);
        
        //static Thread *Create(ThreadCreateCallback p_callback, void *, const Settings &); ///< Create a mutex
        static ID s_MainThreadID;

    };
}
