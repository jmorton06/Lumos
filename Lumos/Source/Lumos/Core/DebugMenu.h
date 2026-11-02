#pragma once
#include "Core/Core.h"
#include "Core/String.h"
#include "Core/Function.h"
#include "Core/DataStructures/TDArray.h"
#include "Core/OS/KeyCodes.h"
#include "Utilities/TSingleton.h"
#include "Maths/Vector2.h"
#include "Maths/Vector3.h"
#include "Maths/Quaternion.h"

namespace Lumos
{
    struct Arena;
    namespace Graphics
    {
        class SceneRenderer;
    }

    enum class DebugCommandType : u8
    {
        Action, // fire and forget
        Toggle, // bool get/set
        Float,  // ranged float get/set
    };

    struct DebugCommand
    {
        String8 Name     = { 0 };
        String8 Category = { 0 };
        String8 Hint     = { 0 }; // right-aligned hint, usually a key binding

        DebugCommandType Type = DebugCommandType::Action;

        Function<void()> Action;
        Function<bool()> GetBool;
        Function<void(bool)> SetBool;
        Function<float()> GetFloat;
        Function<void(float)> SetFloat;

        float Min = 0.0f;
        float Max = 1.0f;

        i32 Shortcut  = -1; // InputCode::Key, -1 = unbound
        bool IsScript = false;
        u32 UseCount  = 0;
        u64 LastUsed  = 0;
    };

    enum DebugPanel : u32
    {
        DebugPanel_Stats = 0,
        DebugPanel_FrameGraph,
        DebugPanel_Log,
        DebugPanel_Toggles,
        DebugPanel_Tweaks,
        DebugPanel_Entities,
        DebugPanel_Inspector,
        DebugPanel_FrameExplorer,
        DebugPanel_PhysicsExplorer,
        DebugPanel_Count
    };

    // In-game debug overlay: a searchable command palette plus a set of HUD
    // panels. Built on the engine UI, so it works in any game using the engine
    // without pulling in ImGui.
    class LUMOS_EXPORT DebugMenu : public TSingleton<DebugMenu>
    {
        friend class TSingleton<DebugMenu>;

    public:
        void Init();
        void Shutdown();

        void RegisterAction(const char* name, const char* category, Function<void()> action, const char* hint = nullptr, i32 shortcutKey = -1);
        void RegisterToggle(const char* name, const char* category, Function<bool()> get, Function<void(bool)> set, i32 shortcutKey = -1);
        void RegisterFloat(const char* name, const char* category, Function<float()> get, Function<void(float)> set, float minValue, float maxValue);

        // Convenience wrappers around a raw pointer the caller keeps alive.
        void RegisterBool(const char* name, const char* category, bool* value);
        void RegisterFloatPtr(const char* name, const char* category, float* value, float minValue, float maxValue);

        void Unregister(const char* name);
        void MarkNextAsScript(bool isScript) { m_NextIsScript = isScript; }
        // Drops everything registered from Lua - call before tearing the state down.
        void UnregisterScriptCommands();

        bool Run(const char* name);
        // For other launchers that want to list the same commands.
        u32 GetCommandCount() const { return (u32)m_Commands.Size(); }
        const DebugCommand& GetCommandAt(u32 index) const { return m_Commands[index]; }
        void RunCommandAt(u32 index);
        // State of a toggle command (false for anything else).
        bool IsCommandOn(u32 index);
        bool SetToggle(const char* name, bool value);

        void SetEnabled(bool enabled);
        bool IsEnabled() const { return m_Enabled; }

        void OpenPalette();
        void ClosePalette();
        void TogglePalette();
        bool IsPaletteOpen() const { return m_PaletteOpen; }

        void SetPanelVisible(DebugPanel panel, bool visible);
        bool IsPanelVisible(DebugPanel panel) const { return m_Panels[panel]; }

        // Key that opens the palette. Ctrl/Cmd+P always works as well.
        void SetToggleKey(i32 key) { m_ToggleKey = key; }

        void OnUpdate(float dt);
        // After the game update, before rendering: the fly camera and the gizmo
        // both write transforms the game has already had its say on this frame.
        void OnPostUpdate(float dt);
        void OnUI();

        // Selection is by raw entt id; it is revalidated against the registry
        // every frame, so a destroyed entity just clears it.
        void SelectEntity(u32 entityId);
        u32 GetSelectedEntity() const { return m_SelectedEntity; }
        void ClearSelection() { m_SelectedEntity = 0xFFFFFFFFu; }

        // GizmoOperation bits (move / rotate / scale).
        void SetGizmoOperation(u32 operation) { m_GizmoOperation = operation; }
        u32 GetGizmoOperation() const { return m_GizmoOperation; }
        void SetGizmoLocal(bool local) { m_GizmoLocal = local; }
        bool GetGizmoLocal() const { return m_GizmoLocal; }
        void SetGizmoSnap(bool snap) { m_GizmoSnap = snap; }
        bool GetGizmoSnap() const { return m_GizmoSnap; }

        void SetFlyCamera(bool enabled);
        bool IsFlyCamera() const { return m_FlyCamera; }
        // 0 = auto (follow the camera's projection), 1 = 2D pan, 2 = 3D free look.
        void SetFlyMode(i32 mode);
        i32 GetFlyMode() const { return m_FlyMode; }

