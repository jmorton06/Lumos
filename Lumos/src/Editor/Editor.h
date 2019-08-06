#pragma once
#include "Maths/Vector2.h"

namespace Lumos
{
	class Application;
	class Entity;
	class Console;
	class Scene;
	
	namespace Graphics 
	{
		class Texture2D;
	}

	class Editor
	{
		friend class Application;
	public:
		Editor(Application* app, u32 width, u32 height);
		~Editor();
        
        Editor(Editor const&) = delete;
        Editor& operator=(Editor const&) = delete;
        
		void OnImGui();
		void DrawConsole();
		void DrawMenuBar();
		void DrawHierarchyWindow();
		void DrawInspectorWindow();
		void DrawSceneView();
        void DrawGraphicsInfoWindow();
		void BeginDockSpace();
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
        
        static bool m_ShowConsole;
        static bool m_ShowHierarchy;
        static bool m_ShowSceneView;
        static bool m_ShowGraphicsInfo;
        static bool m_ShowInspector;
        static bool m_ShowImGuiDemo;
	};
}
