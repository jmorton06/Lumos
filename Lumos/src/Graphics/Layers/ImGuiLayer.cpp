#include "LM.h"
#include "ImGuiLayer.h"
#include "App/Input.h"
#include "App/Application.h"
#include "Graphics/API/IMGUIRenderer.h"
#include "System/VFS.h"

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include <imgui/imgui.h>
#include <imgui/plugins/ImGuizmo.h>

namespace Lumos
{
	ImGuiLayer::ImGuiLayer(bool clearScreen, const std::string& debugName)
		: Layer(debugName)
	{
		m_ClearScreen = clearScreen;
	}

	ImGuiLayer::~ImGuiLayer()
	{

	}

	void ImGuiLayer::OnAttach()
	{
		Application* app = Application::Instance();
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(static_cast<float>(app->GetWindow()->GetWidth()), static_cast<float>(app->GetWindow()->GetHeight()));
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigWindowsMoveFromTitleBarOnly = true;

		SetImGuiKeyCodes();
		SetImGuiStyle();

		m_IMGUIRenderer = std::unique_ptr<graphics::api::IMGUIRenderer>(graphics::api::IMGUIRenderer::Create(app->GetWindow()->GetWidth(),app->GetWindow()->GetHeight(), m_ClearScreen));

        if(m_IMGUIRenderer)
            m_IMGUIRenderer->Init();
	}

	void ImGuiLayer::OnDetach()
	{
	}

	void ImGuiLayer::OnUpdate(TimeStep* dt, Scene* scene)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.DeltaTime = dt->GetMillis();

		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
        
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

	void ImGuiLayer::OnNewScene(Scene* scene)
	{
		m_IMGUIRenderer->Clear();
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
		io.KeyAlt = io.KeysDown[LUMOS_KEY_ALT];// || io.KeysDown[LUMOS_KEY_ALT];

		
		return io.WantTextInput;
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
		io.DisplaySize = ImVec2(static_cast<float>(e.GetWidth()), static_cast<float>(e.GetHeight()));
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

	void ImGuiLayer::SetImGuiStyle()
	{
#if 0
		std::string filePath = "/CoreTextures/Roboto-Medium.ttf";
		std::string physicalPath;
		if (!VFS::Get()->ResolvePhysicalPath(filePath, physicalPath))
			LUMOS_CORE_ERROR("Failed to Load font {0}", filePath);

		filePath = physicalPath;
        ImGuiIO& io = ImGui::GetIO();
		io.Fonts->AddFontFromFileTTF(filePath.c_str(), 15.0f);
		io.IniFilename = nullptr;
#endif

		ImGuiStyle& style = ImGui::GetStyle();

		// dark theme, adapted from Cinder
		//style.WindowMinSize = ImVec2(160, 20);
		style.FramePadding = ImVec2(4, 2);
		style.ItemSpacing = ImVec2(6, 2);
		style.ItemInnerSpacing = ImVec2(6, 4);
		style.Alpha = 0.95f;
		//mStyle.WindowFillAlphaDefault = 1.0f;
		style.WindowRounding = 4.0f;
		style.FrameRounding = 2.0f;
		style.IndentSpacing = 6.0f;
		style.ItemInnerSpacing = ImVec2(2, 4);
		style.ColumnsMinSpacing = 50.0f;
		style.GrabMinSize = 14.0f;
		style.GrabRounding = 16.0f;
		style.ScrollbarSize = 12.0f;
		style.ScrollbarRounding = 16.0f;

		//style.Colors[ImGuiCol_Text] = ImVec4(0.86f, 0.93f, 0.89f, 0.61f);
		style.Colors[ImGuiCol_Text] = ImVec4(1, 1, 1, 1);
		style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.86f, 0.93f, 0.89f, 0.28f);
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
		//style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.20f, 0.22f, 0.27f, 0.58f);
		style.Colors[ImGuiCol_Border] = ImVec4(0.31f, 0.31f, 1.00f, 0.00f);
		style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.20f, 0.22f, 0.27f, 0.75f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.20f, 0.22f, 0.27f, 0.47f);
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.47f, 0.77f, 0.83f, 0.21f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		//style.Colors[ImGuiCol_ComboBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_Button] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.92f, 0.18f, 0.29f, 0.76f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		//style.Colors[ImGuiCol_Column] = ImVec4(0.47f, 0.77f, 0.83f, 0.32f);
		//style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
		//style.Colors[ImGuiCol_ColumnActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		//style.Colors[ImGuiCol_CloseButton] = ImVec4(0.86f, 0.93f, 0.89f, 0.16f);
		//style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.86f, 0.93f, 0.89f, 0.39f);
		//style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.86f, 0.93f, 0.89f, 1.00f);
		style.Colors[ImGuiCol_PlotLines] = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
		style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.92f, 0.18f, 0.29f, 0.43f);
		//style.Colors[ImGuiCol_TooltipBg] = ImVec4(0.47f, 0.77f, 0.83f, 0.72f);
		//style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.22f, 0.27f, 0.73f);
	}
}
