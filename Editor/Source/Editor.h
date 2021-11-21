#pragma once

#include "EditorPanel.h"
#include "FileBrowserPanel.h"

#include <Lumos/Maths/Maths.h>
#include <Lumos/Maths/Ray.h>
#include <Lumos/Maths/Transform.h>
#include <Lumos/Utilities/IniFile.h>
#include <Lumos/Graphics/Camera/EditorCamera.h>
#include <Lumos/Graphics/Camera/Camera.h>
#include <Lumos/ImGui/ImGuiUtilities.h>
#include <Lumos/Core/Application.h>

#include <imgui/imgui.h>
#include <entt/entity/fwd.hpp>

namespace Lumos
{
#define BIND_FILEBROWSER_FN(fn) [this](auto&&... args) -> decltype(auto) { \
    return this->fn(std::forward<decltype(args)>(args)...);                \
}

    class Scene;
    class Event;
    class WindowCloseEvent;
    class WindowResizeEvent;
    class TimeStep;

    namespace Graphics
    {
        class Texture2D;
        class GridRenderer;
        class Mesh;
    }

    enum EditorDebugFlags : uint32_t
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
        friend class SceneViewPanel;

    public:
        Editor();
        virtual ~Editor();

        void Init() override;
        void OnImGui() override;
        void OnRender() override;
        void OnDebugDraw() override;
        void OnEvent(Event& e) override;
        void OnQuit() override;

        void DrawMenuBar();
        void BeginDockSpace(bool gameFullScreen);
        void EndDockSpace();

        bool IsTextFile(const std::string& filePath);
        bool IsAudioFile(const std::string& filePath);
        bool IsSceneFile(const std::string& filePath);
        bool IsModelFile(const std::string& filePath);
        bool IsTextureFile(const std::string& filePath);

        void SetImGuizmoOperation(uint32_t operation)
        {
            m_ImGuizmoOperation = operation;
        }
        uint32_t GetImGuizmoOperation() const
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
            return m_Settings.m_ShowGrid;
        }
        const float& GetGridSize() const
        {
            return m_Settings.m_GridSize;
        }

        bool& ShowGizmos()
        {
            return m_Settings.m_ShowGizmos;
        }
        bool& ShowViewSelected()
        {
            return m_Settings.m_ShowViewSelected;
        }

        void ToggleSnap()
        {
            m_Settings.m_SnapQuizmo = !m_Settings.m_SnapQuizmo;
        }

        bool& FullScreenOnPlay()
        {
            return m_Settings.m_FullScreenOnPlay;
        }

        bool& SnapGuizmo()
        {
            return m_Settings.m_SnapQuizmo;
        }
        float& SnapAmount()
        {
            return m_Settings.m_SnapAmount;
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

        void FocusCamera(const glm::vec3& point, float distance, float speed = 1.0f);

        void RecompileShaders();
        void DebugDraw();
        void SelectObject(const Ray& ray);

        void OpenTextFile(const std::string& filePath, const std::function<void()>& callback);
        void RemovePanel(EditorPanel* panel);
        EditorPanel* GetTextEditPanel();

        void ShowPreview();
        void DrawPreview();

        static Editor* GetEditor() { return s_Editor; }

        glm::vec2 m_SceneViewPanelPosition;
        Ray GetScreenRay(int x, int y, Camera* camera, int width, int height);

        void FileOpenCallback(const std::string& filepath);
        void ProjectOpenCallback(const std::string& filepath);
        void NewProjectOpenCallback(const std::string& filepath);

        FileBrowserPanel& GetFileBrowserPanel()
        {
            return m_FileBrowserPanel;
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
        const SharedPtr<Graphics::GridRenderer>& GetGridRenderer();

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
        void EmbedShader(const std::string& shaderPath);

        struct EditorSettings
        {
            float m_GridSize = 10.0f;
            uint32_t m_DebugDrawFlags = 0;
            uint32_t m_Physics2DDebugFlags = 0;
            uint32_t m_Physics3DDebugFlags = 0;

            bool m_ShowGrid = true;
            bool m_ShowGizmos = true;
            bool m_ShowViewSelected = false;
            bool m_SnapQuizmo = false;
            bool m_ShowImGuiDemo = true;
            bool m_View2D = false;
            bool m_FullScreenOnPlay = false;
            float m_SnapAmount = 1.0f;
            bool m_SleepOutofFocus = true;
            float m_ImGuizmoScale = 0.25f;

            bool m_FullScreenSceneView = false;
            ImGuiUtilities::Theme m_Theme = ImGuiUtilities::Theme::Dark;

            //Camera Settings
        };

        EditorSettings& GetSettings() { return m_Settings; }

    protected:
        NONCOPYABLE(Editor)
        bool OnWindowResize(WindowResizeEvent& e);

        Application* m_Application;

        uint32_t m_ImGuizmoOperation = 0;
        entt::entity m_SelectedEntity;
        entt::entity m_CopiedEntity;
        bool m_CutCopyEntity = false;
        float m_CurrentSceneAspectRatio = 0.0f;
        float m_CameraTransitionStartTime = 0.0f;
        float m_CameraTransitionSpeed = 0.0f;
        bool m_TransitioningCamera = false;
        glm::vec3 m_CameraDestination;
        glm::vec3 m_CameraStartPosition;

        EditorSettings m_Settings;
        std::vector<SharedPtr<EditorPanel>> m_Panels;

        std::unordered_map<size_t, const char*> m_ComponentIconMap;

        FileBrowserPanel m_FileBrowserPanel;
        Camera* m_CurrentCamera = nullptr;
        EditorCameraController m_EditorCameraController;
        Maths::Transform m_EditorCameraTransform;

        SharedPtr<Camera> m_EditorCamera = nullptr;
        //SharedPtr<Graphics::ForwardRenderer> m_PreviewRenderer;
        SharedPtr<Graphics::Texture2D> m_PreviewTexture;
        SharedPtr<Graphics::Mesh> m_PreviewSphere;
        SharedPtr<Graphics::GridRenderer> m_GridRenderer;
        std::string m_TempSceneSaveFilePath;

        IniFile m_IniFile;

        static Editor* s_Editor;
    };
}
