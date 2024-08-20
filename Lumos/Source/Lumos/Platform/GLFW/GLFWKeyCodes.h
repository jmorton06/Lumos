#pragma once

#include "Core/OS/KeyCodes.h"

#include <GLFW/glfw3.h>

namespace Lumos
{
    namespace GLFWKeyCodes
    {
        static Lumos::InputCode::Key GLFWToLumosKeyboardKey(uint32_t glfwKey)
        {
            constexpr int GLFW_KEY_RANGE = 349; // Define the range of GLFW key codes you want to handle
            static Lumos::InputCode::Key keyMapArray[GLFW_KEY_RANGE];
            static bool arrayInitialised = false;
            if(!arrayInitialised)
            {
                arrayInitialised = true;
                keyMapArray[GLFW_KEY_A] = Lumos::InputCode::Key::A;
                keyMapArray[GLFW_KEY_B] = Lumos::InputCode::Key::B;
                keyMapArray[GLFW_KEY_C] = Lumos::InputCode::Key::C;
                keyMapArray[GLFW_KEY_D] = Lumos::InputCode::Key::D;
                keyMapArray[GLFW_KEY_E] = Lumos::InputCode::Key::E;
                keyMapArray[GLFW_KEY_F] = Lumos::InputCode::Key::F;
                keyMapArray[GLFW_KEY_G] = Lumos::InputCode::Key::G;
                keyMapArray[GLFW_KEY_H] = Lumos::InputCode::Key::H;
                keyMapArray[GLFW_KEY_I] = Lumos::InputCode::Key::I;
                keyMapArray[GLFW_KEY_J] = Lumos::InputCode::Key::J;
                keyMapArray[GLFW_KEY_K] = Lumos::InputCode::Key::K;
                keyMapArray[GLFW_KEY_L] = Lumos::InputCode::Key::L;
                keyMapArray[GLFW_KEY_M] = Lumos::InputCode::Key::M;
                keyMapArray[GLFW_KEY_N] = Lumos::InputCode::Key::N;
                keyMapArray[GLFW_KEY_O] = Lumos::InputCode::Key::O;
                keyMapArray[GLFW_KEY_P] = Lumos::InputCode::Key::P;
                keyMapArray[GLFW_KEY_Q] = Lumos::InputCode::Key::Q;
                keyMapArray[GLFW_KEY_R] = Lumos::InputCode::Key::R;
                keyMapArray[GLFW_KEY_S] = Lumos::InputCode::Key::S;
                keyMapArray[GLFW_KEY_T] = Lumos::InputCode::Key::T;
                keyMapArray[GLFW_KEY_U] = Lumos::InputCode::Key::U;
                keyMapArray[GLFW_KEY_V] = Lumos::InputCode::Key::V;
                keyMapArray[GLFW_KEY_W] = Lumos::InputCode::Key::W;
                keyMapArray[GLFW_KEY_X] = Lumos::InputCode::Key::X;
                keyMapArray[GLFW_KEY_Y] = Lumos::InputCode::Key::Y;
                keyMapArray[GLFW_KEY_Z] = Lumos::InputCode::Key::Z;
            
                keyMapArray[GLFW_KEY_0] = Lumos::InputCode::Key::D0;
                keyMapArray[GLFW_KEY_1] = Lumos::InputCode::Key::D1;
                keyMapArray[GLFW_KEY_2] = Lumos::InputCode::Key::D2;
                keyMapArray[GLFW_KEY_3] = Lumos::InputCode::Key::D3;
                keyMapArray[GLFW_KEY_4] = Lumos::InputCode::Key::D4;
                keyMapArray[GLFW_KEY_5] = Lumos::InputCode::Key::D5;
                keyMapArray[GLFW_KEY_6] = Lumos::InputCode::Key::D6;
                keyMapArray[GLFW_KEY_7] = Lumos::InputCode::Key::D7;
                keyMapArray[GLFW_KEY_8] = Lumos::InputCode::Key::D8;
                keyMapArray[GLFW_KEY_9] = Lumos::InputCode::Key::D9;
            
                keyMapArray[GLFW_KEY_F1] = Lumos::InputCode::Key::F1;
                keyMapArray[GLFW_KEY_F2] = Lumos::InputCode::Key::F2;
                keyMapArray[GLFW_KEY_F3] = Lumos::InputCode::Key::F3;
                keyMapArray[GLFW_KEY_F4] = Lumos::InputCode::Key::F4;
                keyMapArray[GLFW_KEY_F5] = Lumos::InputCode::Key::F5;
                keyMapArray[GLFW_KEY_F6] = Lumos::InputCode::Key::F6;
                keyMapArray[GLFW_KEY_F7] = Lumos::InputCode::Key::F7;
                keyMapArray[GLFW_KEY_F8] = Lumos::InputCode::Key::F8;
                keyMapArray[GLFW_KEY_F9] = Lumos::InputCode::Key::F9;
                keyMapArray[GLFW_KEY_F10] = Lumos::InputCode::Key::F10;
                keyMapArray[GLFW_KEY_F11] = Lumos::InputCode::Key::F11;
                keyMapArray[GLFW_KEY_F12] = Lumos::InputCode::Key::F12;
            
                keyMapArray[GLFW_KEY_MINUS] = Lumos::InputCode::Key::Minus;
                keyMapArray[GLFW_KEY_DELETE] = Lumos::InputCode::Key::Delete;
                keyMapArray[GLFW_KEY_SPACE] = Lumos::InputCode::Key::Space;
                keyMapArray[GLFW_KEY_LEFT] = Lumos::InputCode::Key::Left;
                keyMapArray[GLFW_KEY_RIGHT] = Lumos::InputCode::Key::Right;
                keyMapArray[GLFW_KEY_UP] = Lumos::InputCode::Key::Up;
                keyMapArray[GLFW_KEY_DOWN] = Lumos::InputCode::Key::Down;
                keyMapArray[GLFW_KEY_LEFT_SHIFT] = Lumos::InputCode::Key::LeftShift;
                keyMapArray[GLFW_KEY_RIGHT_SHIFT] = Lumos::InputCode::Key::RightShift;
                keyMapArray[GLFW_KEY_ESCAPE] = Lumos::InputCode::Key::Escape;
                keyMapArray[GLFW_KEY_KP_ADD] = Lumos::InputCode::Key::KPAdd;
                keyMapArray[GLFW_KEY_COMMA] = Lumos::InputCode::Key::Comma;
                keyMapArray[GLFW_KEY_BACKSPACE] = Lumos::InputCode::Key::Backspace;
                keyMapArray[GLFW_KEY_ENTER] = Lumos::InputCode::Key::Enter;
                keyMapArray[GLFW_KEY_LEFT_SUPER] = Lumos::InputCode::Key::LeftSuper;
                keyMapArray[GLFW_KEY_RIGHT_SUPER] = Lumos::InputCode::Key::RightSuper;
                keyMapArray[GLFW_KEY_LEFT_ALT] = Lumos::InputCode::Key::LeftAlt;
                keyMapArray[GLFW_KEY_RIGHT_ALT] = Lumos::InputCode::Key::RightAlt;
                keyMapArray[GLFW_KEY_LEFT_CONTROL] = Lumos::InputCode::Key::LeftControl;
                keyMapArray[GLFW_KEY_RIGHT_CONTROL] = Lumos::InputCode::Key::RightControl;
                keyMapArray[GLFW_KEY_TAB] = Lumos::InputCode::Key::Tab;
            }

            return keyMapArray[glfwKey];
        }

        static Lumos::InputCode::MouseKey GLFWToLumosMouseKey(uint32_t glfwKey)
        {
            constexpr int GLFW_KEY_RANGE = 4; // Define the range of GLFW key codes you want to handle
            static Lumos::InputCode::MouseKey keyMap[GLFW_KEY_RANGE];
            static bool arrayInitialised = false;
            if(!arrayInitialised)
            {
                arrayInitialised = true;
                keyMap[GLFW_MOUSE_BUTTON_LEFT] = Lumos::InputCode::MouseKey::ButtonLeft;
                keyMap[GLFW_MOUSE_BUTTON_RIGHT] = Lumos::InputCode::MouseKey::ButtonRight;
                keyMap[GLFW_MOUSE_BUTTON_MIDDLE] = Lumos::InputCode::MouseKey::ButtonMiddle;
            }
            return keyMap[glfwKey];
        }
    }
}
