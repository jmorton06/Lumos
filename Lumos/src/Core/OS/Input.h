#pragma once
#include "lmpch.h"
#include "Maths/Maths.h"

#include "Events/Event.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"

#include "Core/OS/KeyCodes.h"

#define MAX_KEYS	1024
#define MAX_BUTTONS	32

namespace Lumos
{
	class Event;

	class LUMOS_EXPORT Input
	{
	public:
		static Input* GetInput() { return s_Input; }

		static void Create();
		static void Release() { lmdel s_Input; }

		bool GetKeyPressed(u32 key)   const { return m_KeyPressed[key]; }
		bool GetKeyHeld(u32 key)      const { return m_KeyHeld[key]; }
		bool GetMouseClicked(u32 key) const { return m_MouseClicked[key]; }
		bool GetMouseHeld(u32 key)    const { return m_MouseHeld[key]; }

		void SetKeyPressed(u32 key, bool a)   { m_KeyPressed[key] = a; }
		void SetKeyHeld(u32 key, bool a)	   { m_KeyHeld[key] = a; }
		void SetMouseClicked(u32 key, bool a) { m_MouseClicked[key] = a; }
		void SetMouseHeld(u32 key, bool a)    { m_MouseHeld[key] = a; }

		void SetMouseOnScreen(bool onScreen){ m_MouseOnScreen = onScreen; }
		bool GetMouseOnScreen() const { return m_MouseOnScreen; }

		void StoreMousePosition(int xpos, int ypos) { m_MousePosition = Maths::Vector2(float(xpos), float(ypos)); }
		const Maths::Vector2& GetMousePosition() const { return m_MousePosition; }

		void SetWindowFocus(bool focus)		{ m_WindowFocus = focus; }
		bool GetWindowFocus() const { return m_WindowFocus; }

		void SetScrollOffset(float offset)	{ m_ScrollOffset = offset; }
		float GetScrollOffset() const { return m_ScrollOffset; }

		void Reset();
		void ResetPressed();
		void OnEvent(Event& e);

	protected:
		Input();
		virtual ~Input() = default;

		static Input* s_Input;

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

		Maths::Vector2 m_MousePosition;
	};
}
