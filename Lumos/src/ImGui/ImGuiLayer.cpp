#include "lmpch.h"
#include "ImGuiLayer.h"
#include "Core/OS/Input.h"
#include "Core/OS/Window.h"
#include "App/Application.h"
#include "Graphics/API/IMGUIRenderer.h"
#include "Core/VFS.h"
#include "ImGuiHelpers.h"
#include "Core/Profiler.h"

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include <imgui/imgui.h>
#include <imgui/plugins/ImGuizmo.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <imgui/plugins/ImGuiAl/fonts/CousineRegular.inl>
#include <imgui/plugins/ImGuiAl/fonts/KarlaRegular.inl>
#include <imgui/plugins/ImGuiAl/fonts/GoogleMaterialDesign.inl>
#include <imgui/plugins/ImGuiAl/fonts/FontAwesome5Solid900.inl>
#include <imgui/plugins/ImGuiAl/fonts/FontAwesome5Brands400.inl>
#include <imgui/misc/freetype/imgui_freetype.h>

namespace Lumos
{
	ImGuiLayer::ImGuiLayer(bool clearScreen, const std::string& debugName)
		: Layer(debugName)
	{
		m_ClearScreen = clearScreen;
		m_FontSize = 16.0f;
	}

	ImGuiLayer::~ImGuiLayer()
	{
	}

	void ImGuiLayer::OnAttach()
	{
		Application& app = Application::Get();
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(static_cast<float>(app.GetWindow()->GetWidth()), static_cast<float>(app.GetWindow()->GetHeight()));
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    
        #ifdef LUMOS_PLATFORM_IOS
        io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
        #endif
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
		io.ConfigWindowsMoveFromTitleBarOnly = true;
    
		SetImGuiKeyCodes();
		SetImGuiStyle();

		m_IMGUIRenderer = Scope<Graphics::IMGUIRenderer>(Graphics::IMGUIRenderer::Create(app.GetWindow()->GetWidth(),app.GetWindow()->GetHeight(), m_ClearScreen));

        if(m_IMGUIRenderer)
            m_IMGUIRenderer->Init();
	}

	void ImGuiLayer::OnDetach()
	{
	}

	void ImGuiLayer::OnUpdate(const TimeStep& dt, Scene* scene)
	{
		LUMOS_PROFILE_FUNC;
		ImGuizmo::BeginFrame();

		Application::Get().OnImGui();
        
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
		io.KeyRepeatDelay = 400.0f;
		io.KeyRepeatRate = 40.0f;
	}

	void ImGuiLayer::SetImGuiStyle()
	{
		ImGuiIO& io = ImGui::GetIO();

		ImGui::StyleColorsDark();

		std::string physicalPath;

		std::string filePath = "/CoreTextures/Roboto-Medium.ttf";
		
		if (!VFS::Get()->ResolvePhysicalPath(filePath, physicalPath))
			LUMOS_LOG_CRITICAL("Failed to Load font {0}", filePath);

        io.FontGlobalScale = 1.0f;
		filePath = physicalPath;
        ImFontConfig icons_config;
        icons_config.MergeMode = false;
        icons_config.PixelSnapH = true;
        icons_config.OversampleH = icons_config.OversampleV = 1;
        icons_config.PixelSnapH = true;
        
        io.Fonts->AddFontFromFileTTF(filePath.c_str(), m_FontSize, &icons_config);
		AddIconFont();

		io.Fonts->AddFontDefault();
		io.Fonts->AddFontFromMemoryCompressedTTF(KarlaRegular_compressed_data, KarlaRegular_compressed_size, m_FontSize, &icons_config);
		AddIconFont();

		io.Fonts->AddFontFromMemoryCompressedTTF(CousineRegular_compressed_data, CousineRegular_compressed_size, m_FontSize, &icons_config);
		AddIconFont();
    
        io.Fonts->TexGlyphPadding = 1;
        for (int n = 0; n < io.Fonts->ConfigData.Size; n++)
        {
            ImFontConfig* font_config = (ImFontConfig*)&io.Fonts->ConfigData[n];
            font_config->RasterizerMultiply = 1.0f;
            font_config->RasterizerFlags = ImGuiFreeType::RasterizerFlags::ForceAutoHint;
        }
    
        ImGuiFreeType::BuildFontAtlas(io.Fonts, ImGuiFreeType::RasterizerFlags::ForceAutoHint);

        ImGuiStyle& style = ImGui::GetStyle();
        
		style.WindowPadding = ImVec2(5,5);
		style.FramePadding = ImVec2(2, 2);
		style.ItemSpacing = ImVec2(6, 2);
		style.ItemInnerSpacing = ImVec2(6, 4);
		style.IndentSpacing = 6.0f;

		style.ScrollbarSize = 18;

		style.WindowBorderSize = 0;
		style.ChildBorderSize = 1;
		style.PopupBorderSize = 3;
		style.PopupRounding = 4;
		style.FrameBorderSize = 0.0f;
		
		style.WindowRounding = 4;
		style.ChildRounding = 4;
		style.FrameRounding = 4;
		style.ScrollbarRounding = 4;
		style.GrabRounding = 4;
        style.WindowMinSize = ImVec2(10.0f,10.0f);

#ifdef IMGUI_HAS_DOCK 
		style.TabBorderSize = 0.0f;
		style.TabRounding = 4;

		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 4;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
#endif

		ImGuiHelpers::SetTheme(ImGuiHelpers::Theme::Dark);
	}

	void ImGuiLayer::AddIconFont()
	{
		ImGuiIO& io = ImGui::GetIO();

#define USE_FA_ICONS
#ifdef USE_FA_ICONS
		static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		ImFontConfig icons_config;
		// merge in icons from Font Awesome
		icons_config.MergeMode = true;
		icons_config.PixelSnapH = true;
		icons_config.GlyphOffset.y += 1.0f;
		icons_config.OversampleH = icons_config.OversampleV = 1;
		icons_config.PixelSnapH = true;
		icons_config.SizePixels = 13.0f * 1.0f;

		io.Fonts->AddFontFromMemoryCompressedTTF(FontAwesome5Solid900_compressed_data, FontAwesome5Solid900_compressed_size, m_FontSize, &icons_config, icons_ranges);
        io.Fonts->AddFontFromMemoryCompressedTTF(FontAwesome5Brands400_compressed_data, FontAwesome5Brands400_compressed_size, m_FontSize, &icons_config, icons_ranges);

#else

		static const ImWchar ranges[] = { ICON_MIN_MD, ICON_MAX_MD, 0 };
		ImFontConfig config;
		config.MergeMode = true;
		io.Fonts->AddFontFromMemoryCompressedTTF(GoogleMaterialDesign_compressed_data, GoogleMaterialDesign_compressed_size, m_FontSize, &config, ranges);
#endif
	}
}
