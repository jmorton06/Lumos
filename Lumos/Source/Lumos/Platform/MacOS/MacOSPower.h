#pragma once
#include "Core/OS/OS.h"
#include <CoreFoundation/CoreFoundation.h>

namespace Lumos
{
    class MacOSPower
    {
    public:
        MacOSPower();
        virtual ~MacOSPower();

        PowerState GetPowerState();
        int GetPowerSecondsLeft();
        int GetPowerPercentageLeft();

    private:
        int m_NumberSecondsLeft;
        int m_PercentageLeft;
        PowerState m_PowerState;

        void CheckPS(CFDictionaryRef dict, bool* have_ac, bool* have_battery, bool* charging);
        bool GetPowerInfo_MacOSX(/*PowerState * state, int *seconds, int *percent*/);
        bool UpdatePowerInfo();
    };
}
