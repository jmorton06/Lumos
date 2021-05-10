#pragma once

#include "Core/OS/KeyCodes.h"

namespace Lumos
{
    namespace WindowsKeyCodes
    {
        // http://msdn.microsoft.com/en-us/library/ms645540(VS.85).aspx
        enum KeyboardKeys : uint32_t
        {
            KEYBOARD_LBUTTON = 0x01, // Left mouse button
            KEYBOARD_RBUTTON = 0x02, // Right mouse button
            KEYBOARD_CANCEL = 0x03, // Control-break processing
            KEYBOARD_MBUTTON = 0x04, // Middle mouse button (three-button mouse)
            KEYBOARD_XBUTTON1 = 0x05, // Windows 2000/XP: X1 mouse button
            KEYBOARD_XBUTTON2 = 0x06, // Windows 2000/XP: X2 mouse button
            KEYBOARD_BACK = 0x08, // BACKSPACE key
            KEYBOARD_TAB = 0x09, // TAB key
            KEYBOARD_CLEAR = 0x0C, // CLEAR key
            KEYBOARD_RETURN = 0x0D, // ENTER key
            KEYBOARD_SHIFT = 0x10, // SHIFT key
            KEYBOARD_CONTROL = 0x11, // CTRL key
            KEYBOARD_MENU = 0x12, // ALT key
            KEYBOARD_PAUSE = 0x13, // PAUSE key
            KEYBOARD_CAPITAL = 0x14, // CAPS LOCK key
            KEYBOARD_KANA = 0x15, // IME Kana mode
            KEYBOARD_HANGUEL = 0x15, // IME Hanguel mode (maintained for compatibility use KEYBOARD_HANGUL)
            KEYBOARD_HANGUL = 0x15, // IME Hangul mode
            KEYBOARD_JUNJA = 0x17, // IME Junja mode
            KEYBOARD_FINAL = 0x18, // IME final mode
            KEYBOARD_HANJA = 0x19, // IME Hanja mode
            KEYBOARD_KANJI = 0x19, // IME Kanji mode
            KEYBOARD_ESCAPE = 0x1B, // ESC key
            KEYBOARD_CONVERT = 0x1C, // IME convert
            KEYBOARD_NONCONVERT = 0x1D, // IME nonconvert
            KEYBOARD_ACCEPT = 0x1E, // IME accept
            KEYBOARD_MODECHANGE = 0x1F, // IME mode change request
            KEYBOARD_SPACE = 0x20, // SPACEBAR
            KEYBOARD_PRIOR = 0x21, // PAGE UP key
            KEYBOARD_NEXT = 0x22, // PAGE DOWN key
            KEYBOARD_END = 0x23, // END key
            KEYBOARD_HOME = 0x24, // HOME key
            KEYBOARD_LEFT = 0x25, // LEFT ARROW key
            KEYBOARD_UP = 0x26, // UP ARROW key
            KEYBOARD_RIGHT = 0x27, // RIGHT ARROW key
            KEYBOARD_DOWN = 0x28, // DOWN ARROW key
            KEYBOARD_SELECT = 0x29, // SELECT key
            KEYBOARD_PRINT = 0x2A, // PRINT key
            KEYBOARD_EXECUT = 0x2B, // EXECUTE key
            KEYBOARD_SNAPSHOT = 0x2C, // PRINT SCREEN key
            KEYBOARD_INSERT = 0x2D, // INS key
            KEYBOARD_DELETE = 0x2E, // DEL key
            KEYBOARD_HELP = 0x2F, // HELP key
            KEYBOARD_0 = 0x30, // 0 key
            KEYBOARD_1 = 0x31, // 1 key
            KEYBOARD_2 = 0x32, // 2 key
            KEYBOARD_3 = 0x33, // 3 key
            KEYBOARD_4 = 0x34, // 4 key
            KEYBOARD_5 = 0x35, // 5 key
            KEYBOARD_6 = 0x36, // 6 key
            KEYBOARD_7 = 0x37, // 7 key
            KEYBOARD_8 = 0x38, // 8 key
            KEYBOARD_9 = 0x39, // 9 key
            KEYBOARD_A = 0x41, // A key
            KEYBOARD_B = 0x42, // B key
            KEYBOARD_C = 0x43, // C key
            KEYBOARD_D = 0x44, // D key
            KEYBOARD_E = 0x45, // E key
            KEYBOARD_F = 0x46, // F key
            KEYBOARD_G = 0x47, // G key
            KEYBOARD_H = 0x48, // H key
            KEYBOARD_I = 0x49, // I key
            KEYBOARD_J = 0x4A, // J key
            KEYBOARD_K = 0x4B, // K key
            KEYBOARD_L = 0x4C, // L key
            KEYBOARD_M = 0x4D, // M key
            KEYBOARD_N = 0x4E, // N key
            KEYBOARD_O = 0x4F, // O key
            KEYBOARD_P = 0x50, // P key
            KEYBOARD_Q = 0x51, // Q key
            KEYBOARD_R = 0x52, // R key
            KEYBOARD_S = 0x53, // S key
            KEYBOARD_T = 0x54, // T key
            KEYBOARD_U = 0x55, // U key
            KEYBOARD_V = 0x56, // V key
            KEYBOARD_W = 0x57, // W key
            KEYBOARD_X = 0x58, // X key
            KEYBOARD_Y = 0x59, // Y key
            KEYBOARD_Z = 0x5A, // Z key
            KEYBOARD_LWIN = 0x5B, // Left Windows key (Microsoft� Natural� keyboard)
            KEYBOARD_RWIN = 0x5C, // Right Windows key (Natural keyboard)
            KEYBOARD_APPS = 0x5D, // Applications key (Natural keyboard)
            KEYBOARD_SLEEP = 0x5F, // Computer Sleep key
            KEYBOARD_NUMPAD0 = 0x60, // Numeric keypad 0 key
            KEYBOARD_NUMPAD1 = 0x61, // Numeric keypad 1 key
            KEYBOARD_NUMPAD2 = 0x62, // Numeric keypad 2 key
            KEYBOARD_NUMPAD3 = 0x63, // Numeric keypad 3 key
            KEYBOARD_NUMPAD4 = 0x64, // Numeric keypad 4 key
            KEYBOARD_NUMPAD5 = 0x65, // Numeric keypad 5 key
            KEYBOARD_NUMPAD6 = 0x66, // Numeric keypad 6 key
            KEYBOARD_NUMPAD7 = 0x67, // Numeric keypad 7 key
            KEYBOARD_NUMPAD8 = 0x68, // Numeric keypad 8 key
            KEYBOARD_NUMPAD9 = 0x69, // Numeric keypad 9 key
            KEYBOARD_MULTIPLY = 0x6A, // Multiply key
            KEYBOARD_ADD = 0x6B, // Add key
            KEYBOARD_SEPARATOR = 0x6C, // Separator key
            KEYBOARD_SUBTRACT = 0x6D, // Subtract key
            KEYBOARD_DECIMAL = 0x6E, // Decimal key
            KEYBOARD_DIVIDE = 0x6F, // Divide key
            KEYBOARD_F1 = 0x70, // F1 key
            KEYBOARD_F2 = 0x71, // F2 key
            KEYBOARD_F3 = 0x72, // F3 key
            KEYBOARD_F4 = 0x73, // F4 key
            KEYBOARD_F5 = 0x74, // F5 key
            KEYBOARD_F6 = 0x75, // F6 key
            KEYBOARD_F7 = 0x76, // F7 key
            KEYBOARD_F8 = 0x77, // F8 key
            KEYBOARD_F9 = 0x78, // F9 key
            KEYBOARD_F10 = 0x79, // F10 key
            KEYBOARD_F11 = 0x7A, // F11 key
            KEYBOARD_F12 = 0x7B, // F12 key
            KEYBOARD_F13 = 0x7C, // F13 key
            KEYBOARD_F14 = 0x7D, // F14 key
            KEYBOARD_F15 = 0x7E, // F15 key
            KEYBOARD_F16 = 0x7F, // F16 key
            KEYBOARD_F17 = 0x80, // F17 key
            KEYBOARD_F18 = 0x81, // F18 key
            KEYBOARD_F19 = 0x82, // F19 key
            KEYBOARD_F20 = 0x83, // F20 key
            KEYBOARD_F21 = 0x84, // F21 key
            KEYBOARD_F22 = 0x85, // F22 key
            KEYBOARD_F23 = 0x86, // F23 key
            KEYBOARD_F24 = 0x87, // F24 key
            KEYBOARD_NUMLOCK = 0x90, // NUM LOCK key
            KEYBOARD_SCROLL = 0x91, // SCROLL LOCK key
            KEYBOARD_LSHIFT = 16, //TODO: Temp //0xA0,     // Left SHIFT key
            KEYBOARD_RSHIFT = 0xA1, // Right SHIFT key
            KEYBOARD_LCONTROL = 0xA2, // Left CONTROL key
            KEYBOARD_RCONTROL = 0xA3, // Right CONTROL key
            KEYBOARD_LMENU = 0xA4, // Left MENU key
            KEYBOARD_RMENU = 0xA5, // Right MENU key
            KEYBOARD_PLUS = 0xBB, // Plus Key   (+)
            KEYBOARD_COMMA = 0xBC, // Comma Key  (,)
            KEYBOARD_MINUS = 0xBD, // Minus Key  (-)
            KEYBOARD_PERIOD = 0xBE, // Period Key (.)
            KEYBOARD_ATTN = 0xF6, // Attn key
            KEYBOARD_CRSEL = 0xF7, // CrSel key
            KEYBOARD_EXSEL = 0xF8, // ExSel key
            KEYBOARD_EREOF = 0xF9, // Erase EOF key
            KEYBOARD_PLAY = 0xFA, // Play key
            KEYBOARD_ZOOM = 0xFB, // Zoom key
            KEYBOARD_PA1 = 0xFD, // PA1 key
            KEYBOARD_OEM_CLEAR = 0xFE, // Clear key
            KEYBOARD_MAX = 0xFF
        };

