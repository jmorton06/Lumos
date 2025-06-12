#pragma once
#include <string>
#include <Lumos/Maths/Transform.h>
#include <Lumos/Utilities/IniFile.h>
#include <Lumos/Graphics/Camera/EditorCamera.h>
#include <Lumos/ImGui/ImGuiUtilities.h>
#include <Lumos/Core/Application.h>
#include <Lumos/Scene/Entity.h>
#include <Lumos/Maths/Vector3.h>
#include <imgui/imgui.h>

namespace Lumos
{
#define BIND_FILEBROWSER_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

    class Scene;
    class Event;
    class WindowCloseEvent;
    class WindowResizeEvent;
    class WindowFileEvent;
    class TimeStep;
    class Entity;
    class FileBrowserPanel;
    class PreviewDraw;
    class EditorPanel;
    class Camera;

    namespace Graphics
    {
        class Texture2D;
        class GridRenderer;
        class Mesh;
        class Environment;
        class SceneRenderer;
    }

    namespace Maths
    {
        class Ray;
    }

    enum EditorDebugFlags : uint32_t
    {
        Grid              = 1,
        Gizmo             = 2,
        ViewSelected      = 4,
        CameraFrustum     = 8,
        MeshBoundingBoxes = 16,
        SpriteBoxes       = 32,
        EntityNames       = 64,

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
        bool IsShaderFile(const std::string& filePath);
        bool IsFontFile(const std::string& filePath);
        bool IsMaterialFile(const std::string& filePath);

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

        void ClearSelected()
        {
            m_SelectedEntities.clear();
        }

        void SetSelected(Entity entity);
        void UnSelect(Entity entity);
        void SetHoveredEntity(Entity entity) { m_HoveredEntity = entity; }
        Entity GetHoveredEntity() { return m_HoveredEntity; }
        const std::vector<Entity>& GetSelected() const
        {
            return m_SelectedEntities;
        }

        bool IsSelected(Entity entity)
        {
            if(std::find(m_SelectedEntities.begin(), m_SelectedEntities.end(), entity) != m_SelectedEntities.end())
                return true;

            return false;
        }

        bool IsCopied(Entity entity)
        {
            if(std::find(m_CopiedEntities.begin(), m_CopiedEntities.end(), entity) != m_CopiedEntities.end())
                return true;

            return false;
        }

        void SetCopiedEntity(Entity entity, bool cut = false)
        {
            if(std::find(m_CopiedEntities.begin(), m_CopiedEntities.end(), entity) != m_CopiedEntities.end())
                return;

            m_CopiedEntities.push_back(entity);
            m_CutCopyEntity = cut;
        }

        const std::vector<Entity>& GetCopiedEntity() const
        {
            return m_CopiedEntities;
        }

        bool GetCutCopyEntity()
        {
            return m_CutCopyEntity;
        }

        std::unordered_map<size_t, const char*>& GetComponentIconMap()
        {
            return m_ComponentIconMap;
        }

        void FocusCamera(const Vec3& point, float distance, float speed = 1.0f);

        void RecompileShaders();
        void DebugDraw();
        void SelectObject(const Maths::Ray& ray, bool hoveredOnly = false);

        void OpenTextFile(const std::string& filePath, const std::function<void()>& callback);
        void RemovePanel(EditorPanel* panel);
        EditorPanel* GetTextEditPanel();

        void ShowPreview();
        void DrawPreview();
        void SavePreview();
        void RequestThumbnail(String8 asset);

        static Editor* GetEditor() { return (Editor*)&Application::Get(); }

        Maths::Ray GetScreenRay(int x, int y, Camera* camera, int width, int height);

        void FileOpenCallback(const std::string& filepath);
        void ProjectOpenCallback(const std::string& filepath);
        void NewProjectOpenCallback(const std::string& filepath);
        void FileEmbedCallback(const std::string& filepath);
        void NewProjectLocationCallback(const std::string& filepath);

        FileBrowserPanel& GetFileBrowserPanel()
        {
            return *m_FileBrowserPanel;
        }

        void AddDefaultEditorSettings();
        void SaveEditorSettings();
        void LoadEditorSettings();

        void OpenFile();
        void EmbedFile();
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
        bool OnFileDrop(WindowFileEvent& e);

        struct EditorSettings
        {
            float m_GridSize               = 10.0f;
            uint32_t m_DebugDrawFlags      = 0;
            uint32_t m_Physics2DDebugFlags = 0;
            uint32_t m_Physics3DDebugFlags = 0;

            bool m_ShowGrid         = true;
            bool m_ShowGizmos       = true;
            bool m_ShowViewSelected = false;
            bool m_SnapQuizmo       = false;
            bool m_ShowImGuiDemo    = true;
            bool m_View2D           = false;
            bool m_FullScreenOnPlay = false;
            float m_SnapAmount      = 1.0f;
            bool m_SleepOutofFocus  = true;
            float m_ImGuizmoScale   = 0.25f;

            bool m_FullScreenSceneView    = false;
            ImGuiUtilities::Theme m_Theme = ImGuiUtilities::Theme::Black;
            bool m_FreeAspect             = true;
            float m_FixedAspect           = 1.0f;
            float m_AspectRatio           = 1.0f;

            // Camera Settings
            float m_CameraSpeed = 1000.0f;
            float m_CameraNear  = 0.01f;
            float m_CameraFar   = 1000.0f;

            std::vector<std::string> m_RecentProjects;
        };

        EditorSettings& GetSettings() { return m_Settings; }
        void SetSceneViewActive(bool active) { m_SceneViewActive = active; }
        void SetEditorScriptsPath(const std::string& path);
        String8 GetEditorScriptsPath() { return m_EditorScriptPath; }

        SharedPtr<Graphics::Texture2D> GetPreviewTexture() const;

    protected:
        NONCOPYABLE(Editor)
        bool OnWindowResize(WindowResizeEvent& e);

        uint32_t m_ImGuizmoOperation = 14463;
        std::vector<Entity> m_SelectedEntities;
        std::vector<Entity> m_CopiedEntities;
        Entity m_HoveredEntity;
        bool m_CutCopyEntity            = false;
        float m_CurrentSceneAspectRatio = 0.0f;
        float m_CameraTransitionSpeed   = 0.0f;
        bool m_TransitioningCamera      = false;
        Vec3 m_CameraDestination;
        bool m_SceneViewActive     = false;
        bool m_NewProjectPopupOpen = false;

        EditorSettings m_Settings;
        std::vector<SharedPtr<EditorPanel>> m_Panels;

        std::unordered_map<size_t, const char*> m_ComponentIconMap;

        FileBrowserPanel* m_FileBrowserPanel;
        PreviewDraw* m_PreviewDraw;

        Camera* m_CurrentCamera = nullptr;
        EditorCameraController m_EditorCameraController;
        Maths::Transform m_EditorCameraTransform;

        SharedPtr<Camera> m_EditorCamera = nullptr;

        SharedPtr<Graphics::GridRenderer> m_GridRenderer;

        std::string m_TempSceneSaveFilePath;
        int m_AutoSaveSettingsTime = 15000;
        String8 m_EditorScriptPath;

        bool m_DrawPreview        = false;
        bool m_SavePreviewTexture = false;
        bool m_QueuePreviewSave   = false;
        String8 m_RequestedThumbnailPath;

        IniFile m_IniFile;

        bool m_PreviewScreenshot = false;
    };
}
