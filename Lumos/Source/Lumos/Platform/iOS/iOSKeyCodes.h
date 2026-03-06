#pragma once

#include "Core/OS/KeyCodes.h"
#include <map>

namespace Lumos
{
    namespace iOSKeyCodes
    {
        inline Lumos::InputCode::Key iOSKeyToLumos(char key)
        {
            static std::map<char, Lumos::InputCode::Key> keyMap = {
                { 'a', Lumos::InputCode::Key::A },
                { 'b', Lumos::InputCode::Key::B },
                { 'c', Lumos::InputCode::Key::C },
                { 'd', Lumos::InputCode::Key::D },
                { 'e', Lumos::InputCode::Key::E },
                { 'f', Lumos::InputCode::Key::F },
                { 'g', Lumos::InputCode::Key::G },
                { 'h', Lumos::InputCode::Key::H },
                { 'i', Lumos::InputCode::Key::I },
                { 'j', Lumos::InputCode::Key::J },
                { 'k', Lumos::InputCode::Key::K },
                { 'l', Lumos::InputCode::Key::L },
                { 'm', Lumos::InputCode::Key::M },
                { 'n', Lumos::InputCode::Key::N },
                { 'o', Lumos::InputCode::Key::O },
                { 'p', Lumos::InputCode::Key::P },
                { 'q', Lumos::InputCode::Key::Q },
                { 'r', Lumos::InputCode::Key::R },
                { 's', Lumos::InputCode::Key::S },
                { 't', Lumos::InputCode::Key::T },
                { 'u', Lumos::InputCode::Key::U },
                { 'v', Lumos::InputCode::Key::V },
                { 'w', Lumos::InputCode::Key::W },
                { 'x', Lumos::InputCode::Key::X },
                { 'y', Lumos::InputCode::Key::Y },
                { 'z', Lumos::InputCode::Key::Z },

                // Special keys (iOS hardware keycodes) — must come before ASCII chars
                // to avoid collisions ('0'=0x30=Tab, '3'=0x33=Backspace, '5'=0x35=Escape, '8'=0x38=Shift)
                { 0x24, Lumos::InputCode::Key::Enter },
                { 0x30, Lumos::InputCode::Key::Tab },
                { 0x33, Lumos::InputCode::Key::Backspace },
                { 0x35, Lumos::InputCode::Key::Escape },
                { 0x38, Lumos::InputCode::Key::LeftShift },
                { 0x75, Lumos::InputCode::Key::Delete },
                { 0x7B, Lumos::InputCode::Key::Left },
                { 0x7C, Lumos::InputCode::Key::Right },
                { 0x7D, Lumos::InputCode::Key::Down },
                { 0x7E, Lumos::InputCode::Key::Up },

                // ASCII chars (digits excluded — '0','3','5','8' collide with special keycodes above)
                // Digit keys are handled via insertText: / OnKeyTyped instead
                { ' ', Lumos::InputCode::Key::Space },
                { '-', Lumos::InputCode::Key::Minus },
                { '+', Lumos::InputCode::Key::Equal },
                { ',', Lumos::InputCode::Key::Comma },
                { '\t', Lumos::InputCode::Key::Tab },
                { '\r', Lumos::InputCode::Key::Enter },
                { '\n', Lumos::InputCode::Key::Enter }
            };

            return keyMap[key];
        }

        inline Lumos::InputCode::MouseKey iOSTouchToLumosMouseKey(uint32_t count)
        {
            if(count > 3)
                return Lumos::InputCode::MouseKey::ButtonLeft;

            static std::map<uint32_t, Lumos::InputCode::MouseKey> keyMap = {
                { 0, Lumos::InputCode::MouseKey::ButtonLeft },
                { 1, Lumos::InputCode::MouseKey::ButtonRight },
                { 2, Lumos::InputCode::MouseKey::ButtonMiddle }
            };
            return keyMap[count];
        }
    }
}
