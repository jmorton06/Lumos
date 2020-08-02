#include "lmpch.h"
#include "SceneWindow.h"
#include "Editor.h"
#include "Graphics/Camera/Camera.h"
#include "Core/Application.h"
#include "Scene/SceneManager.h"
#include "Core/Engine.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/API/Texture.h"
#include "Graphics/RenderManager.h"
#include "Graphics/GBuffer.h"
#include "Graphics/Light.h"
#include "Scene/Component/SoundComponent.h"
#include "Graphics/Layers/LayerStack.h"
#include "Graphics/Renderers/GridRenderer.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Physics/B2PhysicsEngine/B2PhysicsEngine.h"
#include "Core/OS/Input.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "EditorCamera.h"
#include <box2d/box2d.h>
#include <imgui/imgui_internal.h>
#include <imgui/plugins/ImGuizmo.h>
#include <IconFontCppHeaders/IconsMaterialDesignIcons.h>

namespace Lumos
{
	SceneWindow::SceneWindow()
	{
		m_Name = ICON_MDI_GAMEPAD_VARIANT " Scene###scene";
		m_SimpleName = "Scene";
		m_CurrentScene = nullptr;

		m_ShowComponentGizmoMap[typeid(Graphics::Light).hash_code()] = true;
		m_ShowComponentGizmoMap[typeid(Camera).hash_code()] = true;
		m_ShowComponentGizmoMap[typeid(SoundComponent).hash_code()] = true;

		//TODO : Get from editors saved settings
		m_AspectRatio = 1.0;
		m_Width = 1280;
		m_Height = 800;
	}

