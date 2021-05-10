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

                { '0', Lumos::InputCode::Key::D0 },
                { '1', Lumos::InputCode::Key::D1 },
                { '2', Lumos::InputCode::Key::D2 },
                { '3', Lumos::InputCode::Key::D3 },
                { '4', Lumos::InputCode::Key::D4 },
                { '5', Lumos::InputCode::Key::D5 },
                { '6', Lumos::InputCode::Key::D6 },
                { '7', Lumos::InputCode::Key::D7 },
                { '8', Lumos::InputCode::Key::D8 },
                { '9', Lumos::InputCode::Key::D9 },

                { '-', Lumos::InputCode::Key::Minus },
                { 0x75, Lumos::InputCode::Key::Delete },
                { ' ', Lumos::InputCode::Key::Space },
                { 0x7B, Lumos::InputCode::Key::Left },
                { 0x7C, Lumos::InputCode::Key::Right },
                { 0x7E, Lumos::InputCode::Key::Up },
                { 0x7D, Lumos::InputCode::Key::Down },
                { 0x38, Lumos::InputCode::Key::LeftShift },
                { 0x35, Lumos::InputCode::Key::Escape },
                { '+', Lumos::InputCode::Key::Equal },
                { 0x33, Lumos::InputCode::Key::Backspace },
                { 0x24, Lumos::InputCode::Key::Enter },
                { ',', Lumos::InputCode::Key::Comma }
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
