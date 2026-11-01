#pragma once
#include <Lumos/Core/Core.h>
#include <Lumos/Core/DataStructures/TDArray.h>
#include <Lumos/Maths/Vector4.h>
#include <Lumos/Graphics/UIDock.h>

namespace Lumos
{
    class Editor;
    class Scene;
    class Camera;
    class TerrainSculpt;
    namespace Maths
    {
        class Transform;
    }
    struct UICodeEditor;
    class SceneViewPanel;
    class GameViewPanel;

    // The editor's whole UI: a dock space holding the shared tool panels, the
    // scene / game views and the editor's own panels.
    class EditorUI
    {
    public:
        void Init(Editor* editor);
        void Shutdown();

        void SetEnabled(bool enabled);
        // a panel by name, case and spaces ignored (--panel=)
        bool FocusPanelByKey(const char* key);
        char m_PendingFocus[64] = { 0 }; // asked for before the layout was loaded
        bool IsEnabled() const { return m_Enabled; }

        // Window area left over by the menu and status bars (px).
        void SetRect(const Vec4& rect) { m_Rect = rect; }
        void OnScreenUI();
        // Nothing is open: only the project hub, over the whole window.
        void OnScreenUINoProject();
        // Recent projects, open and new. newProject = start on the new-project form.
        void ShowProjectHub(bool newProject = false);

        UIDockSpace* GetDockSpace() const { return m_Dock; }
        bool& GizmoLocal() { return m_GizmoLocal; }
        void SaveLayout();

        // Workspaces: named dock layouts for a kind of work, each saved on its own.
        enum Workspace : i32
        {
            Workspace_Layout = 0,
            Workspace_Play,
            Workspace_Debug,
            Workspace_Count
        };
        void SetWorkspace(i32 workspace);
        i32 GetWorkspace() const { return m_Workspace; }
        static const char* WorkspaceName(i32 workspace);
        // Go to the Play workspace when the game starts and back when it stops.
        bool& AutoPlayWorkspace() { return m_AutoPlayWorkspace; }

        // Touch layout: the views fill the window, every other panel is a drawer, everything is a size up
        // and taps act on release. On by default on a phone or tablet.
        void SetTouch(bool touch);
        bool IsTouch() const { return m_Touch; }

        // Focus mode: one panel over the whole window, the bars put away (pointer at the top edge brings the strip back).
        void SetFocusMode(bool focus);
        bool IsFocusMode() const { return m_Focus; }
        // Focus mode on one panel by name; again on the same panel leaves it.
        void ToggleMaximised(const char* panel);
        bool IsMaximised(const char* panel) const;
        // Minimal layout: one panel at a time, picked from the rail. No tabs, cards or drawer bar.
        void SetMinimal(bool minimal);
        bool IsMinimal() const { return m_Minimal; }

        // A guided walk round the editor: each step lights up a panel (by dock name; "" = nothing, "Command Bar" = the strip)
        // and says something about it. Strings are copied. Editor scripts start their own with Editor.StartTour.
        struct TourStep
        {
            const char* Panel = "";
            const char* Title = "";
            const char* Text  = "";
        };
        void StartTour(const TourStep* steps, u32 count);
        void StartDefaultTour();

        // Quit, asking first when scripts or the scene have changes that were not saved.
        void RequestQuit();
        bool HasUnsavedWork() const;
        void NoteSceneSaved();

        // Text file (physical path) as a tab in the Script panel; one already open is brought forward.
        void OpenScript(const char* physicalPath);
        // The tab in front (index < 0) or one by index.
        bool SaveScript(i32 index = -1);
        void SaveAllScripts();
        bool AnyScriptUnsaved() const;

    private:
        static void DrawSceneView(const UIDockPanelContext& context);
        static void DrawGameView(const UIDockPanelContext& context);
        static void DrawEditorSettings(const UIDockPanelContext& context);
        static void DrawScript(const UIDockPanelContext& context);
        void ApplyWorkspaceDefaults(i32 workspace);
        void DrawMenuStrip(float y, float height);
        void DrawRail(const Vec4& rect);
        bool DrawRailTools();
        void DrawSwitcher();
        void UpdateSolo();
        // How far a view's overlays must come in from each edge (left, top, right, bottom) to clear the notch.
        Vec4 ViewInsets(const Vec2& position, const Vec2& size) const;
        void RegisterCommands();