        // Frame explorer: stop the frame after a render pass (-1 = whole frame).
        void SetPassCutoff(i32 pass);
        i32 GetPassCutoff() const;
        // Move the cutoff to the next / previous pass that has something to do.
        void StepPass(i32 direction);

        // The two explorers as the body of a tool panel (no window of their own, the size they are given).
        // The frame explorer steps `renderer`'s frame. Call the Close one when the panel stops being drawn:
        // it hands the frame / the simulation back.
        void DrawFrameExplorerDocked(Graphics::SceneRenderer* renderer, const Vec2& size);
        void DrawPhysicsExplorerDocked(const Vec2& size);
        void CloseFrameExplorer();
        void ClosePhysicsExplorer();

    private:
        DebugMenu()  = default;
        ~DebugMenu() = default;

        DebugCommand* Find(const char* name);
        void RegisterBuiltIns();
        void RunCommand(DebugCommand& cmd);
        void RefreshResults();
        void DrawPalette();
        void DrawStats();
        void DrawFrameGraph();
        void DrawLog();
        void DrawToggles();
        void DrawTweaks();
        void DrawEntities();
        void DrawInspector();
        void DrawFrameExplorer();
        void DrawPhysicsExplorer();
        Graphics::SceneRenderer* ExplorerRenderer() const;
        Graphics::SceneRenderer* m_ExplorerRenderer = nullptr; // docked: whose frame is being stepped
        Vec2 m_DockedSize                           = Vec2(0.0f);
        void DrawPassTweaks(i32 pass, float wrapWidth);
        void DrawToast();
        void DrawFlyHint();
        void UpdateFlyCamera(float dt, bool acceptInput);
        void UpdateGizmo();

        Arena* m_Arena = nullptr;
        TDArray<DebugCommand> m_Commands;

        struct Result
        {
            u32 index;
            i32 score;
            u64 maskBits[2];
        };
        TDArray<Result> m_Results;

        bool m_Enabled     = true;
        bool m_Initialised = false;
        bool m_PaletteOpen = false;
        bool m_NextIsScript = false;
        bool m_ClaimFocus   = false;
        bool m_Panels[DebugPanel_Count] = {};

        char m_Query[96]   = { 0 };
        char m_LastQuery[96] = { 0 };
        i32 m_Selected     = 0;
        i32 m_ScrolledTo   = -1; // selection the list was last auto-scrolled to
        f32 m_LastMouseX   = 0.0f;
        f32 m_LastMouseY   = 0.0f;
        f32 m_Scroll       = 0.0f;
        f32 m_LogScroll    = 0.0f;
        u64 m_LogSeen      = 0;
        u64 m_UseCounter   = 1;
        i32 m_ToggleKey    = -1;
        bool m_BlockedKeyboard = false;

        static constexpr u32 kFrameHistory = 120;
        f32 m_FrameTimes[kFrameHistory] = { 0.0f };
        u32 m_FrameCursor = 0;
        f32 m_SmoothedMs  = 16.0f;

        char m_ToastText[128] = { 0 };
        f32 m_ToastTime       = 0.0f;

        // Entity browser / inspector
        u32 m_SelectedEntity      = 0xFFFFFFFFu; // entt::null
        char m_EntityFilter[64]   = { 0 };
        char m_NameEdit[64]       = { 0 };
        u32 m_NameEditFor         = 0xFFFFFFFFu;
        f32 m_EntityScroll        = 0.0f;
        f32 m_InspectorScroll     = 0.0f;

        // Frame explorer
        f32 m_ExplorerScroll      = 0.0f;
        f32 m_ExplorerInfoScroll  = 0.0f;
        i32 m_ExplorerScrolledTo  = -1;
        bool m_ExplorerWasOpen    = false;
        bool m_ExplorerShowBuffer = false;

        // Physics explorer
        f32 m_PhysicsInfoScroll = 0.0f;
        i32 m_PhysicsSelected   = -1;
        u32 m_PhysicsSavedFlags = 0;
        bool m_PhysicsWasOpen   = false;
        bool m_PhysicsAutoDraw  = true;

        // Gizmo (see Graphics/Gizmo.h)
        bool m_GizmoEnabled        = true;
        bool m_ShowSelectionBounds = true;
        u32 m_GizmoOperation       = 1; // GizmoOperation_Translate
        bool m_GizmoLocal          = false;
        bool m_GizmoSnap           = false;

        // Fly camera. The pose is owned here and written every frame - reading it
        // back would lose the move to whatever the game's camera script wrote
        // earlier in the same frame.
        bool m_FlyCamera      = false;
        u32 m_FlyCameraEntity = 0xFFFFFFFFu;
        Vec3 m_FlyPos;
        Vec3 m_FlySavedPos;
        Quat m_FlySavedRot;
        f32 m_FlySavedScale   = 0.0f; // orthographic zoom
        f32 m_FlyScale        = 1.0f;
        f32 m_FlyYaw          = 0.0f;
        f32 m_FlyPitch        = 0.0f;
        f32 m_FlySpeed        = 8.0f;
        Vec2 m_FlyLastMouse;
        bool m_FlyLooking     = false;
        bool m_FlyPerspective = false;       // resolved mode for this session
        bool m_FlyForcedPerspective = false; // flipped an ortho camera; restore on exit
        f32 m_FlySavedNear    = 0.0f;
        f32 m_FlySavedFar     = 0.0f;
        i32 m_FlyMode         = 0; // 0 = auto, 1 = 2D, 2 = 3D
        i32 m_StepFrames      = 0;
    };
}
