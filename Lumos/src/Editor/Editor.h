#pragma once
#include "Maths/Vector2.h"
#include <imgui/imgui.h>

namespace Lumos
{
	class Application;
	class Entity;
	class Console;
	class Scene;
	
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

		void OnImGui();
		void DrawConsole();
		void DrawMenuBar();
		void DrawHierarchyWindow();
		void DrawInspectorWindow();
		void DrawSceneView();
        void DrawGraphicsInfoWindow();
		void DrawInfoBar();
		void BeginDockSpace(bool infoBar);
		void EndDockSpace();
        
        void DrawNode(Entity* node);

		void SelectEntity();

		u32 GetImGuizmoOperation() const { return m_ImGuizmoOperation; }
		void OnInit();
		void OnNewScene(Scene* scene);

	private:
		Application* m_Application;

		Maths::Vector2 m_SceneViewSize;
		Maths::Vector2 m_SceneViewPosition;
		u32 m_ImGuizmoOperation = 0;
		Entity* m_Selected = nullptr;
        float m_GridSize = 10.0f;

		bool m_ShowGrid = false;
        bool m_ShowConsole = true;
        bool m_ShowHierarchy = true;
        bool m_ShowSceneView = true;
        bool m_ShowGraphicsInfo = true;
        bool m_ShowInspector = true;
        bool m_ShowImGuiDemo = true;
        bool m_ShowProfiler = true;
        
        ImGuiTextFilter m_HierarchyFilter;


		NONCOPYABLE(Editor)
	};
}
