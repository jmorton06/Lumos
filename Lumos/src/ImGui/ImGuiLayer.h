#pragma once

#include "Core/Core.h"
#include "Graphics/Layers/Layer.h"
#include "Events/KeyEvent.h"
#include "Events/Event.h"
#include "Events/MouseEvent.h"
#include "Events/ApplicationEvent.h"

namespace Lumos
{
    namespace Graphics
    {
		class IMGUIRenderer;
    }

	class LUMOS_EXPORT ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer(bool clearScreen = false, const std::string& name = "ImGuiLayer");
		~ImGuiLayer();

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(TimeStep* dt, Scene* scene) override;
		void OnEvent(Event& event) override;
		void OnRender(Scene* scene) override;
		void OnNewScene(Scene* scene) override;

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

        Scope<Graphics::IMGUIRenderer> m_IMGUIRenderer;
		bool m_ClearScreen;
	};

}
