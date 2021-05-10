#pragma once

#include "Core/OS/OS.h"

namespace Lumos
{
    class iOSPower
    {
    public:
        iOSPower();
        virtual ~iOSPower();

        PowerState GetPowerState();
        int GetPowerSecondsLeft();
        int GetPowerPercentageLeft();

    private:
        int m_NumberSecondsLeft;
        int m_PercentageLeft;
        PowerState m_PowerState;

        bool UpdatePowerInfo();
    };
}
