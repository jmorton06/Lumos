#pragma once
#include "Maths/Vector2.h"
#include "EditorWindow.h"

#include <imgui/imgui.h>

namespace ImGui
{
	class FileBrowser;
}

namespace Lumos
{
	class Application;
	class Entity;
	class Scene;
	class Event;
	class WindowCloseEvent;
	class WindowResizeEvent;
	
	namespace Graphics 
	{
		class Texture2D;
		class GridRenderer;
	}

	class Editor
	{
		friend class Application;
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

		void Draw2DGrid(ImDrawList* drawList, const ImVec2& cameraPos, const ImVec2& windowPos, const ImVec2& canvasSize, const float factor, const float thickness);

		const bool& GetShowGrid() const { return m_ShowGrid; }
		const float& GetGridSize() const { return m_GridSize; }

		void SetSelected(Entity* entity) { m_Selected = entity; }
		Entity* GetSelected() const { return m_Selected; }

		void BindEventFunction();

	protected:
		bool OnWindowResize(WindowResizeEvent& e);

		Application* m_Application;

		u32 m_ImGuizmoOperation = 0;
		Entity* m_Selected = nullptr;
        float m_GridSize = 10.0f;

		bool m_ShowGrid = true;
		bool m_SnapQuizmo = true;
		bool m_ShowImGuiDemo = true;
		float m_SnapAmount = 1.0f;
        
        ImGui::FileBrowser* m_FileBrowser;

		std::vector<Ref<EditorWindow>> m_Windows;

		NONCOPYABLE(Editor)
	};
}
