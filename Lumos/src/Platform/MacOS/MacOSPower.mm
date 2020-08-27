#include "MacOSPower.h"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/ps/IOPSKeys.h>
#include <IOKit/ps/IOPowerSources.h>

#define STRMATCH(a, b) (CFStringCompare(a, b, 0) == kCFCompareEqualTo)
#define GETVAL(k, v) \
CFDictionaryGetValueIfPresent(dict, CFSTR(k), (const void **)v)

namespace Lumos
{
    MacOSPower::MacOSPower() :
        m_NumberSecondsLeft(-1),
        m_PercentageLeft(-1),
        m_PowerState(POWERSTATE_UNKNOWN)
    {
    }
    
    MacOSPower::~MacOSPower()
    {
    }

    void MacOSPower::CheckPS(CFDictionaryRef dict, bool *have_ac, bool *have_battery, bool *charging)
    {
        CFStringRef strval; /* don't CFRelease() this. */
        CFBooleanRef bval;
        CFNumberRef numval;
        bool charge = false;
        bool choose = false;
        bool is_ac = false;
        int secs = -1;
        int maxpct = -1;
        int pct = -1;
        
        if ((GETVAL(kIOPSIsPresentKey, &bval)) && (bval == kCFBooleanFalse))
        {
            return; /* nothing to see here. */
        }
        
        if (!GETVAL(kIOPSPowerSourceStateKey, &strval))
        {
            return;
        }
        
        if (STRMATCH(strval, CFSTR(kIOPSACPowerValue)))
        {
            is_ac = *have_ac = true;
        }
        else if (!STRMATCH(strval, CFSTR(kIOPSBatteryPowerValue)))
        {
            return; /* not a battery? */
        }
        
        if ((GETVAL(kIOPSIsChargingKey, &bval)) && (bval == kCFBooleanTrue))
        {
            charge = true;
        }
        
        if (GETVAL(kIOPSMaxCapacityKey, &numval))
        {
            SInt32 val = -1;
            CFNumberGetValue(numval, kCFNumberSInt32Type, &val);
            if (val > 0)
            {
                *have_battery = true;
                maxpct = (int)val;
            }
        }
        
        if (GETVAL(kIOPSMaxCapacityKey, &numval))
        {
            SInt32 val = -1;
            CFNumberGetValue(numval, kCFNumberSInt32Type, &val);
            if (val > 0)
            {
                *have_battery = true;
                maxpct = (int)val;
            }
        }
        
        if (GETVAL(kIOPSTimeToEmptyKey, &numval))
        {
            SInt32 val = -1;
            CFNumberGetValue(numval, kCFNumberSInt32Type, &val);
            
            /* Mac OS X reports 0 minutes until empty if you're plugged in. :( */
            if ((val == 0) && (is_ac))
            {
                val = -1; /* !!! FIXME: calc from timeToFull and capacity? */
            }
            
            secs = (int)val;
            if (secs > 0)
            {
                secs *= 60; /* value is in minutes, so convert to seconds. */
            }
        }
        
        if (GETVAL(kIOPSCurrentCapacityKey, &numval))
        {
            SInt32 val = -1;
            CFNumberGetValue(numval, kCFNumberSInt32Type, &val);
            pct = (int)val;
        }
        
        if ((pct > 0) && (maxpct > 0))
        {
            pct = (int)((((double)pct) / ((double)maxpct)) * 100.0);
        }
        
        if (pct > 100)
        {
            pct = 100;
        }
        
        /*
         * We pick the battery that claims to have the most minutes left.
         *  (failing a report of minutes, we'll take the highest percent.)
         */
        if ((secs < 0) && (m_NumberSecondsLeft < 0))
        {
            if ((pct < 0) && (m_PercentageLeft < 0))
            {
                choose = true; /* at least we know there's a battery. */
            }
            if (pct > m_PercentageLeft)
            {
                choose = true;
            }
        }
        else if (secs > m_NumberSecondsLeft)
        {
            choose = true;
        }
        
        if (choose)
        {
            m_NumberSecondsLeft = secs;
            m_PercentageLeft = pct;
            *charging = charge;
        }
    }

#undef GETVAL
#undef STRMATCH

    bool MacOSPower::GetPowerInfo_MacOSX()
    {
        CFTypeRef blob = IOPSCopyPowerSourcesInfo();
        
        m_NumberSecondsLeft = -1;
        m_PercentageLeft = -1;
        m_PowerState = POWERSTATE_UNKNOWN;
        
        if (blob != NULL)
        {
            CFArrayRef list = IOPSCopyPowerSourcesList(blob);
            if (list != NULL)
            {
                /* don't CFRelease() the list items, or dictionaries! */
                bool have_ac = false;
                bool have_battery = false;
                bool charging = false;
                const CFIndex total = CFArrayGetCount(list);
                CFIndex i;
                for (i = 0; i < total; i++)
                {
                    CFTypeRef ps = (CFTypeRef)CFArrayGetValueAtIndex(list, i);
                    CFDictionaryRef dict = IOPSGetPowerSourceDescription(blob, ps);
                    if (dict != NULL)
                    {
                        CheckPS(dict, &have_ac, &have_battery, &charging);
                    }
                }
                
                if (!have_battery)
                {
                    m_PowerState = POWERSTATE_NO_BATTERY;
                }
                else if (charging)
                {
                    m_PowerState = POWERSTATE_CHARGING;
                }
                else if (have_ac)
                {
                    m_PowerState = POWERSTATE_CHARGED;
                }
                else
                {
                    m_PowerState = POWERSTATE_ON_BATTERY;
                }
                
                CFRelease(list);
            }
            CFRelease(blob);
        }
        
        return true; /* always the definitive answer on Mac OS X. */
    }

    bool MacOSPower::UpdatePowerInfo()
    {
        if (GetPowerInfo_MacOSX())
        {
            return true;
        }
        return false;
    }

    PowerState MacOSPower::GetPowerState()
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

    int MacOSPower::GetPowerSecondsLeft()
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

    int MacOSPower::GetPowerPercentageLeft()
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
