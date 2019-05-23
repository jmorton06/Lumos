#pragma once 
#include "LM.h"
#include "App/KeyCodes.h"

#include <GLFW/glfw3.h>

namespace lumos
{
	namespace GLFWKeyCodes
	{
		static uint GLFWToLumosKeyboardKey(uint glfwKey)
		{
			static std::map<uint, uint> keyMap =
			{
			{GLFW_KEY_A, LUMOS_KEY_A},
			{GLFW_KEY_B, LUMOS_KEY_B},
			{GLFW_KEY_C, LUMOS_KEY_C},
			{GLFW_KEY_D, LUMOS_KEY_D},
			{GLFW_KEY_E, LUMOS_KEY_E},
			{GLFW_KEY_F, LUMOS_KEY_F},
			{GLFW_KEY_G, LUMOS_KEY_G},
			{GLFW_KEY_H, LUMOS_KEY_H},
			{GLFW_KEY_I, LUMOS_KEY_I},
			{GLFW_KEY_J, LUMOS_KEY_J},
			{GLFW_KEY_K, LUMOS_KEY_K},
			{GLFW_KEY_L, LUMOS_KEY_L},
			{GLFW_KEY_M, LUMOS_KEY_M},
			{GLFW_KEY_N, LUMOS_KEY_N},
			{GLFW_KEY_O, LUMOS_KEY_O},
			{GLFW_KEY_P, LUMOS_KEY_P},
			{GLFW_KEY_Q, LUMOS_KEY_Q},
			{GLFW_KEY_R, LUMOS_KEY_R},
			{GLFW_KEY_S, LUMOS_KEY_S},
			{GLFW_KEY_T, LUMOS_KEY_T},
			{GLFW_KEY_U, LUMOS_KEY_U},
			{GLFW_KEY_V, LUMOS_KEY_V},
			{GLFW_KEY_W, LUMOS_KEY_W},
			{GLFW_KEY_X, LUMOS_KEY_X},
			{GLFW_KEY_Y, LUMOS_KEY_Y},
			{GLFW_KEY_Z, LUMOS_KEY_Z},

			{GLFW_KEY_0, LUMOS_KEY_0},
			{GLFW_KEY_1, LUMOS_KEY_1},
			{GLFW_KEY_2, LUMOS_KEY_2},
			{GLFW_KEY_3, LUMOS_KEY_3},
			{GLFW_KEY_4, LUMOS_KEY_4},
			{GLFW_KEY_5, LUMOS_KEY_5},
			{GLFW_KEY_6, LUMOS_KEY_6},
			{GLFW_KEY_7, LUMOS_KEY_7},
			{GLFW_KEY_8, LUMOS_KEY_8},
			{GLFW_KEY_9, LUMOS_KEY_9},

			{GLFW_KEY_KP_SUBTRACT	, LUMOS_KEY_SUBTRACT	},
			{GLFW_KEY_DELETE		, LUMOS_KEY_DELETE		},
			{GLFW_KEY_SPACE			, LUMOS_KEY_SPACE		},
			{GLFW_KEY_LEFT			, LUMOS_KEY_LEFT		},
			{GLFW_KEY_RIGHT			, LUMOS_KEY_RIGHT		},
			{GLFW_KEY_UP			, LUMOS_KEY_UP			},
			{GLFW_KEY_DOWN			, LUMOS_KEY_DOWN		},
			{GLFW_KEY_LEFT_SHIFT	, LUMOS_KEY_LEFT_SHIFT	},
			{GLFW_KEY_ESCAPE		, LUMOS_KEY_ESCAPE		},
			{GLFW_KEY_KP_ADD		, LUMOS_KEY_ADD			},
			{GLFW_KEY_COMMA			, LUMOS_KEY_COMMA		},
			{GLFW_KEY_BACKSPACE		, LUMOS_KEY_BACKSPACE	},
			{GLFW_KEY_ENTER			, LUMOS_KEY_ENTER		}
			};

			return keyMap[glfwKey];
		}
	

		static uint GLFWToLumosMouseKey(uint glfwKey)
		{

			static std::map<uint, uint> keyMap =
			{
			{GLFW_MOUSE_BUTTON_LEFT, LUMOS_MOUSE_LEFT},
			{GLFW_MOUSE_BUTTON_RIGHT, LUMOS_MOUSE_RIGHT},
			{GLFW_MOUSE_BUTTON_MIDDLE, LUMOS_MOUSE_MIDDLE}
			};

			return keyMap[glfwKey];
		}
	}
}