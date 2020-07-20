#include "lmpch.h"
#include "Editor.h"
#include "SceneWindow.h"
#include "ProfilerWindow.h"
#include "ConsoleWindow.h"
#include "HierarchyWindow.h"
#include "InspectorWindow.h"
#include "ApplicationInfoWindow.h"
#include "GraphicsInfoWindow.h"
#include "TextEditWindow.h"
#include "AssetWindow.h"
#include "EditorCamera.h"

#include "Core/Application.h"
#include "Core/OS/Input.h"
#include "Core/OS/FileSystem.h"
#include "Core/Profiler.h"
#include "Core/Version.h"
#include "Core/Engine.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Events/ApplicationEvent.h"

#include "Scene/Component/Components.h"
#include "Scripting/ScriptComponent.h"

#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Physics/B2PhysicsEngine/B2PhysicsEngine.h"

#include "Graphics/Renderers/ForwardRenderer.h"
#include "Graphics/MeshFactory.h"
#include "Graphics/Layers/Layer3D.h"
#include "Graphics/Sprite.h"
#include "Graphics/Light.h"
#include "Graphics/API/Texture.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Layers/LayerStack.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/MeshFactory.h"
#include "Graphics/Renderers/GridRenderer.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Graphics/ModelLoader/ModelLoader.h"
#include "Graphics/Environment.h"

#include "Utilities/AssetsManager.h"

#include "ImGui/ImGuiHelpers.h"

#include <imgui/imgui_internal.h>
#include <imgui/plugins/ImGuizmo.h>
#include <imgui/plugins/ImGuiAl/button/imguial_button.h>
#include <imgui/plugins/ImTextEditor.h>
#include <IconFontCppHeaders/IconsMaterialDesignIcons.h>

#include <imgui/plugins/ImFileBrowser.h>

static ImVec2 operator+(const ImVec2& a, const ImVec2& b)
{
	return ImVec2(a.x + b.x, a.y + b.y);
}

namespace Lumos
{
	Editor::Editor(Application* app, u32 width, u32 height)
		: m_Application(app)
		, m_Selected()
		, m_IniFile("")
	{
	}

	Editor::~Editor()
	{
		SaveEditorSettings();
		// put back when stop overriding with a scene camera
		delete m_EditorCamera;
	}

	void Editor::OnInit()
	{
		const char* ini[] = {ROOT_DIR "/Editor.ini", ROOT_DIR "/Editor/Editor.ini"};
		bool fileFound = false;
		std::string filePath;
		for(int i = 0; i < IM_ARRAYSIZE(ini); ++i)
		{
			auto fexist = [](const char* f) -> bool {
				FILE* fp = fopen(f, "rb");
				return fp ? (static_cast<void>(fclose(fp)), 1) : 0;
			};
			if(fexist(ini[i]))
			{
				filePath = ini[i];
				m_IniFile = IniFile(filePath);
				// ImGui::GetIO().IniFilename = ini[i];
				fileFound = true;
				LoadEditorSettings();
				break;
			}
		}

		if(!fileFound)
		{
			filePath = ROOT_DIR "/editor.ini";
			FileSystem::WriteTextFile(filePath, "");
			m_IniFile = IniFile(filePath);
			AddDefaultEditorSettings();
			// ImGui::GetIO().IniFilename = "editor.ini";
		}

		m_EditorCamera = new Camera(-20.0f,
			-40.0f,
			Maths::Vector3(-31.0f, 12.0f, 51.0f),
			60.0f,
			0.1f,
			1000.0f,
			(float)m_Application->GetWindowSize().x / (float)m_Application->GetWindowSize().y);
		m_EditorCamera->SetCameraController(CreateRef<EditorCameraController>());
		m_CurrentCamera = m_EditorCamera;

		m_ComponentIconMap[typeid(Graphics::Light).hash_code()] = " " ICON_MDI_LIGHTBULB " ";
		m_ComponentIconMap[typeid(Camera).hash_code()] = ICON_MDI_CAMERA;
		m_ComponentIconMap[typeid(SoundComponent).hash_code()] = ICON_MDI_VOLUME_HIGH;
		m_ComponentIconMap[typeid(Graphics::Sprite).hash_code()] = ICON_MDI_IMAGE;
		m_ComponentIconMap[typeid(Maths::Transform).hash_code()] = ICON_MDI_VECTOR_LINE;
		m_ComponentIconMap[typeid(Physics2DComponent).hash_code()] = ICON_MDI_SQUARE_OUTLINE;
		m_ComponentIconMap[typeid(Physics3DComponent).hash_code()] = ICON_MDI_CUBE_OUTLINE;
		m_ComponentIconMap[typeid(MeshComponent).hash_code()] = ICON_MDI_SHAPE;
		m_ComponentIconMap[typeid(MaterialComponent).hash_code()] = ICON_MDI_BRUSH;
		m_ComponentIconMap[typeid(ScriptComponent).hash_code()] = ICON_MDI_SCRIPT;
		m_ComponentIconMap[typeid(Graphics::Environment).hash_code()] = ICON_MDI_EARTH;

		m_Windows.emplace_back(CreateRef<ConsoleWindow>());
		m_Windows.emplace_back(CreateRef<SceneWindow>());
		m_Windows.emplace_back(CreateRef<ProfilerWindow>());
		m_Windows.back()->SetActive(false);
		m_Windows.emplace_back(CreateRef<InspectorWindow>());
		m_Windows.emplace_back(CreateRef<HierarchyWindow>());
		m_Windows.emplace_back(CreateRef<GraphicsInfoWindow>());
		m_Windows.back()->SetActive(false);
		m_Windows.emplace_back(CreateRef<ApplicationInfoWindow>());
		m_Windows.emplace_back(CreateRef<AssetWindow>());

		for(auto& window : m_Windows)
			window->SetEditor(this);

		m_DebugDrawFlags =
			0; // EditorDebugFlags::MeshBoundingBoxes | EditorDebugFlags::CameraFrustum | EditorDebugFlags::SpriteBoxes;

		m_ShowImGuiDemo = false;

		ImGuiHelpers::SetTheme(ImGuiHelpers::Dark);

		m_Selected = entt::null;

		m_PreviewTexture = nullptr;
	}

	bool IsTextFile(const std::string& filePath)
	{
		std::string extension = StringFormat::GetFilePathExtension(filePath);

		if(extension == "txt" || extension == "glsl" || extension == "shader" || extension == "vert"
			|| extension == "frag" || extension == "lua" || extension == "Lua")
			return true;

		return false;
	}

	bool IsAudioFile(const std::string& filePath)
	{
		std::string extension = StringFormat::GetFilePathExtension(filePath);

		if(extension == "ogg" || extension == "wav")
			return true;

		return false;
	}

