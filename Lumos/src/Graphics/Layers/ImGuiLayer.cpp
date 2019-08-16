#include "LM.h"
#include "ImGuiLayer.h"
#include "App/Input.h"
#include "App/Window.h"
#include "App/Application.h"
#include "Graphics/API/IMGUIRenderer.h"
#include "System/VFS.h"

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include <imgui/imgui.h>
#include <imgui/plugins/ImGuizmo.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>

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

		m_IMGUIRenderer = Scope<Graphics::IMGUIRenderer>(Graphics::IMGUIRenderer::Create(app->GetWindow()->GetWidth(),app->GetWindow()->GetHeight(), m_ClearScreen));

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
		ImGuiIO& io = ImGui::GetIO();
		std::string physicalPath;

#if 1
		std::string filePath = "/CoreTextures/Roboto-Medium.ttf";
		
		if (!VFS::Get()->ResolvePhysicalPath(filePath, physicalPath))
			LUMOS_CORE_ERROR("Failed to Load font {0}", filePath);

		filePath = physicalPath;
        
        static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
        ImFontConfig icons_config;
        icons_config.MergeMode = false;
        icons_config.PixelSnapH = true;
        icons_config.OversampleH = 2;
        icons_config.OversampleV = 1;
      //  icons_config.GlyphOffset.y -= 1.0f;      // Move everything by 1 pixels up
      // icons_config.GlyphExtraSpacing.x = 1.0f; // Increase spacing between characters
        icons_config.OversampleH = icons_config.OversampleV = 1;
        icons_config.PixelSnapH = true;
        
        io.Fonts->AddFontFromFileTTF(filePath.c_str(), 16.0f, &icons_config);// icons_ranges);
		io.IniFilename = nullptr;

#else
		io.Fonts->AddFontDefault();
#endif

		physicalPath = "";
		VFS::Get()->ResolvePhysicalPath("/CoreFonts/fa-solid-900.ttf", physicalPath);

		// merge in icons from Font Awesome
		icons_config.MergeMode = true;
		icons_config.PixelSnapH = true;
      	icons_config.OversampleH = 2;
      	icons_config.OversampleV = 1;
      	icons_config.GlyphOffset.y += 1.0f;      // Move everything by 1 pixels down
		icons_config.GlyphOffset.x -= 1.0f;      // Move everything by 1 pixels left
      	//icons_config.GlyphExtraSpacing.x = 1.0f; // Increase spacing between characters
        icons_config.OversampleH = icons_config.OversampleV = 1;
        icons_config.PixelSnapH = true;
        //if (icons_config.SizePixels <= 0.0f)
        icons_config.SizePixels = 13.0f * 1.0f;
     
       // const ImWchar* glyph_ranges = font_cfg.GlyphRanges != NULL ? font_cfg.GlyphRanges : GetGlyphRangesDefault();
        
		io.Fonts->AddFontFromFileTTF(physicalPath.c_str(), 16.0f, &icons_config, icons_ranges);

        ImGuiStyle& style = ImGui::GetStyle();
        
		style.PopupRounding = 3;
		style.WindowPadding = ImVec2(2, 2);
		style.FramePadding = ImVec2(2, 2);
		style.ItemSpacing = ImVec2(6, 2);
		style.ItemInnerSpacing = ImVec2(6, 4);
		style.IndentSpacing = 6.0f;

		style.ScrollbarSize = 18;

		style.WindowBorderSize = 1;
		style.ChildBorderSize = 1;
		style.PopupBorderSize = 1;
		style.FrameBorderSize = 0.0f;

		style.WindowRounding = 3;
		style.ChildRounding = 3;
		style.FrameRounding = 3;
		style.ScrollbarRounding = 2;
		style.GrabRounding = 3;

#ifdef IMGUI_HAS_DOCK
		style.TabBorderSize = 0.0f;
		style.TabRounding = 3;
#endif

		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		colors[ImGuiCol_Border] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
		colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	}
}
