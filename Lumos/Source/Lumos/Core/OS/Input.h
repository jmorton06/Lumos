#pragma once
#include "Events/Event.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "Events/GestureEvent.h"
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

        // Standardised gamepad view of the same device (GLFW mapped state).
        // Raw joystick indices differ per pad; these do not, which is what
        // lets one set of bindings cover a DualSense and a Steam Deck.
        bool IsGamepad = false;
        std::string GamepadName;
        TArray<bool, InputCode::GamepadButtonCount> GamepadDown;
        TArray<ControllerButtonData, InputCode::GamepadButtonCount> GamepadStates;
        TArray<float, InputCode::GamepadAxisCount> GamepadAxes;
    };

    class LUMOS_EXPORT Input : public ThreadSafeSingleton<Input>
    {
        friend class TSingleton<Input>;
        friend class GLFWWindow;

    public:
        Input();
        virtual ~Input() = default;

        bool GetKeyPressed(Lumos::InputCode::Key key) const { return !m_KeyboardBlocked && m_KeyPressed[int(key)]; }
        bool GetKeyHeld(Lumos::InputCode::Key key) const { return !m_KeyboardBlocked && m_KeyHeld[int(key)]; }

        // Unfiltered variants - ignore the keyboard block. For the overlay that
        // owns the keyboard and for UI modifier lookups.
        bool GetKeyPressedRaw(Lumos::InputCode::Key key) const { return m_KeyPressed[int(key)]; }
        bool GetKeyHeldRaw(Lumos::InputCode::Key key) const { return m_KeyHeld[int(key)]; }

        // While blocked, polled key queries report nothing so a debug overlay can
        // take the keyboard without every game opting in. Text input is event
        // driven and unaffected.
        void SetKeyboardBlocked(bool blocked) { m_KeyboardBlocked = blocked; }
        bool GetKeyboardBlocked() const { return m_KeyboardBlocked; }
        bool GetMouseClicked(Lumos::InputCode::MouseKey key) const { return m_MouseClicked[int(key)]; }
        bool GetMouseHeld(Lumos::InputCode::MouseKey key) const { return m_MouseHeld[int(key)]; }

        void SetKeyPressed(Lumos::InputCode::Key key, bool a) { m_KeyPressed[int(key)] = a; }
        void SetKeyHeld(Lumos::InputCode::Key key, bool a) { m_KeyHeld[int(key)] = a; }
        void SetMouseClicked(Lumos::InputCode::MouseKey key, bool a) { m_MouseClicked[int(key)] = a; }
        void SetMouseHeld(Lumos::InputCode::MouseKey key, bool a) { m_MouseHeld[int(key)] = a; }

        void SetMouseOnScreen(bool onScreen) { m_MouseOnScreen = onScreen; }
        bool GetMouseOnScreen() const { return m_MouseOnScreen; }

        void StoreMousePosition(float xpos, float ypos)
        {
            const Vec2 position(xpos, ypos);
            // the first report has nothing to be relative to
            if(m_HasMousePosition)
                m_MouseDelta += position - m_MousePosition;
            m_HasMousePosition = true;
            m_MousePosition    = position;
        }
        // How far the mouse moved this frame, in the same pixels as GetMousePosition.
        const Vec2& GetMouseDelta() const { return m_MouseDelta; }
        // Forget the last position, so the next report is not counted as movement
        // (after the cursor is captured / released, which can teleport it).
        void ResetMouseDelta()
        {
            m_MouseDelta       = Vec2(0.0f, 0.0f);
            m_HasMousePosition = false;
        }
        const Vec2& GetMousePosition() const { return m_MousePosition; }

        void SetScrollOffset(float offset) { m_ScrollOffset = offset; }
        float GetScrollOffset() const { return m_ScrollOffset; }

        void SetScrollOffsetX(float offset) { m_ScrollOffsetX = offset; }
        float GetScrollOffsetX() const { return m_ScrollOffsetX; }

        // Clipboard
        void SetClipboard(const char* text);
        std::string GetClipboard();

        // Gesture state
        bool GetGesturePinchActive() const { return m_GesturePinchActive; }
        float GetGesturePinchScale() const { return m_GesturePinchScale; }
        float GetGesturePinchVelocity() const { return m_GesturePinchVelocity; }

        bool GetGesturePanActive() const { return m_GesturePanActive; }
        const Vec2& GetGesturePanTranslation() const { return m_GesturePanTranslation; }
        const Vec2& GetGesturePanVelocity() const { return m_GesturePanVelocity; }
        uint32_t GetGesturePanTouchCount() const { return m_GesturePanTouchCount; }

        bool GetGestureLongPressActive() const { return m_GestureLongPressActive; }
        const Vec2& GetGestureLongPressLocation() const { return m_GestureLongPressLocation; }

        uint32_t GetTouchCount() const { return m_TouchCount; }
        void SetTouchCount(uint32_t count) { m_TouchCount = count; }

        // Every finger currently down, in the same pixel space as GetMousePosition.
        // The mouse path only follows the first finger; on-screen controls need all of them.
        static constexpr uint32_t MaxTouchPoints = 10;
        struct TouchPoint
        {
            uint64_t Id;
            Vec2 Position;
        };
        uint32_t GetTouchPointCount() const { return m_TouchPointCount; }
        const TouchPoint& GetTouchPoint(uint32_t i) const { return m_TouchPoints[i]; }
        void SetTouchPoints(const TouchPoint* points, uint32_t count)
        {
            m_TouchPointCount = count < MaxTouchPoints ? count : MaxTouchPoints;
            for(uint32_t i = 0; i < m_TouchPointCount; i++)
                m_TouchPoints[i] = points[i];
        }

        void Reset();
        void ResetPressed();
        void ResetGestures();
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

        // Gamepads (standardised layout). id -1 = the first connected gamepad,
        // which is what a single-player game wants every time.
        int FirstGamepadID() const;
        bool IsGamepadPresent(int id = -1) const;
        std::string GetGamepadName(int id = -1) const;
        float GetGamepadAxis(InputCode::GamepadAxis axis, int id = -1) const;
        bool GetGamepadButtonHeld(InputCode::GamepadButton button, int id = -1) const;
        bool GetGamepadButtonPressed(InputCode::GamepadButton button, int id = -1) const;
        // Extra SDL_GameControllerDB lines for pads GLFW's built-in table
        // doesn't know. Returns false if GLFW rejected the string.
        bool AddGamepadMappings(const char* mappings);
        float GetGamepadDeadZone() const { return m_GamepadDeadZone; }
        void SetGamepadDeadZone(float dz) { m_GamepadDeadZone = dz; }

    private:
    protected:
        bool OnKeyPressed(KeyPressedEvent& e);
        bool OnKeyReleased(KeyReleasedEvent& e);
        bool OnMousePressed(MouseButtonPressedEvent& e);
        bool OnMouseReleased(MouseButtonReleasedEvent& e);
        bool OnMouseScrolled(MouseScrolledEvent& e);
        bool OnMouseMoved(MouseMovedEvent& e);
        bool OnMouseEnter(MouseEnterEvent& e);

        bool OnGesturePinch(GesturePinchEvent& e);
        bool OnGesturePan(GesturePanEvent& e);
        bool OnGestureSwipe(GestureSwipeEvent& e);
        bool OnGestureLongPress(GestureLongPressEvent& e);

        bool m_KeyPressed[MAX_KEYS];
        bool m_KeyHeld[MAX_KEYS];

        bool m_MouseHeld[MAX_BUTTONS];
        bool m_MouseClicked[MAX_BUTTONS];

        float m_ScrollOffset  = 0.0f;
        float m_ScrollOffsetX = 0.0f;

        bool m_MouseOnScreen;
        bool m_KeyboardBlocked = false;
        MouseMode m_MouseMode;

        Vec2 m_MousePosition;
        Vec2 m_MouseDelta       = Vec2(0.0f, 0.0f);
        bool m_HasMousePosition = false;

        // Gesture state
        bool m_GesturePinchActive = false;
        float m_GesturePinchScale = 1.0f;
        float m_GesturePinchVelocity = 0.0f;

        bool m_GesturePanActive = false;
        Vec2 m_GesturePanTranslation;
        Vec2 m_GesturePanVelocity;
        uint32_t m_GesturePanTouchCount = 0;

        bool m_GestureLongPressActive = false;
        Vec2 m_GestureLongPressLocation;
        uint32_t m_TouchCount = 0;
        TouchPoint m_TouchPoints[MaxTouchPoints] = {};
        uint32_t m_TouchPointCount = 0;

        Controller m_Controllers[MAX_CONTROLLER_COUNT];
        float m_GamepadDeadZone = 0.18f; // sticks drift; applied radially per stick
    };
}
