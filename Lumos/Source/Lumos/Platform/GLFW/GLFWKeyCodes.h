#pragma once

#include "Core/OS/KeyCodes.h"

#include <GLFW/glfw3.h>

namespace Lumos
{
    namespace GLFWKeyCodes
    {
        static Lumos::InputCode::Key GLFWToLumosKeyboardKey(uint32_t glfwKey)
        {
            static std::map<uint32_t, Lumos::InputCode::Key> keyMap = {
                { GLFW_KEY_A, Lumos::InputCode::Key::A },
                { GLFW_KEY_B, Lumos::InputCode::Key::B },
                { GLFW_KEY_C, Lumos::InputCode::Key::C },
                { GLFW_KEY_D, Lumos::InputCode::Key::D },
                { GLFW_KEY_E, Lumos::InputCode::Key::E },
                { GLFW_KEY_F, Lumos::InputCode::Key::F },
                { GLFW_KEY_G, Lumos::InputCode::Key::G },
                { GLFW_KEY_H, Lumos::InputCode::Key::H },
                { GLFW_KEY_I, Lumos::InputCode::Key::I },
                { GLFW_KEY_J, Lumos::InputCode::Key::J },
                { GLFW_KEY_K, Lumos::InputCode::Key::K },
                { GLFW_KEY_L, Lumos::InputCode::Key::L },
                { GLFW_KEY_M, Lumos::InputCode::Key::M },
                { GLFW_KEY_N, Lumos::InputCode::Key::N },
                { GLFW_KEY_O, Lumos::InputCode::Key::O },
                { GLFW_KEY_P, Lumos::InputCode::Key::P },
                { GLFW_KEY_Q, Lumos::InputCode::Key::Q },
                { GLFW_KEY_R, Lumos::InputCode::Key::R },
                { GLFW_KEY_S, Lumos::InputCode::Key::S },
                { GLFW_KEY_T, Lumos::InputCode::Key::T },
                { GLFW_KEY_U, Lumos::InputCode::Key::U },
                { GLFW_KEY_V, Lumos::InputCode::Key::V },
                { GLFW_KEY_W, Lumos::InputCode::Key::W },
                { GLFW_KEY_X, Lumos::InputCode::Key::X },
                { GLFW_KEY_Y, Lumos::InputCode::Key::Y },
                { GLFW_KEY_Z, Lumos::InputCode::Key::Z },

                { GLFW_KEY_0, Lumos::InputCode::Key::D0 },
                { GLFW_KEY_1, Lumos::InputCode::Key::D1 },
                { GLFW_KEY_2, Lumos::InputCode::Key::D2 },
                { GLFW_KEY_3, Lumos::InputCode::Key::D3 },
                { GLFW_KEY_4, Lumos::InputCode::Key::D4 },
                { GLFW_KEY_5, Lumos::InputCode::Key::D5 },
                { GLFW_KEY_6, Lumos::InputCode::Key::D6 },
                { GLFW_KEY_7, Lumos::InputCode::Key::D7 },
                { GLFW_KEY_8, Lumos::InputCode::Key::D8 },
                { GLFW_KEY_9, Lumos::InputCode::Key::D9 },

                { GLFW_KEY_F1, Lumos::InputCode::Key::F1 },
                { GLFW_KEY_F2, Lumos::InputCode::Key::F2 },
                { GLFW_KEY_F3, Lumos::InputCode::Key::F3 },
                { GLFW_KEY_F4, Lumos::InputCode::Key::F4 },
                { GLFW_KEY_F5, Lumos::InputCode::Key::F5 },
                { GLFW_KEY_F6, Lumos::InputCode::Key::F6 },
                { GLFW_KEY_F7, Lumos::InputCode::Key::F7 },
                { GLFW_KEY_F8, Lumos::InputCode::Key::F8 },
                { GLFW_KEY_F9, Lumos::InputCode::Key::F9 },
                { GLFW_KEY_F10, Lumos::InputCode::Key::F10 },
                { GLFW_KEY_F11, Lumos::InputCode::Key::F11 },
                { GLFW_KEY_F12, Lumos::InputCode::Key::F12 },

                { GLFW_KEY_MINUS, Lumos::InputCode::Key::Minus },
                { GLFW_KEY_DELETE, Lumos::InputCode::Key::Delete },
                { GLFW_KEY_SPACE, Lumos::InputCode::Key::Space },
                { GLFW_KEY_LEFT, Lumos::InputCode::Key::Left },
                { GLFW_KEY_RIGHT, Lumos::InputCode::Key::Right },
                { GLFW_KEY_UP, Lumos::InputCode::Key::Up },
                { GLFW_KEY_DOWN, Lumos::InputCode::Key::Down },
                { GLFW_KEY_LEFT_SHIFT, Lumos::InputCode::Key::LeftShift },
                { GLFW_KEY_RIGHT_SHIFT, Lumos::InputCode::Key::RightShift },
                { GLFW_KEY_ESCAPE, Lumos::InputCode::Key::Escape },
                { GLFW_KEY_KP_ADD, Lumos::InputCode::Key::A },
                { GLFW_KEY_COMMA, Lumos::InputCode::Key::Comma },
                { GLFW_KEY_BACKSPACE, Lumos::InputCode::Key::Backspace },
                { GLFW_KEY_ENTER, Lumos::InputCode::Key::Enter },
                { GLFW_KEY_LEFT_SUPER, Lumos::InputCode::Key::LeftSuper },
                { GLFW_KEY_RIGHT_SUPER, Lumos::InputCode::Key::RightSuper },
                { GLFW_KEY_LEFT_ALT, Lumos::InputCode::Key::LeftAlt },
                { GLFW_KEY_RIGHT_ALT, Lumos::InputCode::Key::RightAlt },
                { GLFW_KEY_LEFT_CONTROL, Lumos::InputCode::Key::LeftControl },
                { GLFW_KEY_RIGHT_CONTROL, Lumos::InputCode::Key::RightControl }
            };

            return keyMap[glfwKey];
        }

        static Lumos::InputCode::MouseKey GLFWToLumosMouseKey(uint32_t glfwKey)
        {

            static std::map<uint32_t, Lumos::InputCode::MouseKey> keyMap = {
                { GLFW_MOUSE_BUTTON_LEFT, Lumos::InputCode::MouseKey::ButtonLeft },
                { GLFW_MOUSE_BUTTON_RIGHT, Lumos::InputCode::MouseKey::ButtonRight },
                { GLFW_MOUSE_BUTTON_MIDDLE, Lumos::InputCode::MouseKey::ButtonMiddle }
            };

            return keyMap[glfwKey];
        }
    }
}
