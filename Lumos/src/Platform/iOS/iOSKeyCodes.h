#pragma once
#include "lmpch.h"
#include "Core/OS/KeyCodes.h"

namespace Lumos
{
    namespace iOSKeyCodes
    {
        inline u32 iOSKeyToLumos(char key)
        {
            static std::map<char, u32> keyMap =
            {
            { 'a', LUMOS_KEY_A},
            { 'b', LUMOS_KEY_B},
            { 'c', LUMOS_KEY_C},
            { 'd', LUMOS_KEY_D},
            { 'e', LUMOS_KEY_E},
            { 'f', LUMOS_KEY_F},
            { 'g', LUMOS_KEY_G},
            { 'h', LUMOS_KEY_H},
            { 'i', LUMOS_KEY_I},
            { 'j', LUMOS_KEY_J},
            { 'k', LUMOS_KEY_K},
            { 'l', LUMOS_KEY_L},
            { 'm', LUMOS_KEY_M},
            { 'n', LUMOS_KEY_N},
            { 'o', LUMOS_KEY_O},
            { 'p', LUMOS_KEY_P},
            { 'q', LUMOS_KEY_Q},
            { 'r', LUMOS_KEY_R},
            { 's', LUMOS_KEY_S},
            { 't', LUMOS_KEY_T},
            { 'u', LUMOS_KEY_U},
            { 'v', LUMOS_KEY_V},
            { 'w', LUMOS_KEY_W},
            { 'x', LUMOS_KEY_X},
            { 'y', LUMOS_KEY_Y},
            { 'z', LUMOS_KEY_Z},

            { '0', LUMOS_KEY_0},
            { '1', LUMOS_KEY_1},
            { '2', LUMOS_KEY_2},
            { '3', LUMOS_KEY_3},
            { '4', LUMOS_KEY_4},
            { '5', LUMOS_KEY_5},
            { '6', LUMOS_KEY_6},
            { '7', LUMOS_KEY_7},
            { '8', LUMOS_KEY_8},
            { '9', LUMOS_KEY_9},

            { '-', LUMOS_KEY_SUBTRACT      },
            { 0x75, LUMOS_KEY_DELETE       },
            { ' ', LUMOS_KEY_SPACE         },
            { 0x7B, LUMOS_KEY_LEFT         },
            { 0x7C, LUMOS_KEY_RIGHT        },
            { 0x7E, LUMOS_KEY_UP           },
            { 0x7D, LUMOS_KEY_DOWN         },
            { 0x38, LUMOS_KEY_LEFT_SHIFT   },
            { 0x35, LUMOS_KEY_ESCAPE       },
            { '+', LUMOS_KEY_ADD           },
            { 0x33, LUMOS_KEY_BACKSPACE    },
            { 0x24, LUMOS_KEY_ENTER        },
            { ',', LUMOS_KEY_COMMA         }
            };
            
            return keyMap[key];
        }

        inline u32 iOSTouchToLumosMouseKey(u32 count)
		{
            if(count > 3)
                return 0;

			static std::map<u32, u32> keyMap =
			{
			{0, LUMOS_MOUSE_LEFT},
			{1, LUMOS_MOUSE_RIGHT},
			{2, LUMOS_MOUSE_MIDDLE}
			};

			return keyMap[count];
		}
    }
}
