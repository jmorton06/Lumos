#pragma once
#include "JM.h"
#include "Maths/Maths.h"

#include "Events/Event.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"

#define MAX_KEYS	1024
#define MAX_BUTTONS	32

namespace jm
{

	class Event;

	class JM_EXPORT Input
	{
	public:
		static Input& GetInput() { return *s_Input; }
		static Input* s_Input;

		static void Create();
		static void Release() { delete s_Input; }

		Input();
		virtual ~Input() {}

		bool GetKeyPressed(uint key)   const { return m_KeyPressed[key]; }
		bool GetKeyHeld(uint key)      const { return m_KeyHeld[key]; }
		bool GetMouseClicked(uint key) const { return m_MouseClicked[key]; }
		bool GetMouseHeld(uint key)    const { return m_MouseHeld[key]; }

		void SetKeyPressed(uint key, bool a)   { m_KeyPressed[key] = a; }
		void SetKeyHeld(uint key, bool a)	   { m_KeyHeld[key] = a; }
		void SetMouseClicked(uint key, bool a) { m_MouseClicked[key] = a; }
		void SetMouseHeld(uint key, bool a)    { m_MouseHeld[key] = a; }

		void SetMouseOnScreen(bool onScreen){ m_MouseOnScreen = onScreen; }
		bool GetMouseOnScreen() const { return m_MouseOnScreen; }

		void StoreMousePosition(int xpos, int ypos){ m_MousePosition = maths::Vector2(float(xpos), float(ypos)); }
		maths::Vector2 GetMousePosition() const { return m_MousePosition; }

		void SetWindowFocus(bool focus)		{ m_WindowFocus = focus; }
		bool GetWindowFocus() const { return m_WindowFocus; }

		void SetScrollOffset(float offset)	{ m_ScrollOffset = offset; }
		float GetScrollOffset() const { return m_ScrollOffset; }

		void Reset();
		void ResetPressed();
		void OnEvent(Event& e);

	protected:

		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnKeyReleased(KeyReleasedEvent& e);
		bool OnMousePressed(MouseButtonPressedEvent& e);
		bool OnMouseReleased(MouseButtonReleasedEvent& e);
		bool OnMouseScrolled(MouseScrolledEvent& e);
		bool OnMouseMoved(MouseMovedEvent& e);
		bool OnMouseEnter(MouseEnterEvent& e);

		bool m_KeyPressed[MAX_KEYS];
		bool m_KeyHeld[MAX_KEYS];

		bool m_MouseHeld[MAX_BUTTONS];
		bool m_MouseClicked[MAX_BUTTONS];

		bool m_UpdateCamera = false;
		float m_ScrollOffset = 0.0f;

		bool m_MouseOnScreen;
		bool m_WindowFocus;

