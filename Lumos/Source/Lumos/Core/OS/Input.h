#pragma once
#include "Events/Event.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "Utilities/TSingleton.h"
#include "Core/OS/KeyCodes.h"
#include <glm/ext/vector_float2.hpp>
#include <map>

#define MAX_KEYS 1024
#define MAX_BUTTONS 32

namespace Lumos
{
    class Event;

    enum class MouseMode
    {
        Visible,
        Hidden,
        Captured
    };

    enum class KeyState
    {
        None = -1,
        Pressed,
        Held,
        Released
    };

    struct ControllerButtonData
    {
        int Button;
        KeyState State    = KeyState::None;
        KeyState OldState = KeyState::None;
    };

    struct Controller
    {
        int ID;
        std::string Name;
        std::map<int, bool> ButtonDown;
        std::map<int, ControllerButtonData> ButtonStates;
        std::map<int, float> AxisStates;
        std::map<int, float> DeadZones;
        std::map<int, uint8_t> HatStates;
    };

    class LUMOS_EXPORT Input : public ThreadSafeSingleton<Input>
    {
        friend class TSingleton<Input>;

    public:
        Input();
        virtual ~Input() = default;

        bool GetKeyPressed(Lumos::InputCode::Key key) const { return m_KeyPressed[int(key)]; }
        bool GetKeyHeld(Lumos::InputCode::Key key) const { return m_KeyHeld[int(key)]; }
        bool GetMouseClicked(Lumos::InputCode::MouseKey key) const { return m_MouseClicked[int(key)]; }
        bool GetMouseHeld(Lumos::InputCode::MouseKey key) const { return m_MouseHeld[int(key)]; }

        void SetKeyPressed(Lumos::InputCode::Key key, bool a) { m_KeyPressed[int(key)] = a; }
        void SetKeyHeld(Lumos::InputCode::Key key, bool a) { m_KeyHeld[int(key)] = a; }
        void SetMouseClicked(Lumos::InputCode::MouseKey key, bool a) { m_MouseClicked[int(key)] = a; }
        void SetMouseHeld(Lumos::InputCode::MouseKey key, bool a) { m_MouseHeld[int(key)] = a; }

        void SetMouseOnScreen(bool onScreen) { m_MouseOnScreen = onScreen; }
        bool GetMouseOnScreen() const { return m_MouseOnScreen; }

        void StoreMousePosition(float xpos, float ypos) { m_MousePosition = glm::vec2(float(xpos), float(ypos)); }
        const glm::vec2& GetMousePosition() const { return m_MousePosition; }

        void SetScrollOffset(float offset) { m_ScrollOffset = offset; }
        float GetScrollOffset() const { return m_ScrollOffset; }

        void SetScrollOffsetX(float offset) { m_ScrollOffsetX = offset; }
        float GetScrollOffsetX() const { return m_ScrollOffsetX; }

        void Reset();
        void ResetPressed();
        void OnEvent(Event& e);

        MouseMode GetMouseMode() const { return m_MouseMode; }
        void SetMouseMode(MouseMode mode) { m_MouseMode = mode; }

        // Controllers
        bool IsControllerPresent(int id);
        std::vector<int> GetConnectedControllerIDs();
        Controller* GetController(int id);
        Controller* GetOrAddController(int id);

        std::string GetControllerName(int id);
        bool IsControllerButtonPressed(int controllerID, int button);
        float GetControllerAxis(int controllerID, int axis);
        uint8_t GetControllerHat(int controllerID, int hat);
        void RemoveController(int id);

        std::map<int, Controller>& GetControllers() { return m_Controllers; }

    private:
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

        float m_ScrollOffset  = 0.0f;
        float m_ScrollOffsetX = 0.0f;

        bool m_MouseOnScreen;
        MouseMode m_MouseMode;

        glm::vec2 m_MousePosition;

        std::map<int, Controller> m_Controllers;
    };
}
