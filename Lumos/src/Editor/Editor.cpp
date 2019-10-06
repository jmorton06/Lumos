#include "lmpch.h"
#include "Editor.h"
#include "ImGUIConsoleSink.h"
#include "SceneWindow.h"
#include "ProfilerWindow.h"
#include "ConsoleWindow.h"
#include "HierarchyWindow.h"
#include "InspectorWindow.h"
#include "ApplicationInfoWindow.h"
#include "GraphicsInfoWindow.h"

#include "App/Application.h"
#include "Core/OS/Input.h"
#include "Core/Profiler.h"
#include "App/Engine.h"
#include "App/Scene.h"
#include "App/SceneManager.h"

#include "Maths/BoundingSphere.h"
#include "ECS/EntityManager.h"
#include "ECS/Component/Components.h"
#include "ECS/SystemManager.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"

#include "Graphics/GBuffer.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/RenderManager.h"
#include "Graphics/Layers/LayerStack.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/API/Texture.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/MeshFactory.h"

#include <imgui/imgui_internal.h>
#include <imgui/plugins/ImGuizmo.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>

#ifdef LUMOS_PLATFORM_WINDOWS
#include <imgui/plugins/ImFileBrowser.h"
#endif

static ImVec2 operator+(const ImVec2 &a, const ImVec2 &b) {
	return ImVec2(a.x + b.x, a.y + b.y);
}

static ImVec2 operator-(const ImVec2 &a, const ImVec2 &b) {
	return ImVec2(a.x - b.x, a.y - b.y);
}

namespace Lumos
{
	Editor::Editor(Application* app, u32 width, u32 height) : m_Application(app)
	{
		m_Windows.emplace_back(CreateRef<ConsoleWindow>());
		m_Windows.emplace_back(CreateRef<SceneWindow>());
		m_Windows.emplace_back(CreateRef<ProfilerWindow>());
		m_Windows.emplace_back(CreateRef<InspectorWindow>());
		m_Windows.emplace_back(CreateRef<HierarchyWindow>());
		m_Windows.emplace_back(CreateRef<GraphicsInfoWindow>());
		m_Windows.back()->SetActive(false);
		m_Windows.emplace_back(CreateRef<ApplicationInfoWindow>());
		m_Windows.back()->SetActive(false);
		for (auto& window : m_Windows)
			window->SetEditor(this);
        
#ifdef LUMOS_PLATFORM_WINDOWS
		m_FileBrowser = lmnew ImGui::FileBrowser(ImGuiFileBrowserFlags_CreateNewDir | ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_NoModal);
        m_FileBrowser->SetTitle("Test File Browser");
		m_FileBrowser->SetFileFilters({ ".sh" , ".h" });
		m_FileBrowser->SetLabels(ICON_FA_FOLDER, ICON_FA_FILE_ALT, ICON_FA_FOLDER_PLUS);
		m_FileBrowser->Refresh();
#endif

	}

	Editor::~Editor()
	{
	}

	void Editor::OnImGui()
	{
		DrawMenuBar();
		DrawInfoBar();

		BeginDockSpace(true);
        EndDockSpace();

		for (auto& window : m_Windows)
		{
			if (window->Active())
				window->OnImGui();
		}

        if(m_ShowImGuiDemo)
            ImGui::ShowDemoWindow(&m_ShowImGuiDemo);
	}