        Editor* m_Editor            = nullptr;
        SceneViewPanel* m_SceneView = nullptr;
        GameViewPanel* m_GameView   = nullptr;
        i32 m_Workspace             = Workspace_Layout;
        i32 m_ReturnWorkspace       = Workspace_Layout;
        bool m_AutoPlayWorkspace    = false; // from the editor ini (UIPlayWorkspace)
        bool m_WasPlaying           = false;
        bool m_HadSelection         = true;
        UIDockSpace* m_Dock         = nullptr;
        Vec4 m_Rect                 = Vec4(0.0f);
        bool m_Enabled              = false;
        bool m_GizmoLocal           = true;
        bool m_VoxelEdit            = false;
        // terrain sculpt / voxel edit over the scene view; true = they had the pointer
        bool SceneTools(Editor* editor, Scene* scene, Camera* camera, Maths::Transform* transform, const Vec2& mouse, const Vec2& size, const Vec2& origin, bool hovered);
        void DrawSculptCard(TerrainSculpt* sculpt, const Vec2& origin, const Vec2& size);
        bool m_CameraDrag           = false;
        bool m_Touch                = false;
        bool m_Focus                = false;
        bool m_Minimal              = false;
        f32 m_BuildMs               = 0.0f; // what building this UI cost last frame
        struct StoredTourStep
        {
            char Panel[48];
            char Title[80];
            char Text[420];
        };
        StoredTourStep m_Tour[16];
        u32 m_TourCount             = 0;
        i32 m_TourAt                = -1; // -1 = no tour running
        void DrawTour();
        // the scene file changed on disk under us (another device through iCloud, another program)
        u64 m_SceneFileTime         = 0;
        bool m_SceneChangedOnDisk   = false;
        u32 m_SceneFileCheck        = 0;
        void CheckSceneFile();
        // A view's render target follows its panel once the panel has stopped changing size: remaking the
        // targets every frame of an open / close animation or a splitter drag is what made those lag.
        struct ViewTarget
        {
            Vec2 Current = Vec2(0.0f);
            Vec2 Pending = Vec2(0.0f);
            i32 Frames   = 0;
            Vec2 Settle(const Vec2& wanted);
        };
        ViewTarget m_SceneTarget;
        ViewTarget m_GameTarget;
        bool m_MaximisedForPlay     = false; // focus mode was entered by pressing play: leave it on stop
        bool m_QuitAsk              = false;
        bool m_AboutOpen            = false;
        float m_KeyboardCover       = 0.0f;
        void DrawAbout();
        void* m_SavedScene          = nullptr; // scene the next two belong to
        u32 m_SavedUndoCount        = 0;       // undo steps there were when it was last saved (or opened)
        void DrawQuitPrompt();
        bool SceneUnsaved() const;
        bool m_FrameExplorerDrawn   = false;
        bool m_PhysicsExplorerDrawn = false;
        Vec4 m_Safe                 = Vec4(0.0f); // left, top, right, bottom, px
        bool m_StripShown           = true;       // touch / focus: the strip is an overlay that comes and goes
        f32 m_StripT                = 1.0f;
        f32 m_StripPull             = -1.0f; // where a pull down from the top edge started
        f32 m_StripPullX            = 0.0f;
        bool m_SwitcherOpen         = false;
        i32 m_SwitcherIndex         = 0;
        bool m_SceneBarHidden       = false; // the overlays on the scene view are put away
        bool m_GameBarHidden        = false;
        f32 m_PlayChipTime          = 0.0f;  // seconds the "Playing" chip has left
        i32 m_PlayChipState         = -1;    // editor state it was last shown for
        f32 m_SpeedShown            = -1.0f; // camera speed last frame, to notice it changing
        f32 m_SpeedFlash            = 0.0f;  // seconds the speed read-out stays up

        // Scene view camera by touch: one finger looks, two pan and pinch, the stick flies. A tap selects.
        struct TouchCamera
        {
            u64 LookId      = 0; // finger turning the view (0 = none)
            u64 SecondId    = 0; // a second one: pan + pinch with the first
            u64 StickId     = 0; // finger on the move stick
            Vec2 LookStart  = Vec2(0.0f);
            Vec2 LookLast   = Vec2(0.0f);
            Vec2 SecondLast = Vec2(0.0f);
            Vec2 Stick      = Vec2(0.0f); // -1..1
            bool Moved      = false;      // the look finger travelled: not a tap
            bool OnGizmo    = false;      // it went down on the gizmo: that drag is the gizmo's
            u64 Seen[10]    = {};         // fingers that were down last frame, wherever they were
            u32 SeenCount   = 0;
        };
        TouchCamera m_TouchCamera;
        // true when a finger was lifted without moving: outTap is where (px in the view)
        bool UpdateTouchCamera(const UIDockPanelContext& context, const Vec2& imageOrigin, const Vec2& imageSize, const Vec4& barRect,
                               bool primaryOnView, bool gizmoHasIt, Vec2& outTap);
        bool m_LayoutLoaded         = false;

        void CloseScript(i32 index);
        void DrawProjectHub(bool standalone);

        bool m_HubOpen          = false;
        bool m_HubNew           = false; // the new-project form rather than the list
        char m_HubFilter[64]    = { 0 };
        char m_NewName[96]      = "NewProject";
        char m_NewLocation[512] = { 0 };
        f32 m_HubScroll         = 0.0f;

        struct ScriptTab
        {
            UICodeEditor* Code = nullptr;
            char Path[512]     = { 0 };
        };
        static constexpr i32 kMaxScripts = 12;
        ScriptTab m_Scripts[kMaxScripts];
        i32 m_ScriptCount      = 0;
        i32 m_ScriptCurrent    = 0;
        i32 m_ScriptClosing    = -1;   // tab waiting on save / discard / cancel
        f32 m_CodeSavedFlash   = 0.0f; // "saved" shown for a moment
    };
}
