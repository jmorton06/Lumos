#pragma once
#include "Events/Event.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "Utilities/TSingleton.h"
#include "Core/OS/KeyCodes.h"
#include "Core/DataStructures/TArray.h"
#include "Core/DataStructures/Map.h"
#include "Core/DataStructures/TDArray.h"
#include "Maths/Vector2.h"

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

#define MAX_CONTROLLER_COUNT 16
    struct Controller
    {
        int ID;
        std::string Name;
        bool Present = false;
        TArray<bool, 64> ButtonDown;
        TArray<ControllerButtonData, 64> ButtonStates;
        TArray<float, 16> AxisStates;
        TArray<float, 16> DeadZones;
        TArray<uint8_t, 16> HatStates;
    };

    class LUMOS_EXPORT Input : public ThreadSafeSingleton<Input>
    {
        friend class TSingleton<Input>;
        friend class GLFWWindow;

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

        void StoreMousePosition(float xpos, float ypos) { m_MousePosition = Vec2(float(xpos), float(ypos)); }
        const Vec2& GetMousePosition() const { return m_MousePosition; }

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
        TDArray<int> GetConnectedControllerIDs();
        Controller* GetController(int id);
        Controller* GetOrAddController(int id);

        std::string GetControllerName(int id);
        bool IsControllerButtonPressed(int controllerID, int button);
        float GetControllerAxis(int controllerID, int axis);
        uint8_t GetControllerHat(int controllerID, int hat);
        void RemoveController(int id);

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

        Vec2 m_MousePosition;

        Controller m_Controllers[MAX_CONTROLLER_COUNT];
    };
}