	bool IsModelFile(const std::string& filePath)
	{
		std::string extension = StringFormat::GetFilePathExtension(filePath);

		if(extension == "obj" || extension == "gltf" || extension == "glb" || extension == "fbx" || extension == "FBX")
			return true;

		return false;
	}

	void Editor::OnImGui()
	{
		LUMOS_PROFILE_FUNC;
		DrawMenuBar();

		BeginDockSpace(false);

		for(auto& window : m_Windows)
		{
			if(window->Active())
				window->OnImGui();
		}

		if(m_ShowImGuiDemo)
			ImGui::ShowDemoWindow(&m_ShowImGuiDemo);

		m_View2D = m_CurrentCamera->IsOrthographic();

		m_FileBrowserWindow.OnImGui();

		if(m_Application->GetEditorState() == EditorState::Preview)
			m_Application->GetSceneManager()->GetCurrentScene()->UpdateSceneGraph();

		EndDockSpace();
	}

	Graphics::RenderAPI StringToRenderAPI(const std::string& name)
	{
#ifdef LUMOS_RENDER_API_VULKAN
		if(name == "Vulkan")
			return Graphics::RenderAPI::VULKAN;
#endif
#ifdef LUMOS_RENDER_API_OPENGL
		if(name == "OpenGL")
			return Graphics::RenderAPI::OPENGL;
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
		if(name == "Direct3D11")
			return Graphics::RenderAPI::DIRECT3D;
#endif

		Lumos::Debug::Log::Error("Unsupported Graphics API");

		return Graphics::RenderAPI::OPENGL;
	}

	void Editor::OpenFile()
	{
		m_FileBrowserWindow.SetCallback(BIND_FILEBROWSER_FN(Editor::FileOpenCallback));
		m_FileBrowserWindow.Open();
	}

	void Editor::DrawMenuBar()
	{
		if(ImGui::BeginMainMenuBar())
		{
			if(ImGui::BeginMenu("File"))
			{
				if(ImGui::MenuItem("Exit"))
				{
					Application::Get().SetAppState(AppState::Closing);
				}

				if(ImGui::MenuItem("Open File"))
				{
					m_FileBrowserWindow.SetCallback(BIND_FILEBROWSER_FN(Editor::FileOpenCallback));
					m_FileBrowserWindow.Open();
				}

				if(ImGui::MenuItem("New Scene"))
				{
					m_Application->GetSceneManager()->EnqueueScene<Scene>("New Scene");
					m_Application->GetSceneManager()->SwitchScene((int)(m_Application->GetSceneManager()->GetScenes().size()) - 1);
				}

				if(ImGui::BeginMenu("Style"))
				{
					if(ImGui::MenuItem("Dark", ""))
					{
						ImGuiHelpers::SetTheme(ImGuiHelpers::Dark);
					}
					if(ImGui::MenuItem("Black", ""))
					{
						ImGuiHelpers::SetTheme(ImGuiHelpers::Black);
					}
					if(ImGui::MenuItem("Grey", ""))
					{
						ImGuiHelpers::SetTheme(ImGuiHelpers::Grey);
					}
					if(ImGui::MenuItem("Light", ""))
					{
						ImGuiHelpers::SetTheme(ImGuiHelpers::Light);
					}
					if(ImGui::MenuItem("Cherry", ""))
					{
						ImGuiHelpers::SetTheme(ImGuiHelpers::Cherry);
					}
					if(ImGui::MenuItem("Blue", ""))
					{
						ImGuiHelpers::SetTheme(ImGuiHelpers::Blue);
					}
					if(ImGui::MenuItem("Cinder", ""))
					{
						ImGuiHelpers::SetTheme(ImGuiHelpers::Cinder);
					}
					if(ImGui::MenuItem("Classic", ""))
					{
						ImGuiHelpers::SetTheme(ImGuiHelpers::Classic);
					}
					if(ImGui::MenuItem("ClassicDark", ""))
					{
						ImGuiHelpers::SetTheme(ImGuiHelpers::ClassicDark);
					}
					if(ImGui::MenuItem("ClassicLight", ""))
					{
						ImGuiHelpers::SetTheme(ImGuiHelpers::ClassicLight);
					}
					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}
			if(ImGui::BeginMenu("Edit"))
			{
				if(ImGui::MenuItem("Undo", "CTRL+Z"))
				{
				}
				if(ImGui::MenuItem("Redo", "CTRL+Y", false, false))
				{
				} // Disabled item
				ImGui::Separator();
				if(ImGui::MenuItem("Cut", "CTRL+X"))
				{
				}
				if(ImGui::MenuItem("Copy", "CTRL+C"))
				{
				}
				if(ImGui::MenuItem("Paste", "CTRL+V"))
				{
				}
				ImGui::EndMenu();
			}

			if(ImGui::BeginMenu("Windows"))
			{
				for(auto& window : m_Windows)
				{
					if(ImGui::MenuItem(window->GetName().c_str(), "", &window->Active(), true))
					{
						window->SetActive(true);
					}
				}

				if(ImGui::MenuItem("ImGui Demo", "", &m_ShowImGuiDemo, true))
				{
					m_ShowImGuiDemo = true;
				}

				ImGui::EndMenu();
			}

			if(ImGui::BeginMenu("Scenes"))
			{
				auto scenes = Application::Get().GetSceneManager()->GetSceneNames();

				for(size_t i = 0; i < scenes.size(); i++)
				{
					auto name = scenes[i];
					if(ImGui::MenuItem(name.c_str()))
					{
						Application::Get().GetSceneManager()->SwitchScene(name);
					}
				}
				ImGui::EndMenu();
			}

			if(ImGui::BeginMenu("Entity"))
			{
				auto& registry = m_Application->GetSceneManager()->GetCurrentScene()->GetRegistry();

				if(ImGui::MenuItem("CreateEmpty"))
				{
					registry.create();
				}

				if(ImGui::MenuItem("Cube"))
				{
					auto entity = registry.create();
					registry.emplace<MeshComponent>(entity, Graphics::CreatePrimative(Graphics::PrimitiveType::Cube));
					registry.emplace<NameComponent>(entity, "Cube");
					registry.emplace<Maths::Transform>(entity);
				}

				if(ImGui::MenuItem("Sphere"))
				{
					auto entity = registry.create();
					registry.emplace<MeshComponent>(entity, Graphics::CreatePrimative(Graphics::PrimitiveType::Sphere));
					registry.emplace<NameComponent>(entity, "Sphere");
					registry.emplace<Maths::Transform>(entity);
				}

				if(ImGui::MenuItem("Pyramid"))
				{
					auto entity = registry.create();
					registry.emplace<MeshComponent>(
						entity, Graphics::CreatePrimative(Graphics::PrimitiveType::Pyramid));
					registry.emplace<NameComponent>(entity, "Pyramid");
					registry.emplace<Maths::Transform>(entity);
				}

				if(ImGui::MenuItem("Plane"))
				{
					auto entity = registry.create();
					registry.emplace<MeshComponent>(entity, Graphics::CreatePrimative(Graphics::PrimitiveType::Plane));
					registry.emplace<NameComponent>(entity, "Plane");
					registry.emplace<Maths::Transform>(entity);
				}

				if(ImGui::MenuItem("Cylinder"))
				{
					auto entity = registry.create();
					registry.emplace<MeshComponent>(
						entity, Graphics::CreatePrimative(Graphics::PrimitiveType::Cylinder));
					registry.emplace<NameComponent>(entity, "Cylinder");
					registry.emplace<Maths::Transform>(entity);
				}

				if(ImGui::MenuItem("Capsule"))
				{
					auto entity = registry.create();
					registry.emplace<MeshComponent>(
						entity, Graphics::CreatePrimative(Graphics::PrimitiveType::Capsule));
					registry.emplace<NameComponent>(entity, "Capsule");
					registry.emplace<Maths::Transform>(entity);
				}

				ImGui::EndMenu();
			}

			if(ImGui::BeginMenu("Graphics"))
			{
				if(ImGui::MenuItem("Compile Shaders"))
				{
					RecompileShaders();
				}
				ImGui::EndMenu();
			}

			if(ImGui::BeginMenu("About"))
			{
				auto& version = Lumos::LumosVersion;
				ImGui::Text("Version : %d.%d.%d", version.major, version.minor, version.patch);
				ImGui::Separator();

				std::string githubMenuText = ICON_MDI_GITHUB_BOX " Github";
				if(ImGui::MenuItem(githubMenuText.c_str()))
				{
#ifdef LUMOS_PLATFORM_WINDOWS
// TODO
#else
#	ifndef LUMOS_PLATFORM_IOS
					system("open https://www.github.com/jmorton06/Lumos");
#	endif
#endif
				}

				ImGui::EndMenu();
			}

			ImGui::SameLine(ImGui::GetWindowContentRegionMax().x / 2.0f);

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.2f, 0.7f, 0.0f));

			if(m_Application->GetEditorState() == EditorState::Next)
				m_Application->SetEditorState(EditorState::Paused);

			bool selected;
			{
				selected = m_Application->GetEditorState() == EditorState::Play;
				if(selected)
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.9f, 1.0f));

