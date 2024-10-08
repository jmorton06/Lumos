#include "Precompiled.h"
#include "Input.h"

namespace Lumos
{
    Input::Input()
        : m_MouseMode(MouseMode::Visible)
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
        m_ScrollOffset  = 0.0f;
        m_ScrollOffsetX = 0.0f;
    }

    void Input::ResetPressed()
    {
        memset(m_KeyPressed, 0, MAX_KEYS);
        memset(m_MouseClicked, 0, MAX_BUTTONS);
        m_ScrollOffset  = 0.0f;
        m_ScrollOffsetX = 0.0f;
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
        SetScrollOffsetX(e.GetXOffset());
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

    bool Input::IsControllerPresent(int id)
    {
        return m_Controllers[id].Present;
    }

    TDArray<int> Input::GetConnectedControllerIDs()
    {
        TDArray<int> ids;
        ids.Reserve(MAX_CONTROLLER_COUNT);
        for(int i = 0; i < MAX_CONTROLLER_COUNT; i++)
        {
            if(m_Controllers[i].Present)
            {
                ids.PushBack(i);
            }
        }

        return ids;
    }

    Controller* Input::GetController(int id)
    {
        return &m_Controllers[id];
    }

    Controller* Input::GetOrAddController(int id)
    {
        return &m_Controllers[id];
    }

    std::string Input::GetControllerName(int id)
    {
        if(!Input::IsControllerPresent(id))
            return {};

        Controller& controller = *GetController(id);
        return controller.Name;
    }

    bool Input::IsControllerButtonPressed(int controllerID, int button)
    {
        if(!Input::IsControllerPresent(controllerID))
            return false;

        Controller& controller     = *GetController(controllerID);
        ControllerButtonData& data = controller.ButtonStates[button];
        return data.State == KeyState::Pressed;
    }

    float Input::GetControllerAxis(int controllerID, int axis)
    {
        if(!Input::IsControllerPresent(controllerID))
            return 0.0f;

        Controller* controller = GetController(controllerID);
        return controller->AxisStates[axis];
    }

    uint8_t Input::GetControllerHat(int controllerID, int hat)
    {
        if(!Input::IsControllerPresent(controllerID))
            return 0;

        Controller& controller = *GetController(controllerID);
        uint8_t value          = 0;
        return controller.HatStates[hat];
    }

    void Input::RemoveController(int id)
    {
    }
}