	void SceneWindow::OnImGui()
	{
		Application& app = Application::Get();
		auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
		ImGui::SetNextWindowBgAlpha(0.0f);
		ImGui::Begin(m_Name.c_str(), &m_Active, flags);

		Camera* camera = nullptr;
        Maths::Transform* transform = nullptr;

		bool gameView = false;

		if(app.GetEditorState() == EditorState::Preview)
		{
			camera = m_Editor->GetCamera();
			transform = &m_Editor->GetEditorCameraTransform();

			auto sceneLayers = app.GetSceneLayers();
			for(auto layer : *sceneLayers)
			{
				layer->SetOverrideCamera(camera, transform);
			}

			DebugRenderer::SetOverrideCamera(camera, transform);
		}
		else
		{
			gameView = true;
			auto sceneLayers = app.GetSceneLayers();
			for(auto layer : *sceneLayers)
			{
				layer->SetOverrideCamera(nullptr, nullptr);
			}

			DebugRenderer::SetOverrideCamera(nullptr, nullptr);

			auto& registry = m_CurrentScene->GetRegistry();
			auto cameraView = registry.view<Camera>();
			if(!cameraView.empty())
			{
				camera = &registry.get<Camera>(cameraView.front());
			}
		}

		if(!gameView)
			ToolBar();

		if(!camera)
			return;

		ImGuizmo::SetDrawlist();
		auto sceneViewSize = ImGui::GetContentRegionAvail();
		auto sceneViewPosition = ImGui::GetWindowPos();

		auto viewportOffset = ImGui::GetCursorPos();

		sceneViewPosition.x = viewportOffset.x + ImGui::GetWindowPos().x;
		sceneViewPosition.y = viewportOffset.y + ImGui::GetWindowPos().y;

		m_Editor->m_SceneWindowPos = {sceneViewPosition.x, sceneViewPosition.y};

		sceneViewSize.x = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
		sceneViewSize.y = ImGui::GetWindowContentRegionMax().y - ImGui::GetWindowContentRegionMin().y;

		sceneViewSize.x -= static_cast<int>(sceneViewSize.x) % 2 != 0 ? 1.0f : 0.0f;
		sceneViewSize.y -= static_cast<int>(sceneViewSize.y) % 2 != 0 ? 1.0f : 0.0f;
    
    
        if(sceneViewSize.x == 0.0f || sceneViewSize.y == 0.0f)
        {
            sceneViewSize = ImVec2(80.0f, 60.0f);
        }

		float aspect = static_cast<float>(sceneViewSize.x) / static_cast<float>(sceneViewSize.y);

		if(!Maths::Equals(aspect, camera->GetAspectRatio()))
		{
			camera->SetAspectRatio(aspect);
		}

		Resize(static_cast<u32>(sceneViewSize.x), static_cast<u32>(sceneViewSize.y));

		ImGuiHelpers::Image(m_GameViewTexture.get(), {sceneViewSize.x, sceneViewSize.y});

		auto windowSize = ImGui::GetWindowSize();
		ImVec2 minBound = sceneViewPosition;
		minBound.x += viewportOffset.x;
		minBound.y += viewportOffset.y;

		ImVec2 maxBound = {minBound.x + windowSize.x, minBound.y + windowSize.y};
		bool updateCamera = ImGui::IsMouseHoveringRect(minBound, maxBound);

		app.SetSceneActive(ImGui::IsWindowFocused() && !ImGuizmo::IsUsing() && updateCamera);

		if(gameView)
		{
			ImGui::End();
			return;
		}

		ImGuizmo::SetRect(sceneViewPosition.x, sceneViewPosition.y, sceneViewSize.x, sceneViewSize.y);

		if(m_Editor->ShowGrid())
		{
			if(camera->IsOrthographic())
			{
				m_Editor->Draw2DGrid(ImGui::GetWindowDrawList(), { transform->GetWorldPosition().x, transform->GetWorldPosition().y}, sceneViewPosition, {sceneViewSize.x, sceneViewSize.y}, 1.0f, 1.5f);
			}
		}

		m_Editor->OnImGuizmo();

		if(!gameView && app.GetSceneActive() && !ImGuizmo::IsUsing() && Input::GetInput()->GetMouseClicked(InputCode::MouseKey::ButtonLeft))
		{
			auto clickPos = Input::GetInput()->GetMousePosition() - Maths::Vector2(sceneViewPosition.x, sceneViewPosition.y);
			m_Editor->SelectObject(m_Editor->GetScreenRay(int(clickPos.x), int(clickPos.y), camera, int(sceneViewSize.x), int(sceneViewSize.y)));
		}

		DrawGizmos(sceneViewSize.x, sceneViewSize.y, 0.0f, 40.0f, app.GetSceneManager()->GetCurrentScene()); // Not sure why 40

		if(m_ShowStats && ImGui::IsWindowFocused())
		{
			static bool p_open = true;
			const float DISTANCE = 5.0f;
			static int corner = 0;

			if(corner != -1)
			{
				ImVec2 window_pos = ImVec2((corner & 1) ? (sceneViewPosition.x + sceneViewSize.x - DISTANCE) : (sceneViewPosition.x + DISTANCE), (corner & 2) ? (sceneViewPosition.y + sceneViewSize.y - DISTANCE) : (sceneViewPosition.y + DISTANCE));
				ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
				ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
			}

			ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
			if(ImGui::Begin("Example: Simple overlay", &p_open, (corner != -1 ? ImGuiWindowFlags_NoMove : 0) | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
			{
				ImGui::Text("%.2f ms (%i FPS)", 1000.0f / (float)Engine::Get().GetFPS(), Engine::Get().GetFPS());
				ImGui::Separator();
				ImGuiIO& io = ImGui::GetIO();
				if(ImGui::IsMousePosValid())
					ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
				else
					ImGui::TextUnformatted("Mouse Position: <invalid>");
				if(ImGui::BeginPopupContextWindow())
				{
					if(ImGui::MenuItem("Custom", NULL, corner == -1))
						corner = -1;
					if(ImGui::MenuItem("Top-left", NULL, corner == 0))
						corner = 0;
					if(ImGui::MenuItem("Top-right", NULL, corner == 1))
						corner = 1;
					if(ImGui::MenuItem("Bottom-left", NULL, corner == 2))
						corner = 2;
					if(ImGui::MenuItem("Bottom-right", NULL, corner == 3))
						corner = 3;
					if(p_open && ImGui::MenuItem("Close"))
						p_open = false;
					ImGui::EndPopup();
				}
			}
			ImGui::End();
		}

		const ImGuiPayload* payload = ImGui::GetDragDropPayload();

		if(ImGui::BeginDragDropTarget())
		{
			auto data = ImGui::AcceptDragDropPayload("selectable", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
			if(data)
			{
				std::string file = (char*)data->Data;
				m_Editor->FileOpenCallback(file);
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::End();
	}

	void SceneWindow::DrawGizmos(float width, float height, float xpos, float ypos, Scene* scene)
	{
		Camera* camera = m_Editor->GetCamera();
        Maths::Transform& cameraTransform = m_Editor->GetEditorCameraTransform();
		auto& registry = scene->GetRegistry();
		Maths::Matrix4 view = cameraTransform.GetWorldMatrix().Inverse();
		Maths::Matrix4 proj = camera->GetProjectionMatrix();

#ifdef LUMOS_RENDER_API_VULKAN
		if(Graphics::GraphicsContext::GetRenderAPI() == Graphics::RenderAPI::VULKAN)
			proj.m11_ *= -1.0f;
#endif

		Maths::Matrix4 viewProj = proj * view;
		const Maths::Frustum& f = camera->GetFrustum(view);

		ShowComponentGizmo<Graphics::Light>(width, height, xpos, ypos, viewProj, f, registry);
        ShowComponentGizmo<Camera>(width, height, xpos, ypos, viewProj, f, registry);
		ShowComponentGizmo<SoundComponent>(width, height, xpos, ypos, viewProj, f, registry);
	}

	void SceneWindow::ToolBar()
	{
		ImGui::Indent();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));
		bool selected = false;

		{
			selected = m_Editor->GetImGuizmoOperation() == 4;
			if(selected)
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.9f, 1.0f));
			ImGui::SameLine();
			if(ImGui::Button(ICON_MDI_CURSOR_DEFAULT, ImVec2(19.0f, 19.0f)))
				m_Editor->SetImGuizmoOperation(4);

			if(selected)
				ImGui::PopStyleColor();
			ImGuiHelpers::Tooltip("Select");
		}

		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();

		{
			selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::TRANSLATE;
			if(selected)
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.9f, 1.0f));
			ImGui::SameLine();
			if(ImGui::Button(ICON_MDI_ARROW_ALL, ImVec2(19.0f, 19.0f)))
				m_Editor->SetImGuizmoOperation(ImGuizmo::TRANSLATE);

			if(selected)
				ImGui::PopStyleColor();
			ImGuiHelpers::Tooltip("Translate");
		}

