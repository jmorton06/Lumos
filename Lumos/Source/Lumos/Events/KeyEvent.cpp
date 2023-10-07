#include "Precompiled.h"
#include "KeyEvent.h"
#include <sstream>

namespace Lumos
{
    std::string KeyPressedEvent::ToString() const
    {
        std::stringstream ss;
        ss << "KeyPressedEvent: " << uint32_t(m_KeyCode) << " (" << m_RepeatCount << " repeats)";
        return ss.str();
    }

    std::string KeyReleasedEvent::ToString() const
    {
        std::stringstream ss;
        ss << "KeyReleasedEvent: " << uint32_t(m_KeyCode);
        return ss.str();
    }

    std::string KeyTypedEvent::ToString() const
    {
        std::stringstream ss;
        ss << "KeyTypedEvent: " << uint32_t(m_KeyCode);
        return ss.str();
    }

}