	void Editor::DrawMenuBar()
	{
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Exit")) { Application::Instance()->SetAppState(AppState::Closing); }
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
				if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
				ImGui::Separator();
				if (ImGui::MenuItem("Cut", "CTRL+X")) {}
				if (ImGui::MenuItem("Copy", "CTRL+C")) {}
				if (ImGui::MenuItem("Paste", "CTRL+V")) {}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Windows"))
			{
				for (auto& window : m_Windows)
				{
					if (ImGui::MenuItem(window->GetName().c_str(), "", &window->Active(), true)) { window->SetActive(true); }
				}

				ImGui::EndMenu();
			}
            
            if (ImGui::BeginMenu("Scenes"))
            {
                auto scenes = Application::Instance()->GetSceneManager()->GetSceneNames();
                
                for(size_t i = 0; i < scenes.size(); i++)
                {
                    auto name = scenes[i];
                    if (ImGui::MenuItem(name.c_str()))
                    {
                        Application::Instance()->GetSceneManager()->SwitchScene(name);
                    }
                }
                ImGui::EndMenu();
            }

			if (ImGui::BeginMenu("Entity"))
			{
				if (ImGui::MenuItem("CreateEmpty"))
				{
					Application::Instance()->GetSceneManager()->GetCurrentScene()->AddEntity(EntityManager::Instance()->CreateEntity());
				}

				if (ImGui::MenuItem("Cube"))
				{
					auto entity = EntityManager::Instance()->CreateEntity();
					entity->AddComponent<MeshComponent>(Graphics::CreatePrimative(Graphics::PrimitiveType::Cube));
					Application::Instance()->GetSceneManager()->GetCurrentScene()->AddEntity(entity);
				}

				if (ImGui::MenuItem("Sphere"))
				{
					auto entity = EntityManager::Instance()->CreateEntity();
					entity->AddComponent<MeshComponent>(Graphics::CreatePrimative(Graphics::PrimitiveType::Sphere));
					Application::Instance()->GetSceneManager()->GetCurrentScene()->AddEntity(entity);
				}

				if (ImGui::MenuItem("Pyramid"))
				{
					auto entity = EntityManager::Instance()->CreateEntity();
					entity->AddComponent<MeshComponent>(Graphics::CreatePrimative(Graphics::PrimitiveType::Pyramid));
					Application::Instance()->GetSceneManager()->GetCurrentScene()->AddEntity(entity);
				}

				if (ImGui::MenuItem("Plane"))
				{
					auto entity = EntityManager::Instance()->CreateEntity();
					entity->AddComponent<MeshComponent>(Graphics::CreatePrimative(Graphics::PrimitiveType::Plane));
					Application::Instance()->GetSceneManager()->GetCurrentScene()->AddEntity(entity);
				}
				
				if (ImGui::MenuItem("Cylinder"))
				{
					auto entity = EntityManager::Instance()->CreateEntity();
					entity->AddComponent<MeshComponent>(Graphics::CreatePrimative(Graphics::PrimitiveType::Cylinder));
					Application::Instance()->GetSceneManager()->GetCurrentScene()->AddEntity(entity);
				}

				if (ImGui::MenuItem("Capsule"))
				{
					auto entity = EntityManager::Instance()->CreateEntity();
					entity->AddComponent<MeshComponent>(Graphics::CreatePrimative(Graphics::PrimitiveType::Capsule));
					Application::Instance()->GetSceneManager()->GetCurrentScene()->AddEntity(entity);
				}

				ImGui::EndMenu();
			}

			ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 20.0f);
			if (ImGui::Button(ICON_FA_TIMES, ImVec2(19.0f, 19.0f)))
				Application::Instance()->SetAppState(AppState::Closing);

			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::TextUnformatted("Exit");
				ImGui::EndTooltip();
			}

			ImGui::EndMainMenuBar();
		}
	}
    
	void Editor::OnImGuizmo()
	{
		if (!m_Selected)
			return;

		Maths::Matrix4 view = Application::Instance()->GetSceneManager()->GetCurrentScene()->GetCamera()->GetViewMatrix();
		Maths::Matrix4 proj = Application::Instance()->GetSceneManager()->GetCurrentScene()->GetCamera()->GetProjectionMatrix();

#ifdef LUMOS_RENDER_API_VULKAN
		if (Graphics::GraphicsContext::GetRenderAPI() == Graphics::RenderAPI::VULKAN)
			proj[5] *= -1.0f;
#endif
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetOrthographic(Application::Instance()->GetSceneManager()->GetCurrentScene()->GetCamera()->Is2D());

		if (m_Selected->GetComponent<Maths::Transform>() != nullptr)
		{
			Maths::Matrix4 model = m_Selected->GetComponent<Maths::Transform>()->GetWorldMatrix();

			float snapAmount[3] = { m_SnapAmount  , m_SnapAmount , m_SnapAmount };
			float delta[16];
			ImGuizmo::Manipulate(view.values, proj.values, static_cast<ImGuizmo::OPERATION>(m_ImGuizmoOperation), ImGuizmo::LOCAL, model.values, delta, m_SnapQuizmo ? snapAmount : nullptr);

			if (ImGuizmo::IsUsing())
			{
				auto mat = Maths::Matrix4(delta) * m_Selected->GetComponent<Maths::Transform>()->GetLocalMatrix();
				m_Selected->GetComponent<Maths::Transform>()->SetLocalTransform(mat);

				auto physics2DComponent = m_Selected->GetComponent<Physics2DComponent>();

				if (physics2DComponent)
				{
					physics2DComponent->GetPhysicsObject()->SetPosition({ mat.GetPositionVector().GetX(), mat.GetPositionVector().GetY() });
				}
				else
				{
					auto physics3DComponent = m_Selected->GetComponent<Physics3DComponent>();
					if (physics3DComponent)
					{
						physics3DComponent->GetPhysicsObject()->SetPosition(mat.GetPositionVector());
						physics3DComponent->GetPhysicsObject()->SetOrientation(mat.GetRotation().ToQuaternion());
					}
					
				}
			}
		}
	}

	void Editor::BeginDockSpace(bool infoBar)
	{
        static bool p_open = true;
        static bool opt_fullscreen_persistant = true;
        static ImGuiDockNodeFlags opt_flags = ImGuiDockNodeFlags_None;
        bool opt_fullscreen = opt_fullscreen_persistant;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen)
        {
            ImGuiViewport* viewport = ImGui::GetMainViewport();

            auto pos = viewport->Pos;
            auto size = viewport->Size;
    
            if (infoBar)
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
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }
        
        // When using ImGuiDockNodeFlags_PassthruDockspace, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
        if (opt_flags & ImGuiDockNodeFlags_DockSpace)
            window_flags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("MyDockspace", &p_open, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

		if (ImGui::DockBuilderGetNode(ImGui::GetID("MyDockspace")) == nullptr)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
			ImGui::DockBuilderRemoveNode(dockspace_id);
			ImGui::DockBuilderAddNode(dockspace_id);
			ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetIO().DisplaySize);

			ImGuiID dock_main_id   = dockspace_id;
			ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.2f, nullptr, &dock_main_id);
			ImGuiID dock_id_left   = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);
            ImGuiID dock_id_right  = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.3f, nullptr, &dock_main_id);
			ImGuiID dock_id_middle = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.8f, nullptr, &dock_main_id);

			ImGui::DockBuilderDockWindow("###scene", dock_id_middle);
			ImGui::DockBuilderDockWindow("###inspector", dock_id_right);
			ImGui::DockBuilderDockWindow("###hierarchy", dock_id_left);
			ImGui::DockBuilderDockWindow("###console", dock_id_bottom);
            ImGui::DockBuilderDockWindow("###profiler", dock_id_bottom);
			ImGui::DockBuilderDockWindow("Dear ImGui Demo", dock_id_left);
            ImGui::DockBuilderDockWindow("GraphicsInfo", dock_id_left);
			ImGui::DockBuilderDockWindow("ApplicationInfo", dock_id_left);
			ImGui::DockBuilderFinish(dockspace_id);
		}
		
		  // Dockspace
          ImGuiIO& io = ImGui::GetIO();
          if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
          {
              ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
              ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), opt_flags);
          }
	}

	void Editor::EndDockSpace()
	{
		ImGui::End();
	}

	void Editor::OnInit()
	{
	}

	void Editor::OnNewScene(Scene * scene)
	{
		m_Selected = nullptr;
	}
   

	void Editor::DrawInfoBar()
	{
		auto windowFlags
			= ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoBringToFrontOnFocus
			| ImGuiWindowFlags_NoNavFocus
			| ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoScrollbar
			//| ImGuiWindowFlags_NoBackground
			| ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoScrollWithMouse;
            //| ImGuiWindowFlags_MenuBar;

		ImGuiViewport* viewport = ImGui::GetMainViewport();

		auto pos = viewport->Pos;
		auto size = viewport->Size;

		size.y = 24.0f;
		pos.y += 20.0f;
		ImGui::SetNextWindowPos(pos);
		ImGui::SetNextWindowSize(size);

		ImGui::Begin("InfoBar", nullptr, windowFlags);
		{
            ImGui::Indent();
			bool selected = false;
			{
				selected = m_ImGuizmoOperation == ImGuizmo::TRANSLATE;
				if (selected)
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));

				if (ImGui::Button(ICON_FA_ARROWS_ALT, ImVec2(19.0f, 19.0f)))
					m_ImGuizmoOperation = ImGuizmo::TRANSLATE;

				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::TextUnformatted("Translate");
					ImGui::EndTooltip();
				}

				if (selected)
					ImGui::PopStyleColor();
			}

			{
				selected = m_ImGuizmoOperation == ImGuizmo::ROTATE;
				if (selected)
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));

				ImGui::SameLine();
				if (ImGui::Button(ICON_FA_SYNC, ImVec2(19.0f, 19.0f)))
					m_ImGuizmoOperation = ImGuizmo::ROTATE;

				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::TextUnformatted("Rotate");
					ImGui::EndTooltip();
				}

				if (selected)
					ImGui::PopStyleColor();
			}

			{
				selected = m_ImGuizmoOperation == ImGuizmo::SCALE;
				if (selected)
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));

				ImGui::SameLine();
				if (ImGui::Button(ICON_FA_EXPAND_ARROWS_ALT, ImVec2(19.0f, 19.0f)))
					m_ImGuizmoOperation = ImGuizmo::SCALE;

				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::TextUnformatted("Scale");
					ImGui::EndTooltip();
				}

				if (selected)
					ImGui::PopStyleColor();
			}

			ImGui::SameLine(ImGui::GetWindowContentRegionMax().x / 2.0f);

			{
				selected = m_Application->GetEditorState() == EditorState::Play;
				if (selected)
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.2f, 0.7f, 1.0f));

				if (ImGui::Button(ICON_FA_PLAY, ImVec2(19.0f, 19.0f)))
					m_Application->SetEditorState(EditorState::Play);

				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::TextUnformatted("Play");
					ImGui::EndTooltip();
				}

				if (selected)
					ImGui::PopStyleColor();
			}

			ImGui::SameLine();

			{
				selected = m_Application->GetEditorState() == EditorState::Paused;
				if (selected)
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.2f, 0.7f, 1.0f));

				if (ImGui::Button(ICON_FA_PAUSE, ImVec2(19.0f, 19.0f)))
					m_Application->SetEditorState(EditorState::Paused);

				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::TextUnformatted("Pause");
					ImGui::EndTooltip();
				}

				if (selected)
					ImGui::PopStyleColor();
			}

			ImGui::SameLine();

			{
				selected = m_Application->GetEditorState() == EditorState::Next;
				if (selected)
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.2f, 0.7f, 1.0f));

				if (ImGui::Button(ICON_FA_STEP_FORWARD, ImVec2(19.0f, 19.0f)))
					m_Application->SetEditorState(EditorState::Next);

				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::TextUnformatted("Next");
					ImGui::EndTooltip();
				}

				if (selected)
					ImGui::PopStyleColor();
			}
            
            ImGui::SameLine();
            
