#pragma once
#include <string>
#include <Lumos/Maths/Transform.h>
#include <Lumos/Utilities/IniFile.h>
#include <Lumos/Graphics/Camera/EditorCamera.h>
#include <Lumos/Core/Application.h>
#include <Lumos/Core/JobSystem.h>
#include <Lumos/Scene/Entity.h>
#include <Lumos/Maths/Vector3.h>

namespace Lumos
{

    class Scene;
    class Event;
    class WindowCloseEvent;
    class WindowResizeEvent;
    class WindowFileEvent;
    class TimeStep;
    class Entity;
    class PreviewDraw;
    class EditorPanel;
    class ImportPanel;
    class Camera;

    class EditorUI;
    class TerrainSculpt;
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

    // What the scene view can leave out to draw faster while working.
    enum SceneViewOff : uint32_t
    {
        SceneViewOff_Shadows      = 1 << 0,
        SceneViewOff_Bloom        = 1 << 1,
        SceneViewOff_SSAO         = 1 << 2,
        SceneViewOff_SSR          = 1 << 3,
        SceneViewOff_AntiAliasing = 1 << 4,
        SceneViewOff_DepthOfField = 1 << 5,
        SceneViewOff_PostEffects  = 1 << 6, // debanding, grain, vignette, chromatic aberration, sharpen, motion blur
        SceneViewOff_Skybox       = 1 << 7,
        SceneViewOff_All          = 0xFF
    };

    class Editor : public Application
    {
        friend class Application;
        friend class SceneViewPanel;

    public:
        Editor();
        virtual ~Editor();

        void Init() override;
        void OnRender() override;
        void OnDebugDraw() override;
        void OnEvent(Event& e) override;
        void OnQuit() override;
        
        void ExitApp() override;



        bool IsTextFile(const std::string& filePath);
        bool IsAudioFile(const std::string& filePath);
        bool IsSceneFile(const std::string& filePath);
        bool IsModelFile(const std::string& filePath);
        bool IsTextureFile(const std::string& filePath);
        bool IsShaderFile(const std::string& filePath);
        bool IsFontFile(const std::string& filePath);
        bool IsMaterialFile(const std::string& filePath);

        // GizmoOperation_* (Graphics/Gizmo.h); None = the select tool
        void SetGizmoOperation(uint32_t operation) { m_GizmoOperation = operation; }
        uint32_t GetGizmoOperation() const { return m_GizmoOperation; }

        void OnNewScene(Scene* scene) override;
        void OnUpdate(const TimeStep& ts) override;

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

        Graphics::Texture2D* GetLogoTexture() const { return m_LogoTexture.get(); }
        ImportPanel* GetImportPanel() { return m_ImportPanel; }
        // File dialog for a .lmproj; opens what is picked.
        void BrowseForProject();
        void ShowNewProjectDialog();

        // What the project list shows for one .lmproj. Read once and kept.
        struct RecentProject
        {
            std::string Title, Folder, Export;
            bool Missing = false;
        };
        const RecentProject& GetRecentProject(const std::string& lmproj);
        void RemoveRecentProject(size_t index);
        // New folder <location>/<name>/ with a project in it, then opens it.
        void CreateProject(const std::string& location, const std::string& name);
        bool IsProjectLoaded() const { return m_ProjectLoaded; }
        // Back to the project list.
        void CloseProject()
        {
            m_ProjectLoaded = false;
            m_ProjectLoadError.clear();
        }
        const std::string& GetProjectLoadError() const { return m_ProjectLoadError; }
        std::string DefaultProjectLocation() const;

        std::unordered_map<size_t, const char*>& GetComponentIconMap()
        {
            return m_ComponentIconMap;
        }

        void FocusCamera(const Vec3& point, float distance, float speed = 1.0f);

        void RecompileShaders();
        void DebugDraw();
        void SelectObject(const Maths::Ray& ray, bool hoveredOnly = false);


        void DrawPreview();
        void SavePreview();
        void RequestThumbnail(String8 asset);
        // A preview is being rendered or written: a request made now would be dropped.
        bool IsThumbnailBusy() const { return m_DrawPreview || m_QueuePreviewSave || m_SavePreviewTexture; }

        static Editor* GetEditor() { return (Editor*)&Application::Get(); }

        Maths::Ray GetScreenRay(int x, int y, Camera* camera, int width, int height);

        TerrainSculpt* GetTerrainSculpt() { return m_TerrainSculpt; }

        // the editor UI's scene / developer menus
        void NewScene(const char* name, bool defaultSetup);
        void ReloadScene();
        void BuildAssetPack();
        bool IsAssetPackBusy();

