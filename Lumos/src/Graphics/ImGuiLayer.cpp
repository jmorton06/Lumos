#include "JM.h"
#include "ImGuiLayer.h"
#include "App/Application.h"

#include "external/imgui/imgui.h"
#include "App/Input.h"

namespace jm
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

		io.KeyCtrl = io.KeysDown[JM_KEY_LEFT_CONTROL] || io.KeysDown[JM_KEY_RIGHT_CONTROL];
		io.KeyShift = io.KeysDown[JM_KEY_LEFT_SHIFT] || io.KeysDown[JM_KEY_RIGHT_SHIFT];
		io.KeyAlt = io.KeysDown[JM_KEY_ALT] || io.KeysDown[JM_KEY_ALT];

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

		return false;
	}

	void ImGuiLayer::SetImGuiKeyCodes()
	{
		ImGuiIO& io = ImGui::GetIO();
		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
		io.KeyMap[ImGuiKey_Tab] 		= JM_KEY_TAB;
		io.KeyMap[ImGuiKey_LeftArrow] 	= JM_KEY_LEFT;
		io.KeyMap[ImGuiKey_RightArrow] 	= JM_KEY_RIGHT;
		io.KeyMap[ImGuiKey_UpArrow] 	= JM_KEY_UP;
		io.KeyMap[ImGuiKey_DownArrow] 	= JM_KEY_DOWN;
		io.KeyMap[ImGuiKey_PageUp] 		= JM_KEY_PAGE_UP;
		io.KeyMap[ImGuiKey_PageDown] 	= JM_KEY_PAGE_DOWN;
		io.KeyMap[ImGuiKey_Home] 		= JM_KEY_HOME;
		io.KeyMap[ImGuiKey_End] 		= JM_KEY_END;
		io.KeyMap[ImGuiKey_Insert] 		= JM_KEY_INSERT;
		io.KeyMap[ImGuiKey_Delete] 		= JM_KEY_DELETE;
		io.KeyMap[ImGuiKey_Backspace] 	= JM_KEY_BACKSPACE;
		io.KeyMap[ImGuiKey_Space] 		= JM_KEY_SPACE;
		io.KeyMap[ImGuiKey_Enter] 		= JM_KEY_ENTER;
		io.KeyMap[ImGuiKey_Escape] 		= JM_KEY_ESCAPE;
		io.KeyMap[ImGuiKey_A] = JM_KEY_A;
		io.KeyMap[ImGuiKey_C] = JM_KEY_C;
		io.KeyMap[ImGuiKey_V] = JM_KEY_V;
		io.KeyMap[ImGuiKey_X] = JM_KEY_X;
		io.KeyMap[ImGuiKey_Y] = JM_KEY_Y;
		io.KeyMap[ImGuiKey_Z] = JM_KEY_Z;
	}
}
