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
        OS() = default;
        virtual ~OS()  = default;

        virtual void Run() = 0;
    
        static void Create();
        static void Release();

        static OS* Instance() { return s_Instance; }
        static String PowerStateToString(PowerState state);

        virtual String GetExecutablePath() = 0;
        virtual String GetAssetPath() { return ""; };
        virtual void Vibrate() const {};

    protected:

        static OS* s_Instance;
	};
}
