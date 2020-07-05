#pragma once
#include "Maths/Maths.h"
#include "Maths/Ray.h"

#include "EditorWindow.h"
#include "FileBrowserWindow.h"
#include "Utilities/IniFile.h"

#include <imgui/imgui.h>
#include <entt/entity/fwd.hpp>

namespace Lumos
{
#define BIND_FILEBROWSER_FN(x) std::bind(&x, this, std::placeholders::_1)

	class Application;
	class Scene;
	class Event;
	class Layer3D;
	class WindowCloseEvent;
	class WindowResizeEvent;
	class TimeStep;
	class Camera;

	namespace Graphics
	{
		class Texture2D;
		class GridRenderer;
		class ForwardRenderer;
		class Mesh;
	}

	enum EditorDebugFlags : u32
	{
		Grid = 1,
		Gizmo = 2,
		ViewSelected = 4,
		CameraFrustum = 8,
		MeshBoundingBoxes = 16,
		SpriteBoxes = 32,
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
		void OnRender();
		void DrawMenuBar();
		void BeginDockSpace(bool infoBar);
		void EndDockSpace();

		void SetImGuizmoOperation(u32 operation)
		{
			m_ImGuizmoOperation = operation;
		}
		u32 GetImGuizmoOperation() const
		{
			return m_ImGuizmoOperation;
		}

		void OnNewScene(Scene* scene);
		void OnImGuizmo();
		void OnEvent(Event& e);
		void OnUpdate(const TimeStep& ts);

		void Draw2DGrid(ImDrawList* drawList, const ImVec2& cameraPos, const ImVec2& windowPos, const ImVec2& canvasSize, const float factor, const float thickness);

		bool& ShowGrid()
		{
			return m_ShowGrid;
		}
		const float& GetGridSize() const
		{
			return m_GridSize;
		}

		bool& ShowGizmos()
		{
			return m_ShowGizmos;
		}
		bool& ShowViewSelected()
		{
			return m_ShowViewSelected;
		}

		void ToggleSnap()
		{
			m_SnapQuizmo = !m_SnapQuizmo;
		}

		bool& SnapGuizmo()
		{
			return m_SnapQuizmo;
		}
		float& SnapAmount()
		{
			return m_SnapAmount;
		}

		void SetSelected(entt::entity entity)
		{
			m_Selected = entity;
		}
		entt::entity GetSelected() const
		{
			return m_Selected;
		}

		void BindEventFunction();

		std::unordered_map<size_t, const char*>& GetComponentIconMap()
		{
			return m_ComponentIconMap;
		}

		void FocusCamera(const Maths::Vector3& point, float distance, float speed = 1.0f);

		void RecompileShaders();
		void DebugDraw();
		void SelectObject(const Maths::Ray& ray);

		void OpenTextFile(const std::string& filePath);
		void RemoveWindow(EditorWindow* window);

		void ShowPreview();
		void DrawPreview();

		Maths::Vector2 m_SceneWindowPos;
		Maths::Ray GetScreenRay(int x, int y, Camera* camera, int width, int height);

		void FileOpenCallback(const std::string& filepath);

		FileBrowserWindow& GetFileBrowserWindow()
		{
			return m_FileBrowserWindow;
		}

		void AddDefaultEditorSettings();
		void SaveEditorSettings();
		void LoadEditorSettings();

		void OpenFile();
		const char* GetIconFontIcon(const std::string& fileType);

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

		std::vector<Ref<EditorWindow>> m_Windows;

		Layer3D* m_3DGridLayer = nullptr;

		std::unordered_map<size_t, const char*> m_ComponentIconMap;

		FileBrowserWindow m_FileBrowserWindow;
		Camera* m_EditorCamera = nullptr;
		//CameraController* m_EditorCameraController = nullptr;

		NONCOPYABLE(Editor)

		Ref<Graphics::ForwardRenderer> m_PreviewRenderer;
		Ref<Graphics::Texture2D> m_PreviewTexture;
		Ref<Graphics::Mesh> m_PreviewSphere;

		IniFile m_IniFile;
	};
}