		{
			selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::ROTATE;
			if(selected)
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.9f, 1.0f));

			ImGui::SameLine();
			if(ImGui::Button(ICON_MDI_ROTATE_ORBIT, ImVec2(19.0f, 19.0f)))
				m_Editor->SetImGuizmoOperation(ImGuizmo::ROTATE);

			if(selected)
				ImGui::PopStyleColor();
			ImGuiHelpers::Tooltip("Rotate");
		}

		{
			selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::SCALE;
			if(selected)
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.9f, 1.0f));

			ImGui::SameLine();
			if(ImGui::Button(ICON_MDI_ARROW_EXPAND_ALL, ImVec2(19.0f, 19.0f)))
				m_Editor->SetImGuizmoOperation(ImGuizmo::SCALE);

			if(selected)
				ImGui::PopStyleColor();
			ImGuiHelpers::Tooltip("Scale");
		}

		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();

		{
			selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::BOUNDS;
			if(selected)
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.9f, 1.0f));

			ImGui::SameLine();
			if(ImGui::Button(ICON_MDI_BORDER_NONE, ImVec2(19.0f, 19.0f)))
				m_Editor->SetImGuizmoOperation(ImGuizmo::BOUNDS);

			if(selected)
				ImGui::PopStyleColor();
			ImGuiHelpers::Tooltip("Bounds");
		}

		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();

		ImGui::SameLine();
		{
			selected = (m_Editor->SnapGuizmo() == true);

			if(selected)
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.9f, 1.0f));

			if(ImGui::Button(ICON_MDI_MAGNET, ImVec2(19.0f, 19.0f)))
				m_Editor->SnapGuizmo() = !selected;

			if(selected)
				ImGui::PopStyleColor();
			ImGuiHelpers::Tooltip("Snap");
		}

		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();

		if(ImGui::Button("Gizmos " ICON_MDI_CHEVRON_DOWN))
			ImGui::OpenPopup("GizmosPopup");
		if(ImGui::BeginPopup("GizmosPopup"))
		{
			{
				ImGui::Checkbox("Grid", &m_Editor->ShowGrid());
				ImGui::Checkbox("Selected Gizmos", &m_Editor->ShowGizmos());
				ImGui::Checkbox("View Selected", &m_Editor->ShowViewSelected());

				ImGui::Separator();
				ImGui::Checkbox("Camera", &m_ShowComponentGizmoMap[typeid(Camera).hash_code()]);
				ImGui::Checkbox("Light", &m_ShowComponentGizmoMap[typeid(Graphics::Light).hash_code()]);
				ImGui::Checkbox("Audio", &m_ShowComponentGizmoMap[typeid(SoundComponent).hash_code()]);

				ImGui::Separator();

				auto physics2D = Application::Get().GetSystem<B2PhysicsEngine>();

				if(physics2D)
				{
					u32 flags = physics2D->GetDebugDrawFlags();

					bool show2DShapes = flags & b2Draw::e_shapeBit;
					if(ImGui::Checkbox("Shapes (2D)", &show2DShapes))
					{
						if(show2DShapes)
							flags += b2Draw::e_shapeBit;
						else
							flags -= b2Draw::e_shapeBit;
					}

					bool showCOG = flags & b2Draw::e_centerOfMassBit;
					if(ImGui::Checkbox("Centre of Mass (2D)", &showCOG))
					{
						if(showCOG)
							flags += b2Draw::e_centerOfMassBit;
						else
							flags -= b2Draw::e_centerOfMassBit;
					}

					bool showJoint = flags & b2Draw::e_jointBit;
					if(ImGui::Checkbox("Joint Connection (2D)", &showJoint))
					{
						if(showJoint)
							flags += b2Draw::e_jointBit;
						else
							flags -= b2Draw::e_jointBit;
					}

					bool showAABB = flags & b2Draw::e_aabbBit;
					if(ImGui::Checkbox("AABB (2D)", &showAABB))
					{
						if(showAABB)
							flags += b2Draw::e_aabbBit;
						else
							flags -= b2Draw::e_aabbBit;
					}

					bool showPairs = static_cast<bool>(flags & b2Draw::e_pairBit);
					if(ImGui::Checkbox("Broadphase Pairs  (2D)", &showPairs))
					{
						if(showPairs)
							flags += b2Draw::e_pairBit;
						else
							flags -= b2Draw::e_pairBit;
					}

					physics2D->SetDebugDrawFlags(flags);
				}

				auto physics3D = Application::Get().GetSystem<LumosPhysicsEngine>();

				if(physics3D)
				{
					u32 flags = physics3D->GetDebugDrawFlags();

					bool showCollisionShapes = flags & PhysicsDebugFlags::COLLISIONVOLUMES;
					if(ImGui::Checkbox("Collision Volumes", &showCollisionShapes))
					{
						if(showCollisionShapes)
							flags += PhysicsDebugFlags::COLLISIONVOLUMES;
						else
							flags -= PhysicsDebugFlags::COLLISIONVOLUMES;
					}

					bool showConstraints = static_cast<bool>(flags & PhysicsDebugFlags::CONSTRAINT);
					if(ImGui::Checkbox("Constraints", &showConstraints))
					{
						if(showConstraints)
							flags += PhysicsDebugFlags::CONSTRAINT;
						else
							flags -= PhysicsDebugFlags::CONSTRAINT;
					}

					bool showManifolds = static_cast<bool>(flags & PhysicsDebugFlags::MANIFOLD);
					if(ImGui::Checkbox("Manifolds", &showManifolds))
					{
						if(showManifolds)
							flags += PhysicsDebugFlags::MANIFOLD;
						else
							flags -= PhysicsDebugFlags::MANIFOLD;
					}

					bool showCollisionNormals = flags & PhysicsDebugFlags::COLLISIONNORMALS;
					if(ImGui::Checkbox("Collision Normals", &showCollisionNormals))
					{
						if(showCollisionNormals)
							flags += PhysicsDebugFlags::COLLISIONNORMALS;
						else
							flags -= PhysicsDebugFlags::COLLISIONNORMALS;
					}

					bool showAABB = flags & PhysicsDebugFlags::AABB;
					if(ImGui::Checkbox("AABB", &showAABB))
					{
						if(showAABB)
							flags += PhysicsDebugFlags::AABB;
						else
							flags -= PhysicsDebugFlags::AABB;
					}

					bool showLinearVelocity = flags & PhysicsDebugFlags::LINEARVELOCITY;
					if(ImGui::Checkbox("Linear Velocity", &showLinearVelocity))
					{
						if(showLinearVelocity)
							flags += PhysicsDebugFlags::LINEARVELOCITY;
						else
							flags -= PhysicsDebugFlags::LINEARVELOCITY;
					}

					bool LINEARFORCE = flags & PhysicsDebugFlags::LINEARFORCE;
					if(ImGui::Checkbox("Linear Force", &LINEARFORCE))
					{
						if(LINEARFORCE)
							flags += PhysicsDebugFlags::LINEARFORCE;
						else
							flags -= PhysicsDebugFlags::LINEARFORCE;
					}

					bool BROADPHASE = flags & PhysicsDebugFlags::BROADPHASE;
					if(ImGui::Checkbox("Broadphase", &BROADPHASE))
					{
						if(BROADPHASE)
							flags += PhysicsDebugFlags::BROADPHASE;
						else
							flags -= PhysicsDebugFlags::BROADPHASE;
					}

					bool showPairs = flags & PhysicsDebugFlags::BROADPHASE_PAIRS;
					if(ImGui::Checkbox("Broadphase Pairs", &showPairs))
					{
						if(showPairs)
							flags += PhysicsDebugFlags::BROADPHASE_PAIRS;
						else
							flags -= PhysicsDebugFlags::BROADPHASE_PAIRS;
					}

					physics3D->SetDebugDrawFlags(flags);
				}

				ImGui::EndPopup();
			}
		}

		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();
		{
			selected = m_ShowStats;
			if(selected)
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.9f, 1.0f));

			ImGui::SameLine();
			if(ImGui::Button("Stats"))
			{
				m_ShowStats = !m_ShowStats;
			}

			if(selected)
				ImGui::PopStyleColor();
			ImGuiHelpers::Tooltip("Show Statistics");
		}

		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();
		//Editor Camera Settings

		auto& camera = *m_Editor->GetCamera();
		bool ortho = camera.IsOrthographic();

		selected = !ortho;
		if(selected)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.9f, 1.0f));
		if(ImGui::Button(ICON_MDI_AXIS_ARROW" 3D"))
		{
			if(ortho)
			{
				camera.SetIsOrthographic(false);
				m_Editor->GetEditorCameraController().SetMode(false);
			}
		}
		if(selected)
			ImGui::PopStyleColor();
		ImGui::SameLine();

		selected = ortho;
		if(selected)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.9f, 1.0f));
		if(ImGui::Button(ICON_MDI_ANGLE_RIGHT "2D"))
		{
			if(!ortho)
			{
				camera.SetIsOrthographic(true);
                m_Editor->GetEditorCameraTransform().SetLocalOrientation(Maths::Quaternion::EulerAnglesToQuaternion(0.0f, 0.0f, 0.0f));
				m_Editor->GetEditorCameraController().SetMode(true);
			}
		}
		if(selected)
			ImGui::PopStyleColor();

		ImGui::PopStyleColor();
		ImGui::Unindent();
	}

	void SceneWindow::OnNewScene(Scene* scene)
	{
		m_AspectRatio = 1.0f;
		m_CurrentScene = scene;
		auto sceneLayers = Application::Get().GetSceneLayers();

		for(auto layer : *sceneLayers)
		{
			layer->SetRenderTarget(m_GameViewTexture.get(), true);
			layer->SetOverrideCamera(m_Editor->GetCamera(), &m_Editor->GetEditorCameraTransform());
		}

		DebugRenderer::SetRenderTarget(m_GameViewTexture.get(), true);
		DebugRenderer::SetOverrideCamera(m_Editor->GetCamera(), &m_Editor->GetEditorCameraTransform());
	}

	void SceneWindow::Resize(u32 width, u32 height)
	{
		bool resize = false;

		LUMOS_ASSERT(width > 0 && height > 0, "Scene View Dimensions 0");

		Application::Get().SetSceneViewDimensions(width, height);

		if(m_Width != width || m_Height != height)
		{
			resize = true;
			m_Width = width;
			m_Height = height;
		}

		if(!m_GameViewTexture)
			m_GameViewTexture = Ref<Graphics::Texture2D>(Graphics::Texture2D::Create());

		if(resize)
		{
			Graphics::GraphicsContext::GetContext()->WaitIdle();

			m_GameViewTexture->BuildTexture(Graphics::TextureFormat::RGBA32, m_Width, m_Height, false, false);

			auto sceneLayers = Application::Get().GetSceneLayers();

			for(auto layer : *sceneLayers)
			{
				layer->SetRenderTarget(m_GameViewTexture.get(), true, false); //Prevent framebuffer being rebuilt as it needs resizing after
			}

			DebugRenderer::SetRenderTarget(m_GameViewTexture.get(), false);

			if(!m_Editor->GetGridRenderer())
				m_Editor->CreateGridRenderer();
			m_Editor->GetGridRenderer()->SetRenderTarget(m_GameViewTexture.get(), false);
			m_Editor->GetGridRenderer()->OnResize(m_Width, m_Height);

			WindowResizeEvent e(width, height);
			auto& app = Application::Get();
			app.GetRenderManager()->OnResize(width, height);

			for(auto layer : *sceneLayers)
			{
				layer->OnEvent(e);
			}
			DebugRenderer::OnResize(width, height);

			Graphics::GraphicsContext::GetContext()->WaitIdle();
		}
	}
}
