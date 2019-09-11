#pragma once
#include "lmpch.h"

namespace Lumos
{
    enum PowerState
    {
        POWERSTATE_UNKNOWN,
        POWERSTATE_ON_BATTERY,
        POWERSTATE_NO_BATTERY,
        POWERSTATE_CHARGING,
        POWERSTATE_CHARGED
    };
    
    class LUMOS_EXPORT OS
    {
    public:
        OS() {}
        virtual ~OS() {}

        virtual void Run() = 0;
    
        static void Create();
        static void Release();

        static OS* Instance() { return s_Instance; }
        static String PowerStateToString(PowerState state);

    private:

        static OS* s_Instance;
	};
}