#ifdef LUMOS_PLATFORM_WINDOWS

            if(ImGui::Button("open file dialog"))
                m_FileBrowser->Open();
            
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->Pos + ImVec2(viewport->Size.x * 0.5f, viewport->Size.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            m_FileBrowser->Display();
            
            if(m_FileBrowser->HasSelected())
            {
                std::cout << "Selected filename" << m_FileBrowser->GetSelected().string() << std::endl;
                m_FileBrowser->ClearSelected();
            }
#endif
			ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 130.0f);
			ImGui::Text("%.2f ms (%i FPS)", 1000.0f / (float)Engine::Instance()->GetFPS(), Engine::Instance()->GetFPS());
		}
		ImGui::End();
	}

	void Editor::Draw2DGrid(ImDrawList* drawList, const ImVec2& cameraPos, const ImVec2& windowPos, const ImVec2& canvasSize, const float factor, const float thickness)
	{	
		static const auto graduation = 10;
		float GRID_SZ = canvasSize.y * 0.5f / factor;
		const ImVec2& offset = { canvasSize.x * 0.5f - cameraPos.x * GRID_SZ, canvasSize.y * 0.5f + cameraPos.y * GRID_SZ };

		ImU32 GRID_COLOR = IM_COL32(200, 200, 200, 40);
		float gridThickness = 1.0f;

		const auto& gridColor = GRID_COLOR;
		auto smallGraduation = GRID_SZ / graduation;
		const auto& smallGridColor = IM_COL32(100, 100, 100, smallGraduation);

		for (float x = -GRID_SZ; x < canvasSize.x + GRID_SZ; x += GRID_SZ)
		{
			auto localX = floorf(x + fmodf(offset.x, GRID_SZ));
			drawList->AddLine(ImVec2{ localX, 0.0f } + windowPos, ImVec2{ localX, canvasSize.y } +windowPos, gridColor, gridThickness);

			if (smallGraduation > 5.0f)
			{
				for (int i = 1; i < graduation; ++i)
				{
					const auto graduation = floorf(localX + smallGraduation * i);
					drawList->AddLine(ImVec2{ graduation, 0.0f } +windowPos, ImVec2{ graduation, canvasSize.y } +windowPos, smallGridColor, 1.0f);
				}
			}
			
		}

		for (float y = -GRID_SZ; y < canvasSize.y + GRID_SZ; y += GRID_SZ)
		{
			auto localY = floorf(y + fmodf(offset.y, GRID_SZ));
			drawList->AddLine(ImVec2{ 0.0f, localY } +windowPos, ImVec2{ canvasSize.x, localY } +windowPos, gridColor, gridThickness);

			if (smallGraduation > 5.0f)
			{
				for (int i = 1; i < graduation; ++i)
				{
					const auto graduation = floorf(localY + smallGraduation * i);
					drawList->AddLine(ImVec2{ 0.0f, graduation } +windowPos, ImVec2{ canvasSize.x, graduation } +windowPos, smallGridColor,1.0f);
				}
			}
		}
	}
}