		maths::Vector2 m_MousePosition;
	};

#define JM_MOUSE_LEFT	      0x00
#define JM_MOUSE_MIDDLE	      0x01
#define JM_MOUSE_RIGHT		  0x02

#define JM_NO_CURSOR		  NULL

#define JM_KEY_TAB			  0x09

#define JM_KEY_0			  0x30
#define JM_KEY_1			  0x31
#define JM_KEY_2			  0x32
#define JM_KEY_3			  0x33
#define JM_KEY_4			  0x34
#define JM_KEY_5			  0x35
#define JM_KEY_6			  0x36
#define JM_KEY_7			  0x37
#define JM_KEY_8			  0x38
#define JM_KEY_9			  0x39

#define JM_KEY_A			  0x41
#define JM_KEY_B			  0x42
#define JM_KEY_C			  0x43
#define JM_KEY_D			  0x44
#define JM_KEY_E			  0x45
#define JM_KEY_F			  0x46
#define JM_KEY_G			  0x47
#define JM_KEY_H			  0x48
#define JM_KEY_I			  0x49
#define JM_KEY_J			  0x4A
#define JM_KEY_K			  0x4B
#define JM_KEY_L			  0x4C
#define JM_KEY_M			  0x4D
#define JM_KEY_N			  0x4E
#define JM_KEY_O			  0x4F
#define JM_KEY_P			  0x50
#define JM_KEY_Q			  0x51
#define JM_KEY_R			  0x52
#define JM_KEY_S			  0x53
#define JM_KEY_T			  0x54
#define JM_KEY_U			  0x55
#define JM_KEY_V			  0x56
#define JM_KEY_W			  0x57
#define JM_KEY_X			  0x58
#define JM_KEY_Y			  0x59
#define JM_KEY_Z			  0x5A

#define JM_KEY_LBUTTON        0x01
#define JM_KEY_RBUTTON        0x02
#define JM_KEY_CANCEL         0x03
#define JM_KEY_MBUTTON        0x04

#define JM_KEY_ESCAPE         0x1B
#define JM_KEY_SHIFT          0x10
#define JM_KEY_CONTROL        0x11
#define JM_KEY_MENU           0x12
#define JM_KEY_ALT	          JM_KEY_MENU
#define JM_KEY_PAUSE          0x13
#define JM_KEY_CAPITAL        0x14

#define JM_KEY_SPACE          0x20
#define JM_KEY_PRIOR          0x21
#define JM_KEY_NEXT           0x22
#define JM_KEY_END            0x23
#define JM_KEY_HOME           0x24
#define JM_KEY_LEFT           0x25
#define JM_KEY_UP             0x26
#define JM_KEY_RIGHT          0x27
#define JM_KEY_DOWN           0x28
#define JM_KEY_SELECT         0x29
#define JM_KEY_PRINT          0x2A
#define JM_KEY_EXECUTE        0x2B
#define JM_KEY_COMMA	      0x2C
#define JM_KEY_INSERT         0x2D
#define JM_KEY_DELETE         0x2E
#define JM_KEY_HELP           0x2F

#define JM_KEY_NUMPAD0        0x60
#define JM_KEY_NUMPAD1        0x61
#define JM_KEY_NUMPAD2        0x62
#define JM_KEY_NUMPAD3        0x63
#define JM_KEY_NUMPAD4        0x64
#define JM_KEY_NUMPAD5        0x65
#define JM_KEY_NUMPAD6        0x66
#define JM_KEY_NUMPAD7        0x67
#define JM_KEY_NUMPAD8        0x68
#define JM_KEY_NUMPAD9        0x69
#define JM_KEY_MULTIPLY       0x6A
#define JM_KEY_ADD            0x6B
#define JM_KEY_SEPARATOR      0x6C
#define JM_KEY_SUBTRACT       0x6D
#define JM_KEY_DECIMAL        0x6E
#define JM_KEY_DIVIDE         0x6F
#define JM_KEY_F1             0x70
#define JM_KEY_F2             0x71
#define JM_KEY_F3             0x72
#define JM_KEY_F4             0x73
#define JM_KEY_F5             0x74
#define JM_KEY_F6             0x75
#define JM_KEY_F7             0x76
#define JM_KEY_F8             0x77
#define JM_KEY_F9             0x78
#define JM_KEY_F10            0x79
#define JM_KEY_F11            0x7A
#define JM_KEY_F12            0x7B
#define JM_KEY_F13            0x7C
#define JM_KEY_F14            0x7D
#define JM_KEY_F15            0x7E
#define JM_KEY_F16            0x7F
#define JM_KEY_F17            0x80
#define JM_KEY_F18            0x81
#define JM_KEY_F19            0x82
#define JM_KEY_F20            0x83
#define JM_KEY_F21            0x84
#define JM_KEY_F22            0x85
#define JM_KEY_F23            0x86
#define JM_KEY_F24            0x87
#define JM_KEY_ENTER		  0x88
#define JM_KEY_BACKSPACE	  0x89
#define JM_KEY_NUMLOCK        0x90
#define JM_KEY_SCROLL         0x91
#define JM_KEY_PAGE_DOWN	  0x92
#define JM_KEY_PAGE_UP		  0x93

#define JM_KEY_LEFT_SHIFT     0xA0
#define JM_KEY_RIGHT_SHIFT    0xA1
#define JM_KEY_LEFT_CONTROL   0xA2
#define JM_KEY_RIGHT_CONTROL  0xA3
#define JM_KEY_LEFT_MENU      0xA4
#define JM_KEY_RIGHT_MENU     0xA5
}