        void FileOpenCallback(const std::string& filepath);
        // Import a glTF/glb as an entity hierarchy (meshes + lights + cameras), opt-in alternative to the single-entity drop.
        void ImportModelAsScene(const std::string& filepath);
        void ProjectOpenCallback(const std::string& filepath);


        void AddDefaultEditorSettings();
        void SaveEditorSettings();
        void LoadEditorSettings();

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
        void AddAppScriptEntity();
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
            bool m_View2D           = false;
            bool m_FullScreenOnPlay = false;
            float m_SnapAmount      = 1.0f;
            bool m_SleepOutofFocus  = true;
            bool m_InspectorFollows = true; // engine UI: the inspector slides in with a selection, away without
            // engine UI: how the workspace was left
            bool m_UIMinimal        = false;
            bool m_UISceneBarHidden = false;
            bool m_UIPlayWorkspace  = false; // switch to the Play layout (game only) while playing
            bool m_UIGameBarHidden  = false;
            float m_UIAssetTileScale = 1.0f;
            // Renderer features switched OFF in the scene view only (SceneViewOff bits): the scene's own
            // settings, and so the game view and a build, are left alone.
            uint32_t m_SceneViewOff = 0;

            bool m_FullScreenSceneView    = false;
            bool m_FreeAspect             = true;
            float m_FixedAspect           = 1.0f;
            float m_AspectRatio           = 1.0f;
            // Render resolution of the views, relative to their panel (times the quality scale)
            float m_SceneViewScale        = 1.0f;
            float m_GameViewScale         = 1.0f;
            // Game view renders at a fixed size (shown fitted to the panel) instead of the panel's
            bool m_GameFixedRes           = false;
            int m_GameResW                = 1920;
            int m_GameResH                = 1080;

            // Camera Settings
            float m_CameraSpeed = 1000.0f;
            float m_CameraNear  = 0.01f;
            float m_CameraFar   = 1000.0f;

            std::vector<std::string> m_RecentProjects;
            std::vector<std::string> m_RecentScenes;

            struct SceneCameraState
            {
                std::string sceneName;
                float posX = -31.0f, posY = 12.0f, posZ = 51.0f;
                float rotX = 0.0f, rotY = 0.0f, rotZ = 0.0f, rotW = 1.0f;
            };
            TDArray<SceneCameraState> m_SceneCameraStates;
        };

        EditorSettings& GetSettings() { return m_Settings; }
        void SetSceneViewActive(bool active) { m_SceneViewActive = active; }

        // The editor's whole UI is built here (EditorUI.h).
        void OnScreenUI() override;
        // Play / stop / pause, as the toolbar buttons do it.
        void TogglePlay();
        void TogglePause();
        void SaveCurrentScene();
        bool& InspectorFollowsSelection() { return m_Settings.m_InspectorFollows; }
        std::vector<SharedPtr<EditorPanel>>& GetPanels() { return m_Panels; }
        void SetEditorScriptsPath(const std::string& path);
        String8 GetEditorScriptsPath() { return m_EditorScriptPath; }

        SharedPtr<Graphics::Texture2D> GetPreviewTexture() const;

    protected:
        NONCOPYABLE(Editor)
        void EditorFrame();
        bool OnWindowResize(WindowResizeEvent& e);

        uint32_t m_GizmoOperation = 7; // GizmoOperation_Universal
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
        bool m_GizmoUsing          = false;
        System::JobSystem::Context m_AssetPackContext;
        bool m_AssetPackResult = false;

        EditorSettings m_Settings;
        std::vector<SharedPtr<EditorPanel>> m_Panels;
        EditorUI* m_EditorUI = nullptr;

        std::unordered_map<size_t, const char*> m_ComponentIconMap;

        TerrainSculpt* m_TerrainSculpt = nullptr;
        ImportPanel* m_ImportPanel     = nullptr;
        PreviewDraw* m_PreviewDraw;

        Camera* m_CurrentCamera = nullptr;
        EditorCameraController m_EditorCameraController;
        Maths::Transform m_EditorCameraTransform;

        SharedPtr<Camera> m_EditorCamera = nullptr;

        SharedPtr<Graphics::GridRenderer> m_GridRenderer;
        SharedPtr<Graphics::Texture2D>    m_LogoTexture;

        std::string m_LastSceneName;
        std::string m_TempSceneSaveFilePath;
        int m_AutoSaveSettingsTime = 15000;
        String8 m_EditorScriptPath;

        bool m_DrawPreview        = false;
        bool m_SavePreviewTexture = false;
        bool m_QueuePreviewSave   = false;
        String8 m_RequestedThumbnailPath;

        IniFile m_IniFile;
        std::string m_ProjectLoadError;

        bool m_PreviewScreenshot = false;
        bool m_QueuedScenePreviewEnd = false;
    };
}
