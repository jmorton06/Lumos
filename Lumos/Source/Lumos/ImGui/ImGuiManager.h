#pragma once

#include "Core/Core.h"
#include "Events/KeyEvent.h"
#include "Events/Event.h"
#include "Events/MouseEvent.h"
#include "Events/ApplicationEvent.h"

namespace Lumos
{
    class Scene;
    class TimeStep;
    namespace Graphics
    {
        class IMGUIRenderer;
    }

    class LUMOS_EXPORT ImGuiManager
    {
    public:
        ImGuiManager(bool clearScreen = false);
        ~ImGuiManager();

        void OnInit();
        void OnUpdate(const TimeStep& dt, Scene* scene);
        void OnEvent(Event& event);
        void OnRender(Scene* scene);
        void OnNewScene(Scene* scene);

    private:
        bool OnMouseButtonPressedEvent(MouseButtonPressedEvent& e);
        bool OnMouseButtonReleasedEvent(MouseButtonReleasedEvent& e);
        bool OnMouseMovedEvent(MouseMovedEvent& e);
        bool OnMouseScrolledEvent(MouseScrolledEvent& e);
        bool OnKeyPressedEvent(KeyPressedEvent& e);
        bool OnKeyReleasedEvent(KeyReleasedEvent& e);
        bool OnKeyTypedEvent(KeyTypedEvent& e);
        bool OnwindowResizeEvent(WindowResizeEvent& e);

        void SetImGuiKeyCodes();
        void SetImGuiStyle();
        void AddIconFont();

        float m_FontSize;
        float m_DPIScale;

        UniqueRef<Graphics::IMGUIRenderer> m_IMGUIRenderer;
        bool m_ClearScreen;
    };

}
