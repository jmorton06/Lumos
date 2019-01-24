#include "LM.h"
#include "ImGuiLayer.h"
#include "App/Input.h"
#include "App/Application.h"
#include "Graphics/API/IMGUIRenderer.h"

#include <imgui/imgui.h>


namespace Lumos
{

	ImGuiLayer::ImGuiLayer(const std::string& debugName)
		: Layer(debugName)
	{
	}

	ImGuiLayer::~ImGuiLayer()
	{

	}

	void ImGuiLayer::OnAttach()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		Application* app = Application::Instance();
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)app->GetWindow()->GetWidth(), (float)app->GetWindow()->GetHeight());

		SetImGuiKeyCodes();

		m_IMGUIRenderer = std::unique_ptr<graphics::api::IMGUIRenderer>(graphics::api::IMGUIRenderer::Create(app->GetWindow()->GetWidth(),app->GetWindow()->GetHeight()));

        if(m_IMGUIRenderer)
            m_IMGUIRenderer->Init();
	}

	void ImGuiLayer::OnDetach()
	{
		ImGui::DestroyContext();
	}

	void ImGuiLayer::OnUpdate(TimeStep* dt)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.DeltaTime = dt->GetMillis();

		ImGui::NewFrame();
		Application::Instance()->OnImGui();
		ImGui::Render();
	}

	void ImGuiLayer::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
 		dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(ImGuiLayer::OnMouseButtonPressedEvent));
 		dispatcher.Dispatch<MouseButtonReleasedEvent>(BIND_EVENT_FN(ImGuiLayer::OnMouseButtonReleasedEvent));
 		dispatcher.Dispatch<MouseMovedEvent>(BIND_EVENT_FN(ImGuiLayer::OnMouseMovedEvent));
 		dispatcher.Dispatch<MouseScrolledEvent>(BIND_EVENT_FN(ImGuiLayer::OnMouseScrolledEvent));
 		dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT_FN(ImGuiLayer::OnKeyPressedEvent));
 		dispatcher.Dispatch<KeyReleasedEvent>(BIND_EVENT_FN(ImGuiLayer::OnKeyReleasedEvent));
 		dispatcher.Dispatch<KeyTypedEvent>(BIND_EVENT_FN(ImGuiLayer::OnKeyTypedEvent));
 		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(ImGuiLayer::OnwindowResizeEvent));
	}

	void ImGuiLayer::OnRender(Scene* scene)
	{
		//TODO: Render Using api
		if(m_IMGUIRenderer && m_IMGUIRenderer->Implemented())
		{
			m_IMGUIRenderer->Render(nullptr);
		}
	}

	bool ImGuiLayer::OnMouseButtonPressedEvent(MouseButtonPressedEvent & e)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.MouseDown[e.GetMouseButton()] = true;

		return false;
	}

	bool ImGuiLayer::OnMouseButtonReleasedEvent(MouseButtonReleasedEvent & e)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.MouseDown[e.GetMouseButton()] = false;

		return false;
	}

	bool ImGuiLayer::OnMouseMovedEvent(MouseMovedEvent & e)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.MousePos = ImVec2(e.GetX(), e.GetY());

		return false;
	}

	bool ImGuiLayer::OnMouseScrolledEvent(MouseScrolledEvent & e)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.MouseWheel += e.GetYOffset();
		io.MouseWheelH += e.GetXOffset();

		return false;
	}

	bool ImGuiLayer::OnKeyPressedEvent(KeyPressedEvent & e)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.KeysDown[e.GetKeyCode()] = true;

		io.KeyCtrl = io.KeysDown[LUMOS_KEY_LEFT_CONTROL] || io.KeysDown[LUMOS_KEY_RIGHT_CONTROL];
		io.KeyShift = io.KeysDown[LUMOS_KEY_LEFT_SHIFT] || io.KeysDown[LUMOS_KEY_RIGHT_SHIFT];
		io.KeyAlt = io.KeysDown[LUMOS_KEY_ALT] || io.KeysDown[LUMOS_KEY_ALT];

		return false;
	}

	bool ImGuiLayer::OnKeyReleasedEvent(KeyReleasedEvent & e)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.KeysDown[e.GetKeyCode()] = false;

		return false;
	}

	bool ImGuiLayer::OnKeyTypedEvent(KeyTypedEvent & e)
	{
		ImGuiIO& io = ImGui::GetIO();
		int keycode = e.GetKeyCode();
		if (keycode > 0 && keycode < 0x10000)
			io.AddInputCharacter((unsigned short)keycode);

		return false;
	}

	bool ImGuiLayer::OnwindowResizeEvent(WindowResizeEvent & e)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)e.GetWidth(), (float)e.GetHeight());
		io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

		m_IMGUIRenderer->OnResize(e.GetWidth(), e.GetHeight());

		return false;
	}

	void ImGuiLayer::SetImGuiKeyCodes()
	{
		ImGuiIO& io = ImGui::GetIO();
		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
		io.KeyMap[ImGuiKey_Tab] 		= LUMOS_KEY_TAB;
		io.KeyMap[ImGuiKey_LeftArrow] 	= LUMOS_KEY_LEFT;
		io.KeyMap[ImGuiKey_RightArrow] 	= LUMOS_KEY_RIGHT;
		io.KeyMap[ImGuiKey_UpArrow] 	= LUMOS_KEY_UP;
		io.KeyMap[ImGuiKey_DownArrow] 	= LUMOS_KEY_DOWN;
		io.KeyMap[ImGuiKey_PageUp] 		= LUMOS_KEY_PAGE_UP;
		io.KeyMap[ImGuiKey_PageDown] 	= LUMOS_KEY_PAGE_DOWN;
		io.KeyMap[ImGuiKey_Home] 		= LUMOS_KEY_HOME;
		io.KeyMap[ImGuiKey_End] 		= LUMOS_KEY_END;
		io.KeyMap[ImGuiKey_Insert] 		= LUMOS_KEY_INSERT;
		io.KeyMap[ImGuiKey_Delete] 		= LUMOS_KEY_DELETE;
		io.KeyMap[ImGuiKey_Backspace] 	= LUMOS_KEY_BACKSPACE;
		io.KeyMap[ImGuiKey_Space] 		= LUMOS_KEY_SPACE;
		io.KeyMap[ImGuiKey_Enter] 		= LUMOS_KEY_ENTER;
		io.KeyMap[ImGuiKey_Escape] 		= LUMOS_KEY_ESCAPE;
		io.KeyMap[ImGuiKey_A] = LUMOS_KEY_A;
		io.KeyMap[ImGuiKey_C] = LUMOS_KEY_C;
		io.KeyMap[ImGuiKey_V] = LUMOS_KEY_V;
		io.KeyMap[ImGuiKey_X] = LUMOS_KEY_X;
		io.KeyMap[ImGuiKey_Y] = LUMOS_KEY_Y;
		io.KeyMap[ImGuiKey_Z] = LUMOS_KEY_Z;
	}
}