				if(ImGui::Button(ICON_MDI_PLAY, ImVec2(19.0f, 19.0f)))
                {
                    m_Application->GetSystem<LumosPhysicsEngine>()->SetPaused(selected);
                    m_Application->GetSystem<B2PhysicsEngine>()->SetPaused(selected);
                    m_Application->SetEditorState(selected ? EditorState::Preview : EditorState::Play);
                }

				ImGuiHelpers::Tooltip("Play");

				if(selected)
					ImGui::PopStyleColor();
			}

			ImGui::SameLine();

			{
				selected = m_Application->GetEditorState() == EditorState::Paused;
				if(selected)
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.9f, 1.0f));

				if(ImGui::Button(ICON_MDI_PAUSE, ImVec2(19.0f, 19.0f)))
					m_Application->SetEditorState(selected ? EditorState::Play : EditorState::Paused);

				ImGuiHelpers::Tooltip("Pause");

				if(selected)
					ImGui::PopStyleColor();
			}

			ImGui::SameLine();

			{
				selected = m_Application->GetEditorState() == EditorState::Next;
				if(selected)
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.9f, 1.0f));

				if(ImGui::Button(ICON_MDI_STEP_FORWARD, ImVec2(19.0f, 19.0f)))
					m_Application->SetEditorState(EditorState::Next);

				ImGuiHelpers::Tooltip("Next");

				if(selected)
					ImGui::PopStyleColor();
			}

			ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 100.0f);

			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetColorU32(ImGuiCol_TitleBg));
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(7, 2));

			bool setNewValue = false;
			std::string RenderAPI = "";
			static auto renderAPI = Graphics::GraphicsContext::GetRenderAPI();

			bool needsRestart = false;
			if(renderAPI != Graphics::GraphicsContext::GetRenderAPI())
			{
				needsRestart = true;
			}

			switch(renderAPI)
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case Graphics::RenderAPI::OPENGL:
				RenderAPI = "OpenGL";
				break;
#endif

#ifdef LUMOS_RENDER_API_VULKAN
			case Graphics::RenderAPI::VULKAN:
				RenderAPI = "Vulkan";
				break;
#endif

#ifdef LUMOS_RENDER_API_DIRECT3D
			case DIRECT3D:
				RenderAPI = "Direct3D";
				break;
#endif
			default:
				break;
			}

			int numSupported = 0;
#ifdef LUMOS_RENDER_API_OPENGL
			numSupported++;
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			numSupported++;
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D11
			numSupported++;