        inline Lumos::InputCode::Key WindowsKeyToLumos(uint32_t key)
        {
            static std::map<uint32_t, Lumos::InputCode::Key> keyMap = {
                { KEYBOARD_A, Lumos::InputCode::Key::A },
                { KEYBOARD_B, Lumos::InputCode::Key::B },
                { KEYBOARD_C, Lumos::InputCode::Key::C },
                { KEYBOARD_D, Lumos::InputCode::Key::D },
                { KEYBOARD_E, Lumos::InputCode::Key::E },
                { KEYBOARD_F, Lumos::InputCode::Key::F },
                { KEYBOARD_G, Lumos::InputCode::Key::G },
                { KEYBOARD_H, Lumos::InputCode::Key::H },
                { KEYBOARD_I, Lumos::InputCode::Key::I },
                { KEYBOARD_J, Lumos::InputCode::Key::J },
                { KEYBOARD_K, Lumos::InputCode::Key::K },
                { KEYBOARD_L, Lumos::InputCode::Key::L },
                { KEYBOARD_M, Lumos::InputCode::Key::M },
                { KEYBOARD_N, Lumos::InputCode::Key::N },
                { KEYBOARD_O, Lumos::InputCode::Key::O },
                { KEYBOARD_P, Lumos::InputCode::Key::P },
                { KEYBOARD_Q, Lumos::InputCode::Key::Q },
                { KEYBOARD_R, Lumos::InputCode::Key::R },
                { KEYBOARD_S, Lumos::InputCode::Key::S },
                { KEYBOARD_T, Lumos::InputCode::Key::T },
                { KEYBOARD_U, Lumos::InputCode::Key::U },
                { KEYBOARD_V, Lumos::InputCode::Key::V },
                { KEYBOARD_W, Lumos::InputCode::Key::W },
                { KEYBOARD_X, Lumos::InputCode::Key::X },
                { KEYBOARD_Y, Lumos::InputCode::Key::Y },
                { KEYBOARD_Z, Lumos::InputCode::Key::Z },

                { KEYBOARD_0, Lumos::InputCode::Key::D0 },
                { KEYBOARD_1, Lumos::InputCode::Key::D1 },
                { KEYBOARD_2, Lumos::InputCode::Key::D2 },
                { KEYBOARD_3, Lumos::InputCode::Key::D3 },
                { KEYBOARD_4, Lumos::InputCode::Key::D4 },
                { KEYBOARD_5, Lumos::InputCode::Key::D5 },
                { KEYBOARD_6, Lumos::InputCode::Key::D6 },
                { KEYBOARD_7, Lumos::InputCode::Key::D7 },
                { KEYBOARD_8, Lumos::InputCode::Key::D8 },
                { KEYBOARD_9, Lumos::InputCode::Key::D9 },

                { KEYBOARD_SUBTRACT, Lumos::InputCode::Key::Minus },
                { KEYBOARD_DELETE, Lumos::InputCode::Key::Delete },
                { KEYBOARD_SPACE, Lumos::InputCode::Key::Space },
                { KEYBOARD_LEFT, Lumos::InputCode::Key::Left },
                { KEYBOARD_RIGHT, Lumos::InputCode::Key::Right },
                { KEYBOARD_UP, Lumos::InputCode::Key::Up },
                { KEYBOARD_DOWN, Lumos::InputCode::Key::Down },
                { KEYBOARD_LSHIFT, Lumos::InputCode::Key::LeftShift },
                { KEYBOARD_RSHIFT, Lumos::InputCode::Key::RightShift },
                { KEYBOARD_LCONTROL, Lumos::InputCode::Key::LeftControl },
                { KEYBOARD_RCONTROL, Lumos::InputCode::Key::RightControl },
                { KEYBOARD_LWIN, Lumos::InputCode::Key::LeftSuper },
                { KEYBOARD_RWIN, Lumos::InputCode::Key::RightSuper },
                { KEYBOARD_LMENU, Lumos::InputCode::Key::Menu },
                { KEYBOARD_ESCAPE, Lumos::InputCode::Key::Escape },
                { KEYBOARD_ADD, Lumos::InputCode::Key::Equal },
                { KEYBOARD_BACK, Lumos::InputCode::Key::Backspace },
                { KEYBOARD_RETURN, Lumos::InputCode::Key::Enter },
                { KEYBOARD_COMMA, Lumos::InputCode::Key::Comma }
            };

            return keyMap[key];
        }
    }
}