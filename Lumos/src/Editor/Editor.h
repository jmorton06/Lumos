#pragma once
#include "Maths/Maths.h"
#include "Maths/Ray.h"

#include "EditorWindow.h"

#include <imgui/imgui.h>
#include <entt/entt.hpp>

namespace ImGui
{
	class FileBrowser;
}

namespace Lumos
{
	class Application;
	class Scene;
	class Event;
	class Layer3D;
	class WindowCloseEvent;
	class WindowResizeEvent;
	class TimeStep;
	
	namespace Graphics 
	{
		class Texture2D;
		class GridRenderer;
	}
    

    enum EditorDebugFlags : u32
    {
        Grid = 1,
        Gizmo = 2,
        ViewSelected = 4,
        CameraFrustum = 8,
        MeshBoundingBoxes = 16,
    };

	class Editor
	{
		friend class Application;
        friend class SceneWindow;
	public:
		Editor(Application* app, u32 width, u32 height);
		~Editor();

		void OnInit();
		void OnImGui();
		void DrawMenuBar();
		void BeginDockSpace(bool infoBar);
		void EndDockSpace();

        void SetImGuizmoOperation(u32 operation) { m_ImGuizmoOperation = operation; }
		u32 GetImGuizmoOperation() const { return m_ImGuizmoOperation; }

		void OnNewScene(Scene* scene);
		void OnImGuizmo();
		void OnEvent(Event& e);
		void OnUpdate(TimeStep* ts);

		void Draw2DGrid(ImDrawList* drawList, const ImVec2& cameraPos, const ImVec2& windowPos, const ImVec2& canvasSize, const float factor, const float thickness);

		bool& ShowGrid() { return m_ShowGrid; }
		const float& GetGridSize() const { return m_GridSize; }

		bool& ShowGizmos() { return m_ShowGizmos; }
		bool& ShowViewSelected() { return m_ShowViewSelected; }

		bool& SnapGuizmo() { return m_SnapQuizmo; }
		float& SnapAmount() { return m_SnapAmount; }

		void SetSelected(entt::entity entity) { m_Selected = entity; }
		entt::entity GetSelected() const { return m_Selected; }

		void BindEventFunction();

		std::unordered_map<size_t, const char*>& GetComponentIconMap() { return m_ComponentIconMap; }

		float& GetCurrentSceneAspectRatio() { return m_CurrentSceneAspectRatio; }
		void FocusCamera(const Maths::Vector3& point, float distance, float speed = 1.0f);
    
        void RecompileShaders();
        void DebugDraw();
        void SelectObject(const Maths::Ray& ray);

		void OpenTextFile(const String& filePath);
		void RemoveWindow(EditorWindow* window);

        Maths::Vector2 m_SceneWindowPos;

	protected:
		bool OnWindowResize(WindowResizeEvent& e);

		Application* m_Application;

		u32 m_ImGuizmoOperation = 0;
		entt::entity m_Selected;
        float m_GridSize = 10.0f;
        u32 m_DebugDrawFlags = 0;

		bool m_ShowGrid = false;
		bool m_ShowGizmos = true;
		bool m_ShowViewSelected = false;
		bool m_SnapQuizmo = false;
		bool m_ShowImGuiDemo = true;
		bool m_View2D = false;
		float m_SnapAmount = 1.0f;
		float m_CurrentSceneAspectRatio = 0.0f;
		bool m_TransitioningCamera = false;
		Maths::Vector3 m_CameraDestination;
		Maths::Vector3 m_CameraStartPosition;
		float m_CameraTransitionStartTime = 0.0f;
		float m_CameraTransitionSpeed = 0.0f;
            
        ImGui::FileBrowser* m_FileBrowser;

		std::vector<Ref<EditorWindow>> m_Windows;

		Layer3D* m_3DGridLayer = nullptr;

		std::unordered_map<size_t, const char*> m_ComponentIconMap;

		NONCOPYABLE(Editor)
	};
}