#endif
			const char* api[] = {"OpenGL", "Vulkan", "Direct3D11"};
			const char* current_api = RenderAPI.c_str();
			if(needsRestart)
				RenderAPI = "*" + RenderAPI;

			ImGui::PushItemWidth(-1.0f);
			if(ImGui::BeginCombo(
				   "", current_api, 0)) // The second parameter is the label previewed before opening the combo.
			{
				for(int n = 0; n < numSupported; n++)
				{
					bool is_selected = (current_api == api[n]);
					if(ImGui::Selectable(api[n], current_api))
					{
						setNewValue = true;
						current_api = api[n];
					}
					if(is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			if(needsRestart)
				ImGuiHelpers::Tooltip("Restart needed to switch Render API");

			if(setNewValue)
			{
				renderAPI = StringToRenderAPI(current_api);
				auto& appProps = m_Application->m_InitialProperties;
				if(appProps.FilePath != "")
				{
					std::string physicalPath;
					if(VFS::Get()->ResolvePhysicalPath(appProps.FilePath, physicalPath))
					{
						auto configFile = FileSystem::ReadTextFile(physicalPath);

						std::string toReplace("renderAPI=");
						size_t pos = configFile.find(toReplace);

						if(pos != std::string::npos)
						{
							configFile.replace(pos + 10, 1, StringFormat::ToString(int(renderAPI)));

							FileSystem::WriteTextFile(physicalPath, configFile);
						}
					}
				}
			}

			ImGui::PopStyleColor(2);
			ImGui::PopStyleVar();

			ImGui::EndMainMenuBar();
		}
	}

	static const float identityMatrix[16] =
		{1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f};

	void Editor::OnImGuizmo()
	{
		Maths::Matrix4 view = m_CurrentCamera->GetViewMatrix();
		Maths::Matrix4 proj = m_CurrentCamera->GetProjectionMatrix();

#ifdef LUMOS_RENDER_API_VULKAN
		if(Graphics::GraphicsContext::GetRenderAPI() == Graphics::RenderAPI::VULKAN)
			proj.m11_ *= -1.0f;
#endif

		view = view.Transpose();
		proj = proj.Transpose();

		//if(m_ShowGrid && !m_CurrentCamera->IsOrthographic())
		//  ImGuizmo::DrawGrid(Maths::ValuePointer(view),
		//                   Maths::ValuePointer(proj), identityMatrix, 120.f);

		if(m_Selected == entt::null || m_ImGuizmoOperation == 4)
			return;

		if(m_ShowGizmos)
		{
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetOrthographic(m_CurrentCamera->IsOrthographic());

			auto& registry = m_Application->GetSceneManager()->GetCurrentScene()->GetRegistry();
			auto transform = registry.try_get<Maths::Transform>(m_Selected);
			if(transform != nullptr)
			{
				Maths::Matrix4 model = transform->GetWorldMatrix();
				model = model.Transpose();

				float snapAmount[3] = {m_SnapAmount, m_SnapAmount, m_SnapAmount};
				float delta[16];

				ImGuizmo::Manipulate(Maths::ValuePointer(view),
					Maths::ValuePointer(proj),
					static_cast<ImGuizmo::OPERATION>(m_ImGuizmoOperation),
					ImGuizmo::LOCAL,
					Maths::ValuePointer(model),
					delta,
					m_SnapQuizmo ? snapAmount : nullptr);

				if(ImGuizmo::IsUsing())
				{
					if(static_cast<ImGuizmo::OPERATION>(m_ImGuizmoOperation) == ImGuizmo::OPERATION::SCALE)
					{
						auto mat = Maths::Matrix4(delta).Transpose();
						transform->SetLocalScale(transform->GetLocalScale() * mat.Scale());
					}
					else
					{
						auto mat = Maths::Matrix4(delta).Transpose() * transform->GetLocalMatrix();
						transform->SetLocalTransform(mat);

						auto physics2DComponent = registry.try_get<Physics2DComponent>(m_Selected);

						if(physics2DComponent)
						{
							physics2DComponent->GetRigidBody()->SetPosition(
								{mat.Translation().x, mat.Translation().y});
						}
						else
						{
							auto physics3DComponent = registry.try_get<Physics3DComponent>(m_Selected);
							if(physics3DComponent)
							{
								physics3DComponent->GetRigidBody()->SetPosition(mat.Translation());
								physics3DComponent->GetRigidBody()->SetOrientation(mat.Rotation());
							}
						}
					}
				}
			}
		}

		if(m_Selected != entt::null && m_ShowViewSelected)
		{
			auto& registry = m_Application->GetSceneManager()->GetCurrentScene()->GetRegistry();

			auto transform = registry.try_get<Maths::Transform>(m_Selected);

			float pos[3] = {
				m_EditorCamera->GetPosition().x, m_EditorCamera->GetPosition().y, m_EditorCamera->GetPosition().z};
			float rot[3] = {m_EditorCamera->GetPitch(), m_EditorCamera->GetYaw(), m_EditorCamera->GetRoll()};
			float scale[3] = {1.0f, 1.0f, 1.0f};
			float view[16];
			ImGuizmo::RecomposeMatrixFromComponents(pos, rot, scale, view);

			float camDistance =
				1.0f; // transform ?
			// (Application::Get().GetSceneManager()->GetCurrentScene()->GetCamera()->GetPosition()
			// - transform->GetWorldMatrix().Translation()).Length() : 0.0f;

			auto window = ImGui::GetCurrentWindow();
			auto size = 128.0f;
			auto windowPos = window->Pos;
			auto windowSize = window->Size;
			auto viewManipulatePos = ImVec2(windowPos.x + windowSize.x - size, size / 2.0f - 20.0f + windowPos.y);

			ImGuizmo::ViewManipulate(view, camDistance, viewManipulatePos, ImVec2(size, size), 0x10101010);
			ImGuizmo::DecomposeMatrixToComponents(view, pos, rot, scale);

			// if (modified)
			{
				m_EditorCamera->SetPitch(rot[0]);
				m_EditorCamera->SetYaw(rot[1]);
				m_EditorCamera->SetRoll(rot[2]);
				m_EditorCamera->SetPosition({pos[0], pos[1], pos[2]});
			}
		}
	}

	void Editor::BeginDockSpace(bool infoBar)
	{
		static bool p_open = true;
		static bool opt_fullscreen_persistant = true;
		static ImGuiDockNodeFlags opt_flags = ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoCloseButton;
		bool opt_fullscreen = opt_fullscreen_persistant;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
		if(opt_fullscreen)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();

			auto pos = viewport->Pos;
			auto size = viewport->Size;
			bool menuBar = true;
			if(menuBar)
			{
				const float infoBarSize = 19.0f;
				pos.y += infoBarSize;
				size.y -= infoBarSize;
			}

			if(infoBar)
			{
				const float infoBarSize = 24.0f;
				pos.y += infoBarSize;
				size.y -= infoBarSize;
			}

			ImGui::SetNextWindowPos(pos);
			ImGui::SetNextWindowSize(size);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
							| ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		// When using ImGuiDockNodeFlags_PassthruDockspace, DockSpace() will render our background and handle the
		// pass-thru hole, so we ask Begin() to not render a background.
		if(opt_flags & ImGuiDockNodeFlags_DockSpace)
			window_flags |= ImGuiWindowFlags_NoBackground;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("MyDockspace", &p_open, window_flags);
		ImGui::PopStyleVar();

		if(opt_fullscreen)
			ImGui::PopStyleVar(2);
    
        ImGuiID DockspaceID = ImGui::GetID("MyDockspace");

        if (!ImGui::DockBuilderGetNode(DockspaceID))
		{
            ImGui::DockBuilderRemoveNode(DockspaceID); // Clear out existing layout
            ImGui::DockBuilderAddNode(DockspaceID); // Add empty node
            ImGui::DockBuilderSetNodeSize(DockspaceID, ImGui::GetIO().DisplaySize);

            ImGuiID dock_main_id = DockspaceID;
            ImGuiID DockBottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.3f, nullptr, &dock_main_id);
            ImGuiID DockLeft = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);
            ImGuiID DockRight = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.20f, nullptr, &dock_main_id);

            ImGuiID DockLeftChild = ImGui::DockBuilderSplitNode(DockLeft, ImGuiDir_Down, 0.875f, nullptr, &DockLeft);
            ImGuiID DockRightChild = ImGui::DockBuilderSplitNode(DockRight, ImGuiDir_Down, 0.875f, nullptr, &DockRight);
            ImGuiID DockingLeftDownChild = ImGui::DockBuilderSplitNode(DockLeftChild, ImGuiDir_Down, 0.06f, nullptr, &DockLeftChild);
            ImGuiID DockingRightDownChild = ImGui::DockBuilderSplitNode(DockRightChild, ImGuiDir_Down, 0.06f, nullptr, &DockRightChild);
        
            ImGuiID DockBottomChild = ImGui::DockBuilderSplitNode(DockBottom, ImGuiDir_Down, 0.2f, nullptr, &DockBottom);
            ImGuiID DockingBottomLeftChild = ImGui::DockBuilderSplitNode(DockBottomChild, ImGuiDir_Left, 0.5f, nullptr, &DockBottomChild);
            ImGuiID DockingBottomRightChild = ImGui::DockBuilderSplitNode(DockBottomChild, ImGuiDir_Right, 0.5f, nullptr, &DockBottomChild);
            
            ImGuiID DockMiddle = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.8f, nullptr, &dock_main_id);
        
            ImGui::DockBuilderDockWindow("###scene", DockMiddle);
            ImGui::DockBuilderDockWindow("###inspector", DockRight);
            ImGui::DockBuilderDockWindow("###console", DockingBottomLeftChild);
            ImGui::DockBuilderDockWindow("###profiler", DockingBottomLeftChild);
            ImGui::DockBuilderDockWindow("Assets", DockingBottomRightChild);
            ImGui::DockBuilderDockWindow("Dear ImGui Demo", DockLeft);
            ImGui::DockBuilderDockWindow("GraphicsInfo", DockLeft);
            ImGui::DockBuilderDockWindow("ApplicationInfo", DockLeft);
            ImGui::DockBuilderDockWindow("###hierarchy", DockLeft);

			ImGui::DockBuilderFinish(DockspaceID);
		}

		// Dockspace
		ImGuiIO& io = ImGui::GetIO();
		if(io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), opt_flags);
		}
	}

	void Editor::EndDockSpace()
	{
		ImGui::End();
	}

	void Editor::OnNewScene(Scene* scene)
	{
		m_Selected = entt::null;

		for(auto window : m_Windows)
		{
			window->OnNewScene(scene);
		}
	}

	void Editor::Draw3DGrid()
	{
#if 1
		if(!m_GridRenderer)
		{
			return;
		}

		m_GridRenderer->BeginScene(Application::Get().GetSceneManager()->GetCurrentScene(), m_EditorCamera);
		m_GridRenderer->RenderScene(Application::Get().GetSceneManager()->GetCurrentScene());
#endif
	}

	void Editor::Draw2DGrid(ImDrawList* drawList,
		const ImVec2& cameraPos,
		const ImVec2& windowPos,
		const ImVec2& canvasSize,
		const float factor,
		const float thickness)
	{
		static const auto graduation = 10;
		float GRID_SZ = canvasSize.y * 0.5f / factor;
		const ImVec2& offset = {
			canvasSize.x * 0.5f - cameraPos.x * GRID_SZ, canvasSize.y * 0.5f + cameraPos.y * GRID_SZ};

		ImU32 GRID_COLOR = IM_COL32(200, 200, 200, 40);
		float gridThickness = 1.0f;

		const auto& gridColor = GRID_COLOR;
		auto smallGraduation = GRID_SZ / graduation;
		const auto& smallGridColor = IM_COL32(100, 100, 100, smallGraduation);

		for(float x = -GRID_SZ; x < canvasSize.x + GRID_SZ; x += GRID_SZ)
		{
			auto localX = floorf(x + fmodf(offset.x, GRID_SZ));
			drawList->AddLine(
				ImVec2{localX, 0.0f} + windowPos, ImVec2{localX, canvasSize.y} + windowPos, gridColor, gridThickness);

			if(smallGraduation > 5.0f)
			{
				for(int i = 1; i < graduation; ++i)
				{
					const auto graduation = floorf(localX + smallGraduation * i);
					drawList->AddLine(ImVec2{graduation, 0.0f} + windowPos,
						ImVec2{graduation, canvasSize.y} + windowPos,
						smallGridColor,
						1.0f);
				}
			}
		}

		for(float y = -GRID_SZ; y < canvasSize.y + GRID_SZ; y += GRID_SZ)
		{
			auto localY = floorf(y + fmodf(offset.y, GRID_SZ));
			drawList->AddLine(
				ImVec2{0.0f, localY} + windowPos, ImVec2{canvasSize.x, localY} + windowPos, gridColor, gridThickness);

			if(smallGraduation > 5.0f)
			{
				for(int i = 1; i < graduation; ++i)
				{
					const auto graduation = floorf(localY + smallGraduation * i);
					drawList->AddLine(ImVec2{0.0f, graduation} + windowPos,
						ImVec2{canvasSize.x, graduation} + windowPos,
						smallGridColor,
						1.0f);
				}
			}
		}
	}

	void Editor::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Editor::OnWindowResize));

		m_Application->OnEvent(e);
	}

	Maths::Ray Editor::GetScreenRay(int x, int y, Camera* camera, int width, int height)
	{
		if(!camera)
			return Maths::Ray();

		float screenX = (float)x / (float)width;
		float screenY = (float)y / (float)height;

		bool flipY = false;

#ifdef LUMOS_RENDER_API_OPENGL
		if(Graphics::GraphicsContext::GetRenderAPI() == Graphics::RenderAPI::OPENGL)
			flipY = true;
#endif
		return camera->GetScreenRay(screenX, screenY, flipY);
	}

	void Editor::OnUpdate(const TimeStep& ts)
	{
        if(m_Application->GetEditorState() == EditorState::Preview)
        {
			auto& registry = m_Application->GetSceneManager()->GetCurrentScene()->GetRegistry();

			auto cameraController = m_CurrentCamera ? m_CurrentCamera->GetController() : nullptr;
			
			if(cameraController && Application::Get().GetSceneActive())
			{
				const Maths::Vector2 mousePos = Input::GetInput()->GetMousePosition();
				
				cameraController->HandleMouse(m_CurrentCamera, ts.GetMillis(), mousePos.x, mousePos.y);
				cameraController->HandleKeyboard(m_CurrentCamera, ts.GetMillis());
			}
			
			if(Input::GetInput()->GetKeyPressed(InputCode::Key::F))
			{
				if(registry.valid(m_Selected))
				{
					auto transform = registry.try_get<Maths::Transform>(m_Selected);
					if(transform)
						FocusCamera(transform->GetWorldPosition(), 2.0f, 2.0f);
				}
			}
			
			if(Input::GetInput()->GetKeyHeld(InputCode::Key::O))
			{
				FocusCamera(Maths::Vector3(0.0f, 0.0f, 0.0f), 2.0f, 2.0f);
			}
			
			if(m_TransitioningCamera)
			{
				if(m_CameraTransitionStartTime < 0.0f)
					m_CameraTransitionStartTime = ts.GetElapsedMillis();
				
				float focusProgress =
				Maths::Min((ts.GetElapsedMillis() - m_CameraTransitionStartTime) / m_CameraTransitionSpeed, 1.f);
				Maths::Vector3 newCameraPosition = m_CameraStartPosition.Lerp(m_CameraDestination, focusProgress);
				m_CurrentCamera->SetPosition(newCameraPosition);
				
				if(m_CurrentCamera->GetPosition().Equals(m_CameraDestination))
					m_TransitioningCamera = false;
			}
			
			if(!Input::GetInput()->GetMouseHeld(InputCode::MouseKey::ButtonRight))
			{
				if(Input::GetInput()->GetKeyPressed(InputCode::Key::Q))
				{
					SetImGuizmoOperation(4);
				}

				if(Input::GetInput()->GetKeyPressed(InputCode::Key::W))
				{
					SetImGuizmoOperation(0);
				}
				
				if(Input::GetInput()->GetKeyPressed(InputCode::Key::E))
				{
					SetImGuizmoOperation(1);
				}
				
				if(Input::GetInput()->GetKeyPressed(InputCode::Key::R))
				{
					SetImGuizmoOperation(2);
				}
				
				if(Input::GetInput()->GetKeyPressed(InputCode::Key::T))
				{
					SetImGuizmoOperation(3);
				}
				
				if(Input::GetInput()->GetKeyPressed(InputCode::Key::Y))
				{
					ToggleSnap();
				}
			}

			if((Input::GetInput()->GetKeyHeld(InputCode::Key::LeftSuper) || (Input::GetInput()->GetKeyHeld(InputCode::Key::LeftShift)) ) && Input::GetInput()->GetKeyPressed(InputCode::Key::Z))
				Application::Get().GetSceneManager()->GetCurrentScene()->Serialise(ROOT_DIR "/Assets/scenes/", true);
			
			if((Input::GetInput()->GetKeyHeld(InputCode::Key::LeftSuper) || (Input::GetInput()->GetKeyHeld(InputCode::Key::LeftShift)) ) && Input::GetInput()->GetKeyPressed(InputCode::Key::X))
				Application::Get().GetSceneManager()->GetCurrentScene()->Deserialise(ROOT_DIR "/Assets/scenes/", true);
		}
	}

	void Editor::BindEventFunction()
	{
		m_Application->GetWindow()->SetEventCallback(BIND_EVENT_FN(Editor::OnEvent));
	}

	void Editor::FocusCamera(const Maths::Vector3& point, float distance, float speed)
	{
		if(m_CurrentCamera->IsOrthographic())
		{
			m_CurrentCamera->SetPosition(point);
			m_CurrentCamera->SetScale(distance / 2.0f);
		}
		else
		{
			m_TransitioningCamera = true;

			m_CameraDestination = point + m_CurrentCamera->GetForwardDirection() * distance;
			m_CameraTransitionStartTime = -1.0f;
			m_CameraTransitionSpeed = 1.0f / speed;
			m_CameraStartPosition = m_CurrentCamera->GetPosition();
		}
	}

	bool Editor::OnWindowResize(WindowResizeEvent& e)
	{
		return false;
	}

	void Editor::RecompileShaders()
	{
		Lumos::Debug::Log::Info("Recompiling shaders");

#ifdef LUMOS_RENDER_API_VULKAN
#	ifdef LUMOS_PLATFORM_WINDOWS
		// std::string filePath = ROOT_DIR"/Assets/shaders/CompileShadersWindows.bat";
		// system(filePath.c_str());
#	elif LUMOS_PLATFORM_MACOS
		std::string filePath = ROOT_DIR "/Assets/shaders/CompileShadersMac.sh";
		system(filePath.c_str());
#	endif
#endif
	}

	void Editor::DebugDraw()
	{
		auto& registry = Application::Get().GetSceneManager()->GetCurrentScene()->GetRegistry();

		if(m_DebugDrawFlags & EditorDebugFlags::MeshBoundingBoxes)
		{
			auto group = registry.group<MeshComponent>(entt::get<Maths::Transform>);

			for(auto entity : group)
			{
				const auto& [mesh, trans] = group.get<MeshComponent, Maths::Transform>(entity);

				if(mesh.GetMesh() && mesh.GetMesh()->GetActive())
				{
					auto& worldTransform = trans.GetWorldMatrix();

					auto bbCopy = mesh.GetMesh()->GetBoundingBox()->Transformed(worldTransform);
					DebugRenderer::DebugDraw(bbCopy, Maths::Vector4(0.1f, 0.9f, 0.1f, 0.4f), true);
				}
			}
		}

		if(m_DebugDrawFlags & EditorDebugFlags::SpriteBoxes)
		{
			auto group = registry.group<Graphics::Sprite>(entt::get<Maths::Transform>);

			for(auto entity : group)
			{
				const auto& [sprite, trans] = group.get<Graphics::Sprite, Maths::Transform>(entity);

				{
					auto& worldTransform = trans.GetWorldMatrix();

					auto bb =
						Maths::BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetPosition() + sprite.GetScale()));
					bb.Transform(trans.GetWorldMatrix());
					DebugRenderer::DebugDraw(bb, Maths::Vector4(0.1f, 0.9f, 0.1f, 0.4f), true);
				}
			}
		}

		if(m_DebugDrawFlags & EditorDebugFlags::CameraFrustum)
		{
			auto cameraGroup = registry.group<Camera>(entt::get<Maths::Transform>);

			for(auto entity : cameraGroup)
			{
				const auto& [camera, trans] = cameraGroup.get<Camera, Maths::Transform>(entity);

				{
					DebugRenderer::DebugDraw(camera.GetFrustum(), Maths::Vector4(0.9f));
				}
			}
		}

		if(registry.valid(m_Selected))
		{
			auto transform = registry.try_get<Maths::Transform>(m_Selected);

			auto meshComponent = registry.try_get<MeshComponent>(m_Selected);
			if(transform && meshComponent)
			{
				if(meshComponent->GetMesh() && meshComponent->GetMesh()->GetActive())
				{
					auto& worldTransform = transform->GetWorldMatrix();

					auto bbCopy = meshComponent->GetMesh()->GetBoundingBox()->Transformed(worldTransform);
					DebugRenderer::DebugDraw(bbCopy, Maths::Vector4(0.1f, 0.9f, 0.1f, 0.4f), true);
				}
			}

			auto sprite = registry.try_get<Graphics::Sprite>(m_Selected);
			if(transform && sprite)
			{
				{
					auto& worldTransform = transform->GetWorldMatrix();

					auto bb = Maths::BoundingBox(
						Maths::Rect(sprite->GetPosition(), sprite->GetPosition() + sprite->GetScale()));
					bb.Transform(worldTransform);
					DebugRenderer::DebugDraw(bb, Maths::Vector4(0.1f, 0.9f, 0.1f, 0.4f), true);
				}
			}

			auto camera = registry.try_get<Camera>(m_Selected);
			if(camera)
			{
				DebugRenderer::DebugDraw(camera->GetFrustum(), Maths::Vector4(0.9f));
			}

			auto light = registry.try_get<Graphics::Light>(m_Selected);
			if(light)
			{
				DebugRenderer::DebugDraw(light, Maths::Vector4(light->Colour.ToVector3(), 0.2f));
			}

			auto sound = registry.try_get<SoundComponent>(m_Selected);
			if(sound)
			{
				DebugRenderer::DebugDraw(sound->GetSoundNode(), Maths::Vector4(0.8f, 0.8f, 0.8f, 0.2f));
			}
		}
	}

	void Editor::SelectObject(const Maths::Ray& ray)
	{
		auto& registry = Application::Get().GetSceneManager()->GetCurrentScene()->GetRegistry();
		float closestEntityDist = Maths::M_INFINITY;
		entt::entity currentClosestEntity = entt::null;

		auto group = registry.group<MeshComponent>(entt::get<Maths::Transform>);

		static Timer timer;
		static float timeSinceLastSelect = 0.0f;

		for(auto entity : group)
		{
			const auto& [mesh, trans] = group.get<MeshComponent, Maths::Transform>(entity);

			if(mesh.GetMesh() && mesh.GetMesh()->GetActive())
			{
				auto& worldTransform = trans.GetWorldMatrix();

				auto bbCopy = mesh.GetMesh()->GetBoundingBox()->Transformed(worldTransform);
				float dist = ray.HitDistance(bbCopy);

				if(dist < Maths::M_INFINITY)
				{
					if(dist < closestEntityDist)
					{
						closestEntityDist = dist;
						currentClosestEntity = entity;
					}
				}
			}
		}

		if(m_Selected != entt::null)
		{
			if(m_Selected == currentClosestEntity)
			{
				if(timer.GetMS(1.0f) - timeSinceLastSelect < 1.0f)
				{
					auto& trans = registry.get<Maths::Transform>(m_Selected);
					auto& mesh = registry.get<MeshComponent>(m_Selected);
					auto bb = mesh.GetMesh()->GetBoundingBox()->Transformed(trans.GetWorldMatrix());

					FocusCamera(trans.GetWorldPosition(), (bb.max_ - bb.min_).Length());
				}
				else
				{
					currentClosestEntity = entt::null;
				}
			}

			timeSinceLastSelect = timer.GetMS(1.0f);
			m_Selected = currentClosestEntity;
			return;
		}

		auto spriteGroup = registry.group<Graphics::Sprite>(entt::get<Maths::Transform>);

		for(auto entity : spriteGroup)
		{
			const auto& [sprite, trans] = spriteGroup.get<Graphics::Sprite, Maths::Transform>(entity);

			auto& worldTransform = trans.GetWorldMatrix();
			auto bb = Maths::BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetPosition() + sprite.GetScale()));
			bb.Transform(trans.GetWorldMatrix());
			float dist = ray.HitDistance(bb);

			if(dist < Maths::M_INFINITY)
			{
				if(dist < closestEntityDist)
				{
					closestEntityDist = dist;
					currentClosestEntity = entity;
				}
			}
		}

		if(m_Selected != entt::null)
		{
			if(m_Selected == currentClosestEntity)
			{
				auto& trans = registry.get<Maths::Transform>(m_Selected);
				auto& sprite = registry.get<Graphics::Sprite>(m_Selected);
				auto bb =
					Maths::BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetPosition() + sprite.GetScale()));

				FocusCamera(trans.GetWorldPosition(), (bb.max_ - bb.min_).Length());
			}
		}

		m_Selected = currentClosestEntity;
	}

	void Editor::OpenTextFile(const std::string& filePath)
	{
		std::string physicalPath;
		if(!VFS::Get()->ResolvePhysicalPath(filePath, physicalPath))
		{
			Debug::Log::Error("Failed to Load Lua script {0}", filePath);
			return;
		}

		for(int i = 0; i < int(m_Windows.size()); i++)
		{
			EditorWindow* w = m_Windows[i].get();
			if(w->GetSimpleName() == "TextEdit")
			{
				m_Windows.erase(m_Windows.begin() + i);
				break;
			}
		}

		m_Windows.emplace_back(CreateRef<TextEditWindow>(physicalPath));
		m_Windows.back()->SetEditor(this);
	}

	void Editor::RemoveWindow(EditorWindow* window)
	{
		for(int i = 0; i < int(m_Windows.size()); i++)
		{
			EditorWindow* w = m_Windows[i].get();
			if(w == window)
			{
				m_Windows.erase(m_Windows.begin() + i);
				return;
			}
		}
	}

	void Editor::ShowPreview()
	{
		ImGui::Begin("Preview");
		if(m_PreviewTexture)
			ImGuiHelpers::Image(m_PreviewTexture.get(), {200, 200});
		ImGui::End();
	}

	void Editor::OnRender()
	{
		//DrawPreview();

		if(m_Application->GetEditorState() == EditorState::Preview && m_ShowGrid && !m_EditorCamera->IsOrthographic())
			Draw3DGrid();
	}

	void Editor::DrawPreview()
	{
		if(!m_PreviewTexture)
		{
			m_PreviewTexture = Ref<Graphics::Texture2D>(Graphics::Texture2D::Create());
			m_PreviewTexture->BuildTexture(Graphics::TextureFormat::RGBA32, 200, 200, false, false);

			m_PreviewRenderer = CreateRef<Graphics::ForwardRenderer>(200, 200, false);
			m_PreviewSphere = Ref<Graphics::Mesh>(Graphics::CreateSphere());

			m_PreviewRenderer->SetRenderTarget(m_PreviewTexture.get(), true);
		}

		Maths::Matrix4 proj = Maths::Matrix4::Perspective(0.1f, 10.0f, 200.0f / 200.0f, 60.0f);
		Maths::Matrix4 view = Maths::Matrix3x4(Maths::Vector3(0.0f, 0.0f, 3.0f),
			Maths::Quaternion::EulerAnglesToQuaternion(0.0f, 0.0f, 0.0f),
			Maths::Vector3(1.0f))
								  .Inverse()
								  .ToMatrix4();
		m_PreviewRenderer->Begin();
		m_PreviewRenderer->BeginScene(proj, view);
		m_PreviewRenderer->SubmitMesh(m_PreviewSphere.get(), nullptr, Maths::Matrix4(), Maths::Matrix4());
		m_PreviewRenderer->SetSystemUniforms(m_PreviewRenderer->GetShader());
		m_PreviewRenderer->Present();
		m_PreviewRenderer->End();
	}

	void Editor::FileOpenCallback(const std::string& filePath)
	{
		if(IsTextFile(filePath))
			OpenTextFile(filePath);
		else if(IsModelFile(filePath))
		{
			auto entity =
				ModelLoader::LoadModel(filePath, m_Application->GetSceneManager()->GetCurrentScene()->GetRegistry());
			m_Selected = entity;
		}
		else if(IsAudioFile(filePath))
		{
			AssetsManager::Sounds()->LoadAsset(StringFormat::GetFileName(filePath), filePath);

			auto soundNode = Ref<SoundNode>(SoundNode::Create());
			soundNode->SetSound(AssetsManager::Sounds()->Get(StringFormat::GetFileName(filePath)).get());
			soundNode->SetVolume(1.0f);
			soundNode->SetPosition(Maths::Vector3(0.1f, 10.0f, 10.0f));
			soundNode->SetLooping(true);
			soundNode->SetIsGlobal(false);
			soundNode->SetPaused(false);
			soundNode->SetReferenceDistance(1.0f);
			soundNode->SetRadius(30.0f);

			auto& registry = m_Application->GetSceneManager()->GetCurrentScene()->GetRegistry();
			entt::entity e = registry.create();
			registry.emplace<SoundComponent>(e, soundNode);
			m_Selected = e;
		}
	}

	void Editor::SaveEditorSettings()
	{
		m_IniFile.SetOrAdd("ShowGrid", m_ShowGrid);
		m_IniFile.SetOrAdd("ShowGizmos", m_ShowGizmos);
		m_IniFile.SetOrAdd("ShowViewSelected", m_ShowViewSelected);
		m_IniFile.SetOrAdd("TransitioningCamera", m_TransitioningCamera);
		m_IniFile.SetOrAdd("ShowImGuiDemo", m_ShowImGuiDemo);
		m_IniFile.SetOrAdd("SnapAmount", m_SnapAmount);
		m_IniFile.SetOrAdd("SnapQuizmo", m_SnapQuizmo);
		m_IniFile.SetOrAdd("DebugDrawFlags", m_DebugDrawFlags);
		// m_IniFile.SetOrAdd("PhysicsDebugDrawFlags",
		// Application::Get().GetSystem<LumosPhysicsEngine>()->GetDebugDrawFlags());
		// m_IniFile.SetOrAdd("PhysicsDebugDrawFlags2D",
		// Application::Get().GetSystem<B2PhysicsEngine>()->GetDebugDrawFlags());
		m_IniFile.Rewrite();
	}

	void Editor::AddDefaultEditorSettings()
	{
		m_IniFile.Add("ShowGrid", m_ShowGrid);
		m_IniFile.Add("ShowGizmos", m_ShowGizmos);
		m_IniFile.Add("ShowViewSelected", m_ShowViewSelected);
		m_IniFile.Add("TransitioningCamera", m_TransitioningCamera);
		m_IniFile.Add("ShowImGuiDemo", m_ShowImGuiDemo);
		m_IniFile.Add("SnapAmount", m_SnapAmount);
		m_IniFile.Add("SnapQuizmo", m_SnapQuizmo);
		m_IniFile.Add("DebugDrawFlags", m_DebugDrawFlags);
		// m_IniFile.Add("PhysicsDebugDrawFlags",
		// Application::Get().GetSystem<LumosPhysicsEngine>()->GetDebugDrawFlags());
		// m_IniFile.Set("PhysicsDebugDrawFlags2D",
		// Application::Get().GetSystem<B2PhysicsEngine>()->GetDebugDrawFlags());
		m_IniFile.Rewrite();
	}

	void Editor::LoadEditorSettings()
	{
		m_ShowGrid = m_IniFile.GetOrDefault("ShowGrid", m_ShowGrid);
		m_ShowGizmos = m_IniFile.GetOrDefault("ShowGizmos", m_ShowGizmos);
		m_ShowViewSelected = m_IniFile.GetOrDefault("ShowViewSelected", m_ShowViewSelected);
		m_TransitioningCamera = m_IniFile.GetOrDefault("TransitioningCamera", m_TransitioningCamera);
		m_ShowImGuiDemo = m_IniFile.GetOrDefault("ShowImGuiDemo", m_ShowImGuiDemo);
		m_SnapAmount = m_IniFile.GetOrDefault("SnapAmount", m_SnapAmount);
		m_SnapQuizmo = m_IniFile.GetOrDefault("SnapQuizmo", m_SnapQuizmo);
		m_DebugDrawFlags = m_IniFile.GetOrDefault("DebugDrawFlags", m_DebugDrawFlags);
		//        Application::Get().GetSystem<LumosPhysicsEngine>()->SetDebugDrawFlags(m_IniFile.GetOrDefault("PhysicsDebugDrawFlags",
		//        0));
		//        Application::Get().GetSystem<B2PhysicsEngine>()->SetDebugDrawFlags(m_IniFile.GetOrDefault("PhysicsDebugDrawFlags2D",
		//        0));
	}

	const char* Editor::GetIconFontIcon(const std::string& filePath)
	{ 
		if(IsTextFile(filePath))
		{
			return ICON_MDI_FILE_XML;
		}
		else if(IsModelFile(filePath))
		{
			return ICON_MDI_SHAPE;
		}
		else if(IsAudioFile(filePath))
		{
			return ICON_MDI_FILE_MUSIC;
		}

		return ICON_MDI_FILE;
	}

	void Editor::CreateGridRenderer()
	{
		if(!m_GridRenderer)
			m_GridRenderer = CreateRef<Graphics::GridRenderer>(u32(Application::Get().m_SceneViewWidth), u32(Application::Get().m_SceneViewHeight), true);
	}

	const Ref<Graphics::GridRenderer>& Editor::GetGridRenderer()
	{
		//if(!m_GridRenderer)
		//  m_GridRenderer = CreateRef<Graphics::GridRenderer>(u32(Application::Get().m_SceneViewWidth), u32(Application::Get().m_SceneViewHeight), true);
		return m_GridRenderer;
	}

}
