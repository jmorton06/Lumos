#pragma once

#include "Event.h"
#include "Core/OS/KeyCodes.h"

#include <sstream>

namespace Lumos
{

    class LUMOS_EXPORT KeyEvent : public Event
    {
    public:
        inline Lumos::InputCode::Key GetKeyCode() const { return m_KeyCode; }

        EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

    protected:
        KeyEvent(Lumos::InputCode::Key keycode)
            : m_KeyCode(keycode)
        {
        }

        Lumos::InputCode::Key m_KeyCode;
    };

    class LUMOS_EXPORT KeyPressedEvent : public KeyEvent
    {
    public:
        KeyPressedEvent(Lumos::InputCode::Key keycode, int repeatCount)
            : KeyEvent(keycode)
            , m_RepeatCount(repeatCount)
        {
        }

        inline int GetRepeatCount() const { return m_RepeatCount; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "KeyPressedEvent: " << uint32_t(m_KeyCode) << " (" << m_RepeatCount << " repeats)";
            return ss.str();
        }

        EVENT_CLASS_TYPE(KeyPressed)
    private:
        int m_RepeatCount;
    };

    class LUMOS_EXPORT KeyReleasedEvent : public KeyEvent
    {
    public:
        KeyReleasedEvent(Lumos::InputCode::Key keycode)
            : KeyEvent(keycode)
        {
        }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "KeyReleasedEvent: " << uint32_t(m_KeyCode);
            return ss.str();
        }

        EVENT_CLASS_TYPE(KeyReleased)
    };

    class LUMOS_EXPORT KeyTypedEvent : public KeyEvent
    {
    public:
        KeyTypedEvent(Lumos::InputCode::Key keycode, char character)
            : KeyEvent(keycode)
            , Character(character)
        {
        }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "KeyPressedEvent: " << uint32_t(m_KeyCode);
            return ss.str();
        }

        EVENT_CLASS_TYPE(KeyTyped)

        char Character;

    private:
    };
}
