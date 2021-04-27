#include "Precompiled.h"
#include "Input.h"

namespace Lumos
{
    Input::Input()
        : m_WindowFocus(true)
        , m_MouseMode(MouseMode::Visible)
    {
        Reset();
    }

    void Input::Reset()
    {
        memset(m_KeyHeld, 0, MAX_KEYS);
        memset(m_KeyPressed, 0, MAX_KEYS);
        memset(m_MouseClicked, 0, MAX_BUTTONS);
        memset(m_MouseHeld, 0, MAX_BUTTONS);

        m_MouseOnScreen = true;
        m_WindowFocus = true;
        m_ScrollOffset = 0.0f;
    }

    void Input::ResetPressed()
    {
        memset(m_KeyPressed, 0, MAX_KEYS);
        memset(m_MouseClicked, 0, MAX_BUTTONS);
        m_ScrollOffset = 0;
    }

    void Input::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT_FN(Input::OnKeyPressed));
        dispatcher.Dispatch<KeyReleasedEvent>(BIND_EVENT_FN(Input::OnKeyReleased));
        dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(Input::OnMousePressed));
        dispatcher.Dispatch<MouseButtonReleasedEvent>(BIND_EVENT_FN(Input::OnMouseReleased));
        dispatcher.Dispatch<MouseScrolledEvent>(BIND_EVENT_FN(Input::OnMouseScrolled));
        dispatcher.Dispatch<MouseMovedEvent>(BIND_EVENT_FN(Input::OnMouseMoved));
        dispatcher.Dispatch<MouseEnterEvent>(BIND_EVENT_FN(Input::OnMouseEnter));
    }

    bool Input::OnKeyPressed(KeyPressedEvent& e)
    {
        SetKeyPressed(Lumos::InputCode::Key(e.GetKeyCode()), e.GetRepeatCount() < 1);
        SetKeyHeld(Lumos::InputCode::Key(e.GetKeyCode()), true);
        return false;
    }

    bool Input::OnKeyReleased(KeyReleasedEvent& e)
    {
        SetKeyPressed(Lumos::InputCode::Key(e.GetKeyCode()), false);
        SetKeyHeld(Lumos::InputCode::Key(e.GetKeyCode()), false);
        return false;
    }

    bool Input::OnMousePressed(MouseButtonPressedEvent& e)
    {
        SetMouseClicked(Lumos::InputCode::MouseKey(e.GetMouseButton()), true);
        SetMouseHeld(Lumos::InputCode::MouseKey(e.GetMouseButton()), true);
        return false;
    }

    bool Input::OnMouseReleased(MouseButtonReleasedEvent& e)
    {
        SetMouseClicked(Lumos::InputCode::MouseKey(e.GetMouseButton()), false);
        SetMouseHeld(Lumos::InputCode::MouseKey(e.GetMouseButton()), false);
        return false;
    }

    bool Input::OnMouseScrolled(MouseScrolledEvent& e)
    {
        SetScrollOffset(e.GetYOffset());
        return false;
    }

    bool Input::OnMouseMoved(MouseMovedEvent& e)
    {
        StoreMousePosition(e.GetX(), e.GetY());
        return false;
    }

    bool Input::OnMouseEnter(MouseEnterEvent& e)
    {
        SetMouseOnScreen(e.GetEntered());
        return false;
    }
}
