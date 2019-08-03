#include "iOSPower.h"


namespace Lumos
{
    iOSPower::iOSPower() :
        m_NumberSecondsLeft(-1),
        m_PercentageLeft(-1),
        m_PowerState(POWERSTATE_UNKNOWN)
    {
    }
    
    iOSPower::~iOSPower()
    {
    }

    bool iOSPower::UpdatePowerInfo()
    {
        return false;
    }

    PowerState iOSPower::GetPowerState()
    {
        if (UpdatePowerInfo())
        {
            return m_PowerState;
        }
        else
        {
            return POWERSTATE_UNKNOWN;
        }
    }

    int iOSPower::GetPowerSecondsLeft()
    {
        if (UpdatePowerInfo())
        {
            return m_NumberSecondsLeft;
        }
        else
        {
            return -1;
        }
    }

    int iOSPower::GetPowerPercentageLeft()
    {
        if (UpdatePowerInfo())
        {
            return m_PercentageLeft;
        }
        else
        {
            return -1;
        }
    }
}
