#include "LM.h"
#include "Win32Input.h"
#include "Win32Window.h"

#include "App/Application.h"

namespace Lumos
{

	Win32Input::Win32Input()
	{
		
	}


	Win32Input::~Win32Input()
	{

	}

	void Win32Input::Update(void* raw)
	{
		RAWINPUT* rw = (RAWINPUT*)raw;
		DWORD key = (DWORD)rw->data.keyboard.VKey;

		// We should do bounds checking!
		if (key < 0 || key > KEYBOARD_MAX)
		{
			return;
		}

		m_KeyPressed[Win32KeyToLumos(key)] = !(rw->data.keyboard.Flags & RI_KEY_BREAK);
	}

	uint Win32Input::Win32KeyToLumos(uint key)
	{
		switch (key)
		{
		case KEYBOARD_A: return (LUMOS_KEY_A); break;
		case KEYBOARD_B: return(LUMOS_KEY_B); break;
		case KEYBOARD_C: return(LUMOS_KEY_C); break;
		case KEYBOARD_D: return(LUMOS_KEY_D); break;
		case KEYBOARD_E: return(LUMOS_KEY_E); break;
		case KEYBOARD_F: return(LUMOS_KEY_F); break;
		case KEYBOARD_G: return(LUMOS_KEY_G); break;
		case KEYBOARD_H: return(LUMOS_KEY_H); break;
		case KEYBOARD_I: return(LUMOS_KEY_I); break;
		case KEYBOARD_J: return(LUMOS_KEY_J); break;
		case KEYBOARD_K: return(LUMOS_KEY_K); break;
		case KEYBOARD_L: return(LUMOS_KEY_L); break;
		case KEYBOARD_M: return(LUMOS_KEY_M); break;
		case KEYBOARD_N: return(LUMOS_KEY_N); break;
		case KEYBOARD_O: return(LUMOS_KEY_O); break;
		case KEYBOARD_P: return(LUMOS_KEY_P); break;
		case KEYBOARD_Q: return(LUMOS_KEY_Q); break;
		case KEYBOARD_R: return(LUMOS_KEY_R); break;
		case KEYBOARD_S: return(LUMOS_KEY_S); break;
		case KEYBOARD_T: return(LUMOS_KEY_T); break;
		case KEYBOARD_U: return(LUMOS_KEY_U); break;
		case KEYBOARD_V: return(LUMOS_KEY_V); break;
		case KEYBOARD_W: return(LUMOS_KEY_W); break;
		case KEYBOARD_X: return(LUMOS_KEY_X); break;
		case KEYBOARD_Y: return(LUMOS_KEY_Y); break;
		case KEYBOARD_Z: return(LUMOS_KEY_Z); break;

		case KEYBOARD_0: return(LUMOS_KEY_0); break;
		case KEYBOARD_1: return(LUMOS_KEY_1); break;
		case KEYBOARD_2: return(LUMOS_KEY_2); break;
		case KEYBOARD_3: return(LUMOS_KEY_3); break;
		case KEYBOARD_4: return(LUMOS_KEY_4); break;
		case KEYBOARD_5: return(LUMOS_KEY_5); break;
		case KEYBOARD_6: return(LUMOS_KEY_6); break;
		case KEYBOARD_7: return(LUMOS_KEY_7); break;
		case KEYBOARD_8: return(LUMOS_KEY_8); break;
		case KEYBOARD_9: return(LUMOS_KEY_9); break;

		case KEYBOARD_SUBTRACT: return(LUMOS_KEY_SUBTRACT); break;
		case KEYBOARD_DELETE: return(LUMOS_KEY_DELETE); break;
		case KEYBOARD_SPACE: return(LUMOS_KEY_SPACE); break;
		case KEYBOARD_LEFT: return(LUMOS_KEY_LEFT); break;
		case KEYBOARD_RIGHT: return(LUMOS_KEY_RIGHT); break;
		case KEYBOARD_UP: return(LUMOS_KEY_UP); break;
		case KEYBOARD_DOWN: return(LUMOS_KEY_DOWN); break;
		case KEYBOARD_LSHIFT: return(LUMOS_KEY_LEFT_SHIFT); break;
		case KEYBOARD_ESCAPE: return(LUMOS_KEY_ESCAPE); break;

		case KEYBOARD_ADD: return(LUMOS_KEY_ADD); break;
		case KEYBOARD_COMMA: return(LUMOS_KEY_COMMA); break;

		default: return 0; break;

		}
	}
}