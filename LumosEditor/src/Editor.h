#pragma once

#include <algorithm>
#include <functional>
#include <iostream>
#include <string>
#include <sstream>

#include <vector>
#include <list>
#include <array>
#include <map>
#include <unordered_map>
#include <cstddef>
#include <fstream>
#include <cfloat>
#include <cstring>
#include <utility>
#include <memory>
#include <thread>

#include <stdio.h>
#include <math.h>

#include "Maths/Maths.h"
#include "Maths/Ray.h"
#include "Maths/Transform.h"

#include "EditorWindow.h"
#include "FileBrowserWindow.h"
#include "Utilities/IniFile.h"
#include "Graphics/Camera/EditorCamera.h"
#include "Graphics/Camera/Camera.h"
#include "Core/Types.h"
#include "ImGui/ImGuiHelpers.h"
#include "Core/Application.h"
#include <imgui/imgui.h>
#include <entt/entity/fwd.hpp>

namespace Lumos
{
#define BIND_FILEBROWSER_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

	class Scene;
	class Event;
	class WindowCloseEvent;
	class WindowResizeEvent;
	class TimeStep;

	namespace Graphics
	{
		class Texture2D;
		class GridRenderer;
		class ForwardRenderer;
        class GridRenderer;
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

	class Editor : public Application
	{
		friend class Application;
		friend class SceneWindow;

	public:
		Editor();
		virtual ~Editor();

		void Init() override;
		void OnImGui() override;
		void OnRender() override;
		void OnEvent(Event& e) override;
        void Quit() override;

		void DrawMenuBar();
		void BeginDockSpace(bool gameFullScreen);
		void EndDockSpace();

		void SetImGuizmoOperation(u32 operation)
		{
			m_ImGuizmoOperation = operation;
		}
		u32 GetImGuizmoOperation() const
		{
			return m_ImGuizmoOperation;
		}

		void OnNewScene(Scene* scene) override;
		void OnImGuizmo();
		void OnUpdate(const TimeStep& ts) override;

		void Draw2DGrid(ImDrawList* drawList, const ImVec2& cameraPos, const ImVec2& windowPos, const ImVec2& canvasSize, const float factor, const float thickness);
        void Draw3DGrid();

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
        
        bool& FullScreenOnPlay()
        {
            return m_FullScreenOnPlay;
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
			m_SelectedEntity = entity;
		}
		entt::entity GetSelected() const
		{
			return m_SelectedEntity;
		}

		void SetCopiedEntity(entt::entity entity, bool cut = false)
		{
			m_CopiedEntity = entity;
			m_CutCopyEntity = cut;
		}

		entt::entity GetCopiedEntity() const
		{
			return m_CopiedEntity;
		}

		bool GetCutCopyEntity()
		{
			return m_CutCopyEntity;
		}

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
		
		static Editor* GetEditor() { return s_Editor; }

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

		Camera* GetCamera() const
		{
            return m_EditorCamera.get();
		}
    
        void CreateGridRenderer();
        const Ref<Graphics::GridRenderer>& GetGridRenderer();

		EditorCameraController& GetEditorCameraController() 
		{
			return m_EditorCameraController;
		}

		Maths::Transform& GetEditorCameraTransform()
		{
			return m_EditorCameraTransform;
		}

		void CacheScene();
		void LoadCachedScene();

	protected:
	
		NONCOPYABLE(Editor)
	    bool OnWindowResize(WindowResizeEvent& e);

		Application* m_Application;

		u32 m_ImGuizmoOperation = 0;
		entt::entity m_SelectedEntity;
		entt::entity m_CopiedEntity;
		bool m_CutCopyEntity = false;

		float m_GridSize = 10.0f;
		u32 m_DebugDrawFlags = 0;

		bool m_ShowGrid = false;
		bool m_ShowGizmos = true;
		bool m_ShowViewSelected = false;
		bool m_SnapQuizmo = false;
		bool m_ShowImGuiDemo = true;
		bool m_View2D = false;
        bool m_FullScreenOnPlay = false;
		float m_SnapAmount = 1.0f;
		float m_CurrentSceneAspectRatio = 0.0f;
		bool m_TransitioningCamera = false;
		Maths::Vector3 m_CameraDestination;
		Maths::Vector3 m_CameraStartPosition;
		float m_CameraTransitionStartTime = 0.0f;
		float m_CameraTransitionSpeed = 0.0f;
        
        bool m_FullScreenSceneView = false;
		ImGuiHelpers::Theme m_Theme = ImGuiHelpers::Theme::Dark;

		std::vector<Ref<EditorWindow>> m_Windows;

		std::unordered_map<size_t, const char*> m_ComponentIconMap;

		FileBrowserWindow m_FileBrowserWindow;
		Camera* m_CurrentCamera = nullptr;
		EditorCameraController m_EditorCameraController;
		Maths::Transform m_EditorCameraTransform;

		Ref<Camera> m_EditorCamera = nullptr;
		Ref<Graphics::ForwardRenderer> m_PreviewRenderer;
		Ref<Graphics::Texture2D> m_PreviewTexture;
		Ref<Graphics::Mesh> m_PreviewSphere;
        Ref<Graphics::GridRenderer> m_GridRenderer;
		std::string m_TempSceneSaveFilePath;

		IniFile m_IniFile;
		
		static Editor* s_Editor;
	};
}
