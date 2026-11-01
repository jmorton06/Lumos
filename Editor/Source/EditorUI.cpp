#include "Editor.h"
#include "ExportPanel.h"
#include "ImportPanel.h"
#include <Lumos/Graphics/UICodeEditor.h>
#include <Lumos/Core/Version.h>
#include <Lumos/Core/BusyOverlay.h>
#include <Lumos/Core/Licences.h>
#include <Lumos/Physics/LumosPhysicsEngine/LumosPhysicsEngine.h>
#include <filesystem>
#include <fstream>
#include "EditorUI.h"
#include "EditorScripts.h"
#include "TerrainSculpt.h"
#include <Lumos/Core/OS/Store.h>
#include <Lumos/Scene/Component/TerrainComponent.h>
#include <Lumos/Scene/Component/VoxelWorldComponent.h>
#include <Lumos/Graphics/Renderers/DebugRenderer.h>
#include "DocsPanel.h"
#include "LuaConsolePanel.h"
#include "TipsPanel.h"
#include <Lumos/Core/Asset/AssetImporter.h>
#include <Lumos/Utilities/Timer.h>
#include <Lumos/Graphics/Light.h>
#include <Lumos/Scene/Component/ModelComponent.h>
#include <Lumos/Scene/Component/SoundComponent.h>
#include <Lumos/Graphics/Renderers/DebugRenderer.h>
#include "EditorPanel.h"
#include "SceneViewPanel.h"
#include "GameViewPanel.h"

#include <Lumos/Core/Application.h>
#include <Lumos/Core/ToolPanels.h>
#include <Lumos/Core/SpriteEditor.h>
#include <Lumos/Core/CommandPalette.h>
#include <Lumos/Core/DebugMenu.h>
#include <Lumos/Core/Engine.h>
#include <Lumos/Core/String.h>
#include <Lumos/Scene/SceneManager.h>
#include <Lumos/Scene/SceneGraph.h>
#include <Lumos/Scene/SceneUndo.h>
#include <Lumos/Core/Undo.h>
#include <Lumos/Core/OS/Input.h>
#include <Lumos/Core/OS/Memory.h>
#include <Lumos/Core/OS/OS.h>
#include <Lumos/Core/Thread.h>
#include <Lumos/Graphics/Gizmo.h>
#include <Lumos/Graphics/UI.h>
#include <Lumos/Graphics/UIInternal.h>
#include <Lumos/Graphics/UITheme.h>
#include <Lumos/Core/OS/FileSystem.h>
#include <Lumos/Graphics/Camera/Camera.h>
#include <Lumos/Graphics/RHI/GraphicsContext.h>
#include <Lumos/Graphics/RHI/Renderer.h>
#include <Lumos/Graphics/RHI/Texture.h>
#include <Lumos/Graphics/Renderers/SceneRenderer.h>
#include <Lumos/Maths/MathsUtilities.h>
#include <Lumos/Maths/Ray.h>
#include <Lumos/Maths/Transform.h>
#include <Lumos/Scene/Scene.h>
#include <Lumos/Scene/Entity.h>
#include <Lumos/ImGui/IconsMaterialDesignIcons.h>

#include <cereal/version.hpp>
#include <entt/entt.hpp>

namespace Lumos
{
    namespace
    {
        constexpr u32 kNoEntity = 0xFFFFFFFFu;

        // The tool panels ask these for the selection, so the editor's own list stays the truth.
        Editor* s_Editor = nullptr;

        Scene* CurrentScene()
        {
            return Application::Get().GetSceneManager() ? Application::Get().GetCurrentScene() : nullptr;
        }

        u32 SelectionGet()
        {
            if(!s_Editor || s_Editor->GetSelected().empty())
                return kNoEntity;
            Entity front = s_Editor->GetSelected().front();
            return front.Valid() ? (u32)entt::to_integral(front.GetHandle()) : kNoEntity;
        }

        void SelectionSet(u32 id, bool additive)
        {
            Scene* scene = CurrentScene();
            if(!s_Editor || !scene)
                return;
            if(id == kNoEntity)
            {
                s_Editor->ClearSelected();
                return;
            }

            Entity entity((entt::entity)id, scene);
            if(!entity.Valid())
                return;

            if(additive)
            {
                if(s_Editor->IsSelected(entity))
                    s_Editor->UnSelect(entity);
                else
                    s_Editor->SetSelected(entity);
                return;
            }
            s_Editor->ClearSelected();
            s_Editor->SetSelected(entity);
        }

        bool SelectionContains(u32 id)
        {
            Scene* scene = CurrentScene();
            if(!s_Editor || !scene || id == kNoEntity)
                return false;
            return s_Editor->IsSelected(Entity((entt::entity)id, scene));
        }

        void SelectionFocus(u32 id)
        {
            Scene* scene = CurrentScene();
            if(!s_Editor || !scene)
                return;
            Entity entity((entt::entity)id, scene);
            if(!entity.Valid())
                return;
            if(Maths::Transform* transform = entity.TryGetComponent<Maths::Transform>())
                s_Editor->FocusCamera(transform->GetWorldPosition(), 4.0f, 2.0f);
        }

        const char* kWorkspaceNames[] = { "Layout", "Play", "Debug" };

        // the touch arrangement is kept apart from the desktop one
        bool s_TouchLayout = false;

        String8 LayoutPath(Arena* arena, i32 workspace)
        {
            return PushStr8F(arena, "%sEditorDock%s%s.json", Application::Get().WritableRoot().c_str(), kWorkspaceNames[workspace], s_TouchLayout ? "Touch" : "");
        }

        // the theme belongs to the person, not the project
        String8 ThemePath(Arena* arena)
        {
            return PushStr8F(arena, "%s/Theme.json", FileSystem::UserDataPath("LumosEditor").c_str());
        }
    }

    // ---- commands -----------------------------------------------------------
    // Everything the workspace can do, by name. The palette is the main way in;
    // the buttons and menus are shortcuts to the same things.

    namespace
    {
        EditorUI* s_Self = nullptr;

        using CommandPalette::CommandDesc;

        void AddCommand(const char* name, const char* category, const char* icon, CommandPalette::RunFn run, u64 arg = 0,
                        const char* hint = nullptr, CommandPalette::StateFn isOn = nullptr)
        {
            CommandDesc desc;
            desc.Name     = name;
            desc.Category = category;
            desc.Icon     = icon;
            desc.Hint     = hint;
            desc.Run      = run;
            desc.Arg      = arg;
            desc.IsOn     = isOn;
            CommandPalette::Register(desc);
        }

        Entity SelectedEntity()
        {
            Scene* scene = CurrentScene();
            const u32 id = SelectionGet();
            if(!scene || id == kNoEntity)
                return Entity();
            return Entity((entt::entity)id, scene);
        }

        // rows that depend on what is typed or what is in the scene
        void ProvideEntities(const char* query)
        {
            Scene* scene = CurrentScene();
            if(!scene || !query[0])
                return;

            auto& registry = scene->GetRegistry();
            auto view      = registry.view<NameComponent>();
            u32 added      = 0;
            for(auto e : view)
            {
                if(added >= 40)
                    break;
                const std::string& name = view.get<NameComponent>(e).name;
                i32 score;
                u64 mask[2];
                if(name.empty() || !CommandPalette::Match(query, name.c_str(), score, mask))
                    continue;

                CommandDesc desc;
                desc.Name     = name.c_str();
                desc.Category = "Entity";
                desc.Icon     = ICON_MDI_CUBE_OUTLINE;
                desc.Arg      = (u64)entt::to_integral(e);
                desc.Run      = [](u64 id)
                {
                    SelectionSet((u32)id, false);
                    SelectionFocus((u32)id);
                    if(s_Self)
                        UIDockFocusPanel(s_Self->GetDockSpace(), "Inspector");
                };
                CommandPalette::AddResult(desc, -4);
                added++;
            }
        }

        void ProvideScenes(const char* query)
        {
            SceneManager* manager = Application::Get().GetSceneManager();
            if(!manager || !query[0])
                return;
            const auto& scenes = manager->GetScenes();
            for(u32 i = 0; i < (u32)scenes.Size() && i < 32; i++)
            {
                if(!scenes[i])
                    continue;
                CommandDesc desc;
                desc.Name     = scenes[i]->GetSceneName().c_str();
                desc.Category = "Scene";
                desc.Icon     = ICON_MDI_MOVIE;
                desc.Arg      = i;
                desc.Run      = [](u64 index) { Application::Get().GetSceneManager()->SwitchScene((int)index); };
                CommandPalette::AddResult(desc, -6);
            }
        }

        // projects opened before: "open <name>"
        void ProvideRecentProjects(const char* query)
        {
            if(!s_Editor || !query[0])
                return;
            const auto& recent = s_Editor->GetSettings().m_RecentProjects;
            for(u32 i = 0; i < (u32)recent.size() && i < 16; i++)
            {
                const char* path = recent[i].c_str();
                const char* name = strrchr(path, '/');
                static char names[16][96];
                snprintf(names[i], sizeof(names[i]), "Open %s", name ? name + 1 : path);
                CommandDesc desc;
                desc.Name     = names[i];
                desc.Category = "Project";
                desc.Icon     = ICON_MDI_FOLDER_OPEN;
                desc.Arg      = i;
                desc.Run      = [](u64 index)
                {
                    const auto& list = s_Editor->GetSettings().m_RecentProjects;
                    if(index < list.size())
                    {
                        const std::string path = list[index]; // the list changes while it opens
                        s_Editor->ProjectOpenCallback(path);
                    }
                };
                CommandPalette::AddResult(desc, -8);
            }
        }

        // the debug overlay's commands (render toggles, physics draw, time scale...) come along for free
        void ProvideDebugCommands(const char* query)
        {
            if(!query[0])
                return;
            DebugMenu& menu = DebugMenu::Get();
            const u32 count = menu.GetCommandCount();
            char name[80];
            char category[24];
            u32 added = 0;
            for(u32 i = 0; i < count && added < 60; i++)
            {
                const DebugCommand& command = menu.GetCommandAt(i);
                if(command.Type == DebugCommandType::Float)
                    continue; // needs a slider, not a row
                snprintf(name, sizeof(name), "%.*s", (int)command.Name.size, (const char*)command.Name.str);
                snprintf(category, sizeof(category), "%.*s", (int)command.Category.size, (const char*)command.Category.str);
                i32 score;
                u64 mask[2];
                if(!CommandPalette::Match(query, name, score, mask))
                    continue;

                CommandDesc desc;
                desc.Name     = name;
                desc.Category = category;
                desc.Icon     = ICON_MDI_TUNE;
                desc.Arg      = i;
                desc.Run      = [](u64 index) { DebugMenu::Get().RunCommandAt((u32)index); };
                if(command.Type == DebugCommandType::Toggle)
                    desc.IsOn = [](u64 index) -> bool { return DebugMenu::Get().IsCommandOn((u32)index); };
                CommandPalette::AddResult(desc, -10);
                added++;
            }
        }
    }

    namespace
    {
        // "@": every entity, by name
        void ModeEntities(const char* query)
        {
            Scene* scene = CurrentScene();
            if(!scene)
                return;
            auto& registry = scene->GetRegistry();
            u32 added      = 0;
            char name[80];
            for(auto [e] : registry.storage<entt::entity>().each())
            {
                if(added >= 100)
                    break;
                const NameComponent* component = registry.try_get<NameComponent>(e);
                if(component && !component->name.empty())
                    snprintf(name, sizeof(name), "%s", component->name.c_str());
                else
                    snprintf(name, sizeof(name), "Entity %u", (u32)entt::to_integral(e));

                i32 score;
                u64 mask[2];
                if(!CommandPalette::Match(query, name, score, mask))
                    continue;

                CommandDesc desc;
                desc.Name     = name;
                desc.Category = "Entity";
                desc.Icon     = registry.all_of<Camera>(e) ? ICON_MDI_CAMERA : ICON_MDI_CUBE_OUTLINE;
                desc.Arg      = (u64)entt::to_integral(e);
                desc.Run      = [](u64 id)
                {
                    SelectionSet((u32)id, false);
                    SelectionFocus((u32)id);
                };
                CommandPalette::AddResult(desc);
                added++;
            }
        }

        // "#": every file in the project
        void ModeAssets(const char* query)
        {
            const u32 count = ToolPanels::GetAssetIndexCount();
            u32 added       = 0;
            for(u32 i = 0; i < count && added < 100; i++)
            {
                const char* path = ToolPanels::GetAssetIndexPath(i);
                const char* file = strrchr(path, '/');
                file             = file ? file + 1 : path;

                i32 score;
                u64 mask[2];
                // the file name first; a folder name can pick it up too
                if(!CommandPalette::Match(query, file, score, mask) && !CommandPalette::Match(query, path, score, mask))
                    continue;

                CommandDesc desc;
                desc.Name     = path;
                desc.Category = "Asset";
                desc.Icon     = ToolPanels::AssetIcon(file, false);
                desc.Arg      = i;
                desc.Always   = true; // matched above, against the file name
                desc.Run      = [](u64 index)
                {
                    const String8& root = FileSystem::Get().GetAssetPath();
                    char full[700];
                    snprintf(full, sizeof(full), "%.*s/%s", (int)root.size, (const char*)root.str, ToolPanels::GetAssetIndexPath((u32)index));
                    s_Editor->FileOpenCallback(full);
                };
                CommandPalette::AddResult(desc, score);
                added++;
            }
        }

        // ":": set something on the selection - ":pos 0 2 0", ":scale 2", ":name Lamp"
        enum SetKind : u64
        {
            Set_Position,
            Set_Rotation,
            Set_Scale,
            Set_Name,
            Set_Active,
        };
        f32 s_SetValues[3]  = { 0.0f, 0.0f, 0.0f };
        char s_SetText[96]  = { 0 };
        bool s_SetFlag      = false;

        void ApplySet(u64 kind)
        {
            Entity entity = SelectedEntity();
            if(!entity.Valid())
                return;

            if(kind == Set_Name)
            {
                SceneUndo::SetName(entity, s_SetText);
                return;
            }
            if(kind == Set_Active)
            {
                entity.SetActive(s_SetFlag);
                return;
            }

            Maths::Transform* transform = entity.TryGetComponent<Maths::Transform>();
            if(!transform)
                return;
            UndoSetLabel("Transform");
            SceneUndo::PushComponent<Maths::Transform>(entity);
            const Vec3 value(s_SetValues[0], s_SetValues[1], s_SetValues[2]);
            if(kind == Set_Position)
                transform->SetLocalPosition(value);
            else if(kind == Set_Rotation)
                transform->SetLocalOrientation(Quat(value));
            else
                transform->SetLocalScale(value);
            transform->SetWorldMatrix(Mat4(1.0f));
            UndoCommit();
        }

        void SetHint(const char* text)
        {
            CommandDesc desc;
            desc.Name     = text;
            desc.Category = "Set";
            desc.Icon     = ICON_MDI_PENCIL;
            desc.Always   = true;
            desc.KeepOpen = true;
            CommandPalette::AddResult(desc);
        }

        void ModeSet(const char* query)
        {
            if(!SelectedEntity().Valid())
            {
                SetHint("Select an entity first");
                return;
            }

            char word[16] = { 0 };
            int consumed  = 0;
            sscanf(query, "%15s%n", word, &consumed);
            const char* rest = query + consumed;
            while(*rest == ' ')
                rest++;
            for(char* c = word; *c; c++)
                *c = (char)tolower((unsigned char)*c);

            auto startsWith = [&](const char* full) { return word[0] && strncmp(full, word, strlen(word)) == 0; };

            const bool position = startsWith("position");
            const bool rotation = startsWith("rotation");
            const bool scale    = startsWith("scale");

            if(position || rotation || scale)
            {
                f32 v[3]    = { 0.0f, 0.0f, 0.0f };
                const int n = sscanf(rest, "%f %f %f", &v[0], &v[1], &v[2]);
                // one number for scale means all three axes
                if(n == 3 || (scale && n == 1))
                {
                    if(n == 1)
                        v[1] = v[2] = v[0];
                    s_SetValues[0] = v[0];
                    s_SetValues[1] = v[1];
                    s_SetValues[2] = v[2];

                    char name[96];
                    snprintf(name, sizeof(name), "Set %s to %.3g, %.3g, %.3g", position ? "position" : (rotation ? "rotation" : "scale"), v[0], v[1], v[2]);
                    CommandDesc desc;
                    desc.Name     = name;
                    desc.Category = "Set";
                    desc.Icon     = ICON_MDI_CHECK;
                    desc.Always   = true;
                    desc.Arg      = position ? Set_Position : (rotation ? Set_Rotation : Set_Scale);
                    desc.Run      = &ApplySet;
                    CommandPalette::AddResult(desc);
                    return;
                }
                SetHint(position ? "pos  x y z" : (rotation ? "rot  x y z   (degrees)" : "scale  x y z   or   scale  s"));
                return;
            }

            if(startsWith("name") && rest[0])
            {
                snprintf(s_SetText, sizeof(s_SetText), "%s", rest);
                char name[128];
                snprintf(name, sizeof(name), "Rename to \"%s\"", s_SetText);
                CommandDesc desc;
                desc.Name     = name;
                desc.Category = "Set";
                desc.Icon     = ICON_MDI_CHECK;
                desc.Always   = true;
                desc.Arg      = Set_Name;
                desc.Run      = &ApplySet;
                CommandPalette::AddResult(desc);
                return;
            }

            if(startsWith("active") && rest[0])
            {
                s_SetFlag = (rest[0] == 'o' && rest[1] == 'n') || rest[0] == '1' || rest[0] == 't' || rest[0] == 'y';
                CommandDesc desc;
                desc.Name     = s_SetFlag ? "Switch the entity on" : "Switch the entity off";
                desc.Category = "Set";
                desc.Icon     = ICON_MDI_CHECK;
                desc.Always   = true;
                desc.Arg      = Set_Active;
                desc.Run      = &ApplySet;
                CommandPalette::AddResult(desc);
                return;
            }

            SetHint("pos  x y z");
            SetHint("rot  x y z   (degrees)");
            SetHint("scale  x y z   or   scale  s");
            SetHint("name  text");
            SetHint("active  on | off");
        }
    }

    // ---- workspaces ------------------------------------------------------------

    const char* EditorUI::WorkspaceName(i32 workspace)
    {
        return (workspace >= 0 && workspace < Workspace_Count) ? kWorkspaceNames[workspace] : "";
    }

    // What a workspace looks like before anyone has rearranged it.
    void EditorUI::SetTouch(bool touch)
    {
        if(touch == m_Touch)
            return;
        // each way of working keeps its own layouts
        ArenaTemp scratch = ScratchBegin(0, 0);
        if(m_LayoutLoaded && m_Dock)
            UIDockSaveLayout(m_Dock, LayoutPath(scratch.arena, m_Workspace));
        m_Touch       = touch;
        s_TouchLayout = touch;
        // the strip waits above the top edge until it is pulled down
        m_StripShown = !touch;
        m_StripT     = touch ? 0.0f : 1.0f;
        if(m_LayoutLoaded && m_Dock && !UIDockLoadLayout(m_Dock, LayoutPath(scratch.arena, m_Workspace)))
            ApplyWorkspaceDefaults(m_Workspace);
        ScratchEnd(scratch);
        if(UI_State* ui = Application::Get().GetScreenUI())
        {
            UI_State* main = GetUIState();
            UISetContext(ui);
            UISetClickOnRelease(touch);
            UISetContext(main);
        }
    }

    void EditorUI::ApplyWorkspaceDefaults(i32 workspace)
    {
        UIDockResetLayout(m_Dock);
        if(m_Touch)
        {
            // the views get the whole window; the rest waits on the bar along the bottom
            for(u32 i = 0; i < UIDockGetPanelCount(m_Dock); i++)
            {
                const char* name = UIDockGetPanelName(m_Dock, i);
                if(strcmp(name, "Scene") != 0 && strcmp(name, "Game") != 0 && strcmp(name, "Script") != 0)
                    UIDockSetPanelDrawer(m_Dock, name, true);
            }
            static const char* kOnBar[] = { "Hierarchy", "Inspector", "Assets", "Console" };
            for(const char* name : kOnBar)
                UIDockSetPanelOpen(m_Dock, name, true);
            // a drawer is most of a small screen
            UIDockSetDrawerSize(m_Dock, nullptr, 0.62f);
            UIDockFocusPanel(m_Dock, workspace == Workspace_Play ? "Game" : "Scene");
            return;
        }
        switch(workspace)
        {
        case Workspace_Play:
            // the game and nothing else; everything is a command away
            UIDockSetPanelOpen(m_Dock, "Hierarchy", false);
            UIDockSetPanelOpen(m_Dock, "Inspector", false);
            UIDockFocusPanel(m_Dock, "Game");
            break;
        case Workspace_Debug:
            UIDockSetPanelDrawer(m_Dock, "Console", false);
            UIDockSetPanelOpen(m_Dock, "Console", true);
            UIDockSetPanelOpen(m_Dock, "Stats", true);
            UIDockFocusPanel(m_Dock, "Scene");
            break;
        default:
            UIDockFocusPanel(m_Dock, "Scene");
            break;
        }
    }

    void EditorUI::SetWorkspace(i32 workspace)
    {
        workspace = Maths::Clamp(workspace, 0, (i32)Workspace_Count - 1);
        if(workspace == m_Workspace || !m_Dock)
            return;

        // each keeps its own arrangement
        ArenaTemp scratch = ScratchBegin(0, 0);
        if(m_LayoutLoaded)
            UIDockSaveLayout(m_Dock, LayoutPath(scratch.arena, m_Workspace));
        m_Workspace = workspace;
        if(!UIDockLoadLayout(m_Dock, LayoutPath(scratch.arena, m_Workspace)))
            ApplyWorkspaceDefaults(m_Workspace);
        ScratchEnd(scratch);

        // whatever the loaded layout says, not what the last workspace left
        m_HadSelection = UIDockIsPanelOpen(m_Dock, "Inspector");
    }

    void EditorUI::RegisterCommands()
    {
        s_Self = this;

        CommandPalette::AddMode('@', "Entities", "  Type an entity name", &ModeEntities);
        CommandPalette::AddMode('#', "Assets", "  Type a file name", &ModeAssets);
        CommandPalette::AddMode(':', "Set", "  pos / rot / scale / name / active", &ModeSet);

        for(i32 i = 0; i < Workspace_Count; i++)
            AddCommand(kWorkspaceNames[i], "Workspace", ICON_MDI_VIEW_GRID, [](u64 workspace) { s_Self->SetWorkspace((i32)workspace); }, (u64)i);
        AddCommand(
            "Inspector Follows Selection", "View", ICON_MDI_INFORMATION_OUTLINE,
            [](u64) { s_Editor->InspectorFollowsSelection() = !s_Editor->InspectorFollowsSelection(); }, 0, nullptr,
            [](u64) -> bool { return s_Editor->InspectorFollowsSelection(); });
        AddCommand(
            "Play Workspace While Playing", "View", ICON_MDI_PLAY, [](u64) { s_Self->AutoPlayWorkspace() = !s_Self->AutoPlayWorkspace(); }, 0, nullptr,
            [](u64) -> bool { return s_Self->AutoPlayWorkspace(); });
        AddCommand(
            "Touch Layout", "View", ICON_MDI_CELLPHONE, [](u64) { s_Self->SetTouch(!s_Self->IsTouch()); }, 0, nullptr,
            [](u64) -> bool { return s_Self->IsTouch(); });
        AddCommand(
            "Minimal Layout", "View", ICON_MDI_VIEW_GRID, [](u64) { s_Self->SetMinimal(!s_Self->IsMinimal()); }, 0, nullptr,
            [](u64) -> bool { return s_Self->IsMinimal(); });
        AddCommand("Maximise Game View", "View", ICON_MDI_FULLSCREEN, [](u64) { s_Self->ToggleMaximised("Game"); }, 0, nullptr,
                   [](u64) -> bool { return s_Self->IsMaximised("Game"); });
        AddCommand("Maximise on Play", "View", ICON_MDI_FULLSCREEN, [](u64) { s_Editor->FullScreenOnPlay() = !s_Editor->FullScreenOnPlay(); }, 0, nullptr,
                   [](u64) -> bool { return s_Editor->FullScreenOnPlay(); });
        AddCommand(
            "Focus Mode", "View", ICON_MDI_FULLSCREEN, [](u64) { s_Self->SetFocusMode(!s_Self->IsFocusMode()); }, 0, "Ctrl Shift F",
            [](u64) -> bool { return s_Self->IsFocusMode(); });
        AddCommand("Editor Tour", "Help", ICON_MDI_INFORMATION_OUTLINE, [](u64) { s_Self->StartDefaultTour(); });
        AddCommand("Rescan Assets", "Assets", ICON_MDI_REFRESH, [](u64) { ToolPanels::RefreshAssetIndex(); });

        // ---- panels ----
        for(u32 i = 0; i < UIDockGetPanelCount(m_Dock); i++)
        {
            AddCommand(UIDockGetPanelName(m_Dock, i), "Panel", ICON_MDI_VIEW_MODULE, [](u64 index)
                       {
                           UIDockSpace* dock = s_Self->GetDockSpace();
                           const char* name  = UIDockGetPanelName(dock, (u32)index);
                           if(UIDockIsPanelDrawer(dock, name))
                               UIDockToggleDrawer(dock, name);
                           else
                               UIDockFocusPanel(dock, name); }, i);
        }
        AddCommand("Console Drawer", "Panel", ICON_MDI_CONSOLE, [](u64) { UIDockToggleDrawer(s_Self->GetDockSpace(), "Console"); }, 0, "Ctrl Space");
        AddCommand("Reset Layout", "Layout", ICON_MDI_REFRESH, [](u64) { UIDockResetLayout(s_Self->GetDockSpace()); });
        AddCommand("Save Layout", "Layout", ICON_MDI_CONTENT_SAVE, [](u64) { s_Self->SaveLayout(); });

        // ---- tools ----
        AddCommand("Select Tool", "Tool", ICON_MDI_CURSOR_DEFAULT, [](u64) { s_Editor->SetGizmoOperation(GizmoOperation_None); }, 0, "Q");
        AddCommand("Move Tool", "Tool", ICON_MDI_ARROW_ALL, [](u64) { s_Editor->SetGizmoOperation(GizmoOperation_Translate); }, 0, "W");
        AddCommand("Rotate Tool", "Tool", ICON_MDI_ROTATE_RIGHT, [](u64) { s_Editor->SetGizmoOperation(GizmoOperation_Rotate); }, 0, "E");
        AddCommand("Scale Tool", "Tool", ICON_MDI_VECTOR_SQUARE, [](u64) { s_Editor->SetGizmoOperation(GizmoOperation_Scale); }, 0, "R");
        AddCommand("Universal Tool", "Tool", ICON_MDI_AXIS_ARROW, [](u64) { s_Editor->SetGizmoOperation(GizmoOperation_Universal); }, 0, "T");
        AddCommand(
            "Local Axes", "Tool", ICON_MDI_CUBE_OUTLINE, [](u64) { s_Self->GizmoLocal() = !s_Self->GizmoLocal(); }, 0, nullptr,
            [](u64) { return s_Self->GizmoLocal(); });
        AddCommand(
            "Snap", "Tool", ICON_MDI_RULER, [](u64) { s_Editor->ToggleSnap(); }, 0, nullptr,
            [](u64) -> bool { return s_Editor->SnapGuizmo(); });
        AddCommand(
            "Grid", "View", ICON_MDI_GRID, [](u64) { s_Editor->ShowGrid() = !s_Editor->ShowGrid(); }, 0, nullptr,
            [](u64) -> bool { return s_Editor->ShowGrid(); });

        // ---- edit ----
        AddCommand("Undo", "Edit", ICON_MDI_ROTATE_LEFT, [](u64) { Lumos::Undo(); }, 0, "Ctrl Z");
        AddCommand("Redo", "Edit", ICON_MDI_ROTATE_RIGHT, [](u64) { Lumos::Redo(); }, 0, "Ctrl Shift Z");
        AddCommand("New Entity", "Create", ICON_MDI_PLUS, [](u64)
                   {
                       Scene* scene = CurrentScene();
                       if(!scene)
                           return;
                       Entity created = scene->CreateEntity("Entity");
                       SceneUndo::EntityCreated(created);
                       SelectionSet((u32)entt::to_integral(created.GetHandle()), false); });
        AddCommand("Delete Selected", "Edit", ICON_MDI_DELETE_OUTLINE, [](u64)
                   {
                       Entity entity = SelectedEntity();
                       if(!entity.Valid())
                           return;
                       s_Editor->ClearSelected();
                       SceneUndo::DestroyEntity(entity); }, 0, "Del");
        AddCommand("Duplicate Selected", "Edit", ICON_MDI_CONTENT_COPY, [](u64)
                   {
                       Entity entity = SelectedEntity();
                       if(entity.Valid())
                           entity.GetScene()->DuplicateEntity(entity); });
        AddCommand("Deselect All", "Edit", ICON_MDI_SELECTION, [](u64) { s_Editor->ClearSelected(); });
        AddCommand("Focus Selected", "View", ICON_MDI_CROSSHAIRS, [](u64) { SelectionFocus(SelectionGet()); }, 0, "F");

        // ---- run ----
        AddCommand("Play / Stop", "Run", ICON_MDI_PLAY, [](u64) { s_Editor->TogglePlay(); });
        AddCommand("Pause", "Run", ICON_MDI_PAUSE, [](u64) { s_Editor->TogglePause(); });
        AddCommand("Save Scene", "Scene", ICON_MDI_CONTENT_SAVE, [](u64) { s_Editor->SaveCurrentScene(); });
        AddCommand("Remove Scene from Project", "Scene", ICON_MDI_CLOSE, [](u64)
            {
                SceneManager* manager = Application::Get().GetSceneManager();
                if(manager->RemoveScene(manager->GetCurrentSceneIndex()))
                    s_Editor->Serialise();
                else
                    LWARN("A project needs at least one scene");
            });

        // ---- project ----
        AddCommand("Projects...", "Project", ICON_MDI_VIEW_GRID, [](u64) { s_Self->ShowProjectHub(false); });
        AddCommand("Open Project...", "Project", ICON_MDI_FOLDER_OPEN, [](u64) { s_Editor->BrowseForProject(); });
        AddCommand("New Project...", "Project", ICON_MDI_PLUS, [](u64) { s_Editor->ShowNewProjectDialog(); });
        AddCommand("Close Project", "Project", ICON_MDI_CLOSE, [](u64) { s_Editor->CloseProject(); });
        AddCommand("Save Script", "Script", ICON_MDI_CONTENT_SAVE, [](u64) { s_Self->SaveScript(); });
        AddCommand("Save All Scripts", "Script", ICON_MDI_CONTENT_SAVE, [](u64) { s_Self->SaveAllScripts(); });

        // ---- look ----
        for(i32 i = 0; i < UIGetToolThemePresetCount(); i++)
            AddCommand(UIGetToolThemePresetName(i), "Theme", ICON_MDI_PALETTE, [](u64 preset) { UISetToolThemePreset((i32)preset); }, (u64)i);
        AddCommand("Edit Theme", "Theme", ICON_MDI_BRUSH, [](u64) { UIDockFocusPanel(s_Self->GetDockSpace(), "Theme"); });

        AddCommand("Quit", "App", ICON_MDI_POWER, [](u64) { Application::Get().ExitApp(); });

        CommandPalette::AddProvider(&ProvideEntities);
        CommandPalette::AddProvider(&ProvideScenes);
        CommandPalette::AddProvider(&ProvideRecentProjects);
        CommandPalette::AddProvider(&ProvideDebugCommands);
    }

    // A model plus everything it pulls in (a .gltf's buffers and images). A thumbnail loads all of it
    // on the main thread - and imports it first if it never has been - so a huge one is not done
    // just because its folder was opened.
    static u64 ModelSourceBytes(const std::string& physical)
    {
        std::error_code ec;
        u64 total = std::filesystem::file_size(physical, ec);
        if(ec)
            return 0;
        if(StringUtilities::GetFilePathExtension(physical) != "gltf")
            return total;

        std::string text;
        {
            std::ifstream file(physical);
            text.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        }
        const std::filesystem::path dir = std::filesystem::path(physical).parent_path();
        for(size_t at = text.find("\"uri\""); at != std::string::npos; at = text.find("\"uri\"", at + 5))
        {
            const size_t open  = text.find('"', text.find(':', at) + 1);
            const size_t close = open == std::string::npos ? std::string::npos : text.find('"', open + 1);
            if(close == std::string::npos)
                break;
            const std::string uri = text.substr(open + 1, close - open - 1);
            if(uri.rfind("data:", 0) != 0)
                total += std::filesystem::file_size(dir / uri, ec);
        }
        return total;
    }

    void EditorUI::Init(Editor* editor)
    {
        m_Editor = editor;

        for(auto& panel : m_Editor->GetPanels())
        {
            if(panel->GetSimpleName() == "Scene")
                m_SceneView = static_cast<SceneViewPanel*>(panel.get());
        }

        m_Dock = UIDockSpaceCreate("editor");
#if defined(LUMOS_PLATFORM_IOS) || defined(LUMOS_PLATFORM_ANDROID)
        m_Touch = true;
#else
        // LUMOS_EDITOR_TOUCH=1: the phone / tablet layout on a desktop, to work on it
        m_Touch = getenv("LUMOS_EDITOR_TOUCH") != nullptr;
#endif
        s_TouchLayout = m_Touch;
        m_StripShown  = !m_Touch;
        m_StripT      = m_Touch ? 0.0f : 1.0f;

        UIDockPanelDesc scene;
        scene.Name     = "Scene";
        scene.Title    = ICON_MDI_EYE " Scene";
        scene.Draw     = &EditorUI::DrawSceneView;
        scene.UserData = this;
        scene.Slot     = UIDockSlot_Centre;
        scene.Flags    = UIDockPanel_NoScroll | UIDockPanel_NoPadding | UIDockPanel_NoClose | UIDockPanel_FullBleed;
        UIDockRegisterPanel(m_Dock, scene);

        for(auto& panel : m_Editor->GetPanels())
        {
            if(panel->GetSimpleName() == "Game")
                m_GameView = static_cast<GameViewPanel*>(panel.get());
        }

        UIDockPanelDesc game;
        game.Name     = "Game";
        game.Title    = ICON_MDI_GAMEPAD_VARIANT " Game";
        game.Draw     = &EditorUI::DrawGameView;
        game.UserData = this;
        game.Slot     = UIDockSlot_Centre;
        game.Flags    = UIDockPanel_NoScroll | UIDockPanel_NoPadding | UIDockPanel_FullBleed;
        UIDockRegisterPanel(m_Dock, game);

        ToolPanels::RegisterPanels(m_Dock, true);

        // the teaching views: step through a frame's render passes / a physics step, on whatever is open
        {
            UIDockPanelDesc frame;
            frame.Name     = "Frame Explorer";
            frame.Title    = ICON_MDI_MOVIE " Frame Explorer";
            frame.UserData = this;
            frame.Slot     = UIDockSlot_Right;
            frame.Open     = false;
            frame.Draw     = [](const UIDockPanelContext& context)
            {
                EditorUI* self = (EditorUI*)context.UserData;
                // the picture being looked at: the game's while it runs or when only its view is up, else the scene view's
                const bool game = Application::Get().GetEditorState() != EditorState::Preview
                    || (UIDockIsPanelVisible(self->m_Dock, "Game") && !UIDockIsPanelVisible(self->m_Dock, "Scene"));
                Graphics::SceneRenderer* renderer = game ? Graphics::SceneRenderer::GetGameRenderer() : Application::Get().GetSceneRenderer();
                DebugMenu::Get().DrawFrameExplorerDocked(renderer, context.Size);
                self->m_FrameExplorerDrawn = true;
            };
            UIDockRegisterPanel(m_Dock, frame);

            UIDockPanelDesc physics;
            physics.Name     = "Physics Explorer";
            physics.Title    = ICON_MDI_SHAPE " Physics Explorer";
            physics.UserData = this;
            physics.Slot     = UIDockSlot_Right;
            physics.Open     = false;
            physics.Draw     = [](const UIDockPanelContext& context)
            {
                EditorUI* self = (EditorUI*)context.UserData;
                DebugMenu::Get().DrawPhysicsExplorerDocked(context.Size);
                self->m_PhysicsExplorerDrawn = true;
            };
            UIDockRegisterPanel(m_Dock, physics);
        }

        // editor panels that draw themselves on the engine UI
        for(auto& panel : m_Editor->GetPanels())
        {
            if(panel->GetSimpleName() == "Export")
            {
                UIDockPanelDesc exportDesc;
                exportDesc.Name     = "Export";
                exportDesc.Title    = ICON_MDI_EXPORT " Export";
                exportDesc.Draw     = [](const UIDockPanelContext& context) { ((ExportPanel*)context.UserData)->OnEditorUI(context); };
                exportDesc.UserData = panel.get();
                exportDesc.Slot     = UIDockSlot_Right;
                exportDesc.Open     = false;
                UIDockRegisterPanel(m_Dock, exportDesc);
            }
        }

        {
            UIDockPanelDesc docs;
            docs.Name  = "Packaging Guide";
            docs.Title = ICON_MDI_BOOK_OPEN_VARIANT " Packaging Guide";
            docs.Draw  = &DocsPanel::Draw;
            docs.Slot  = UIDockSlot_Right;
            docs.Open  = false;
            UIDockRegisterPanel(m_Dock, docs);

            UIDockPanelDesc lua;
            lua.Name  = "Lua Console";
            lua.Title = ICON_MDI_CODE_BRACES " Lua Console";
            lua.Draw  = &LuaConsolePanel::Draw;
            lua.Slot  = UIDockSlot_Bottom;
            lua.Flags = UIDockPanel_NoScroll;
            lua.Open  = false;
            UIDockRegisterPanel(m_Dock, lua);

            UIDockPanelDesc tips;
            tips.Name  = "Support Lumos";
            tips.Title = ICON_MDI_HEART " Support Lumos";
            tips.Draw  = &TipsPanel::Draw;
            tips.Slot  = UIDockSlot_Right;
            tips.Open  = false;
            UIDockRegisterPanel(m_Dock, tips);
        }

        UIDockPanelDesc script;
        script.Name     = "Script";
        script.Title    = ICON_MDI_PENCIL " Script";
        script.Draw     = &EditorUI::DrawScript;
        script.UserData = this;
        script.Slot     = UIDockSlot_Centre;
        script.Flags    = UIDockPanel_NoScroll | UIDockPanel_NoPadding;
        script.Open     = false;
        UIDockRegisterPanel(m_Dock, script);

        UIDockPanelDesc import;
        import.Name     = "Import";
        import.Title    = ICON_MDI_IMPORT " Import";
        import.Draw     = [](const UIDockPanelContext& context) { ((Editor*)context.UserData)->GetImportPanel()->OnEditorUI(context); };
        import.UserData = m_Editor;
        import.Slot     = UIDockSlot_Right;
        import.Open     = false;
        UIDockRegisterPanel(m_Dock, import);

        UIDockPanelDesc settings;
        settings.Name     = "Editor Settings";
        settings.Title    = ICON_MDI_WRENCH " Editor Settings";
        settings.Draw     = &EditorUI::DrawEditorSettings;
        settings.UserData = this;
        settings.Slot     = UIDockSlot_Right;
        settings.Open     = false;
        UIDockRegisterPanel(m_Dock, settings);

        // files: the browser and the palette both end up here
        ToolPanels::AssetHooks assetHooks;
        assetHooks.Open = [](const char* path)
        {
            if(s_Editor)
                s_Editor->FileOpenCallback(path);
        };
        assetHooks.Preview = [](const char* vfsPath, char* outPng, u32 size) -> bool
        {
            if(!s_Editor || strncmp(vfsPath, "//Assets/", 9) != 0)
                return false;
            // previews live in Assets/Cache, named after the file they show - or after its imported copy
            const std::string assets   = Application::Get().GetProjectSettings().m_ProjectRoot + "Assets/";
            const std::string source   = vfsPath;
            const std::string imported = AssetImporter::GetImportedPath(source);
            const bool useImported     = !imported.empty() && imported.compare(0, 9, "//Assets/") == 0 && FileSystem::Get().FileExistsVFS(Str8StdS(imported));
            const std::string names[2] = { source, useImported ? imported : std::string() };
            for(const std::string& name : names)
            {
                if(name.empty())
                    continue;
                const std::string png = assets + "Cache/" + name.substr(9) + "_thumbnail.png";
                if(FileSystem::FileExists(Str8StdS(png)))
                {
                    snprintf(outPng, size, "%s", png.c_str());
                    return true;
                }
            }

            // not made yet: ask once, when the renderer is free (a file that will not load is not asked for again)
            static u64 s_Asked[256];
            static u32 s_AskedCount = 0;
            u64 hash                = 14695981039346656037ull;
            for(const char* c = vfsPath; *c; c++)
                hash = (hash ^ (u8)*c) * 1099511628211ull;
            for(u32 i = 0; i < s_AskedCount; i++)
                if(s_Asked[i] == hash)
                    return false;
            if(s_Editor->IsThumbnailBusy())
                return false;
            if(s_AskedCount < 256)
                s_Asked[s_AskedCount++] = hash;

            constexpr u64 kMaxPreviewBytes = 256ull * 1024 * 1024;
            String8 physical;
            ArenaTemp sizeScratch = ScratchBegin(0, 0);
            const bool resolved   = FileSystem::Get().ResolvePhysicalPath(sizeScratch.arena, Str8C(vfsPath), &physical);
            const u64 bytes       = resolved ? ModelSourceBytes(std::string((const char*)physical.str, physical.size)) : 0;
            ScratchEnd(sizeScratch);
            if(bytes > kMaxPreviewBytes)
            {
                LINFO("No thumbnail for %s: %.0f MB with its files is too much to load just to draw one", vfsPath, double(bytes) / (1024.0 * 1024.0));
                return false;
            }
            const std::string& ask = useImported ? imported : source;
            ArenaTemp scratch      = ScratchBegin(0, 0);
            s_Editor->RequestThumbnail(PushStr8Copy(scratch.arena, Str8StdS(ask)));
            ScratchEnd(scratch);
            return false;
        };
        assetHooks.ImportAsScene = [](const char* vfsPath)
        {
            if(s_Editor)
                s_Editor->ImportModelAsScene(vfsPath);
        };
        assetHooks.Reimport = [](const char* vfsPath)
        {
            // with the settings it was imported with last time; outside the frame so the overlay paints
            std::string path = vfsPath;
            Busy::Run("Reimporting", [path]()
                      {
                AssetImporter::Import(path);
                Graphics::ModelComponent::ReloadSceneModels(path); });
        };
        ToolPanels::SetAssetHooks(assetHooks);

        // always there, so any tab can be dropped on it
        UIDockSetDrawerBarAlwaysVisible(m_Dock, true);
        s_Editor = m_Editor;
        RegisterCommands();
        EditorScripts::Init(editor, m_Dock, this);

        // how it was left
        {
            const auto& settings = m_Editor->GetSettings();
            m_Minimal            = settings.m_UIMinimal;
            m_SceneBarHidden     = settings.m_UISceneBarHidden;
            m_GameBarHidden      = settings.m_UIGameBarHidden;
            m_AutoPlayWorkspace  = settings.m_UIPlayWorkspace;
            ToolPanels::SetAssetTileScale(settings.m_UIAssetTileScale);
        }

        ArenaTemp scratch = ScratchBegin(0, 0);
        UILoadToolTheme(ThemePath(scratch.arena));
        UISetToolThemePlain(m_Minimal);
        ScratchEnd(scratch);
    }

    void EditorUI::SaveLayout()
    {
        if(!m_Dock)
            return;
        ArenaTemp scratch = ScratchBegin(0, 0);
        UIDockSaveLayout(m_Dock, LayoutPath(scratch.arena, m_Workspace));
        UISaveToolTheme(ThemePath(scratch.arena));
        ScratchEnd(scratch);
    }

    void EditorUI::Shutdown()
    {
        if(m_Enabled)
            SaveLayout();
        EditorScripts::Shutdown();
        s_Self = nullptr;
        CommandPalette::Clear();
        ToolPanels::ReleaseThumbnails();
        for(i32 i = 0; i < m_ScriptCount; i++)
            UICodeEditorDestroy(m_Scripts[i].Code);
        m_ScriptCount = 0;
        if(s_Editor == m_Editor)
        {
            s_Editor = nullptr;
            ToolPanels::SetSelectionHooks(ToolPanels::SelectionHooks());
        }
        DocsPanel::Release();
        LuaConsolePanel::Release();
        UIDockSpaceDestroy(m_Dock);
        m_Dock = nullptr;
    }

    bool EditorUI::FocusPanelByKey(const char* key)
    {
        auto squash = [](const char* in, char* out, size_t size)
        {
            size_t n = 0;
            for(; *in && n + 1 < size; in++)
                if(*in != ' ')
                    out[n++] = (char)tolower((unsigned char)*in);
            out[n] = 0;
        };
        if(!m_Enabled)
        {
            snprintf(m_PendingFocus, sizeof(m_PendingFocus), "%s", key);
            return true;
        }
        char want[64], have[64];
        squash(key, want, sizeof(want));
        for(u32 i = 0; i < UIDockGetPanelCount(m_Dock); i++)
        {
            const char* name = UIDockGetPanelName(m_Dock, i);
            squash(name, have, sizeof(have));
            if(strcmp(want, have) == 0)
            {
                UIDockFocusPanel(m_Dock, name);
                return true;
            }
        }
        LWARN("--panel=%s: no such panel", key);
        return false;
    }

    void EditorUI::SetEnabled(bool enabled)
    {
        if(enabled == m_Enabled || !m_Editor)
            return;
        m_Enabled = enabled;

        Application::Get().EnableScreenUI(enabled);

        if(enabled)
        {
            // the saved layout lives with the project, which is not known until now
            if(!m_LayoutLoaded)
            {
                m_LayoutLoaded    = true;
                ArenaTemp scratch = ScratchBegin(0, 0);
                if(!UIDockLoadLayout(m_Dock, LayoutPath(scratch.arena, m_Workspace)))
                    ApplyWorkspaceDefaults(m_Workspace);
                ScratchEnd(scratch);
            }
            if(m_PendingFocus[0])
            {
                FocusPanelByKey(m_PendingFocus);
                m_PendingFocus[0] = 0;
            }

            if(m_Touch)
            {
                if(UI_State* screen = Application::Get().GetScreenUI())
                {
                    UI_State* main = GetUIState();
                    UISetContext(screen);
                    UISetClickOnRelease(true);
                    UISetContext(main);
                }
            }

            // LUMOS_EDITOR_PALETTE=<query>: start with the palette up (screenshots, tests)
            if(const char* query = getenv("LUMOS_EDITOR_PALETTE"))
                CommandPalette::Open(query);

            s_Editor = m_Editor;
            ToolPanels::SelectionHooks hooks;
            hooks.Get        = &SelectionGet;
            hooks.Set        = &SelectionSet;
            hooks.IsSelected = &SelectionContains;
            hooks.Focus      = &SelectionFocus;
            ToolPanels::SetSelectionHooks(hooks);
        }
        else
        {
            SaveLayout();

            ToolPanels::SetSelectionHooks(ToolPanels::SelectionHooks());
            CommandPalette::Close();
        }
    }

    namespace
    {
        // A plain quad at a place in the window.
        void FillRect(const char* id, const Vec4& rect, const Vec4& colour, f32 radius = 0.0f)
        {
            UI_State* ui = GetUIState();
            UIBeginChild(id, { SizeKind_Pixels, rect.z }, { SizeKind_Pixels, rect.w }, WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y);
            UI_Widget* quad                            = ui->parents.Back();
            quad->relative_position                    = Vec2(rect.x, rect.y) - ui->root_parent.position;
            quad->style_vars[StyleVar_BackgroundColor] = colour;
            quad->style_vars[StyleVar_CornerRadius]    = Vec4(radius, 0.0f, 0.0f, 0.0f);
            quad->style_vars[StyleVar_Border]          = Vec4(0.0f);
            UIEndChild();
        }

#if defined(LUMOS_PLATFORM_WINDOWS) || defined(LUMOS_PLATFORM_LINUX)
#define LUMOS_EDITOR_WINDOW_BUTTONS
#endif

        // The window has no title bar of its own there: minimise, maximise, close.
        void DrawWindowButtons()
        {
#ifdef LUMOS_EDITOR_WINDOW_BUTTONS
            UISpacer(6.0f);
            if(UIIconButton(ICON_MDI_WINDOW_MINIMIZE "###windowmin", 28.0f).clicked)
                OS::Get().IconifyWindow();
            UITooltip("Minimise");
            const bool maximised = OS::Get().IsWindowMaximised();
            if(UIIconButton(maximised ? ICON_MDI_WINDOW_RESTORE "###windowmax" : ICON_MDI_WINDOW_MAXIMIZE "###windowmax", 28.0f).clicked)
            {
                if(maximised)
                    OS::Get().RestoreWindow();
                else
                    OS::Get().MaximiseWindow();
            }
            UITooltip(maximised ? "Restore" : "Maximise");
            if(UIIconButton(ICON_MDI_WINDOW_CLOSE "###windowclose", 28.0f).clicked)
            {
                if(s_Self)
                    s_Self->RequestQuit();
                else
                    Application::Get().SetAppState(AppState::Closing);
            }
            UITooltip("Close");
#endif
        }

        // ...and the empty part of the bar moves it. onBar = the pointer is on the bar and on nothing in it.
        void HandleWindowDrag(bool onBar)
        {
#ifdef LUMOS_EDITOR_WINDOW_BUTTONS
            static bool s_Armed = false, s_Dragging = false, s_System = false;
            static Vec2 s_Press      = Vec2(0.0f);
            static f64 s_LastPress   = -1.0;
            Input& input             = Input::Get();
            const Vec2 mouse         = UIMousePosition();
            if(onBar && input.GetMouseClicked(InputCode::MouseKey::ButtonLeft))
            {
                const f64 now        = Engine::GetTimeStep().GetElapsedSeconds();
                const bool twice     = s_LastPress >= 0.0 && now - s_LastPress < 0.3;
                s_LastPress          = twice ? -1.0 : now;
                s_Armed              = true;
                s_Press              = mouse;
                if(twice)
                {
                    s_Armed = false;
                    if(OS::Get().IsWindowMaximised())
                        OS::Get().RestoreWindow();
                    else
                        OS::Get().MaximiseWindow();
                }
            }
            if(!input.GetMouseHeld(InputCode::MouseKey::ButtonLeft))
            {
                s_Armed = s_Dragging = s_System = false;
                return;
            }
            const Vec2 moved = mouse - s_Press;
            if(s_Armed && !s_Dragging && moved.x * moved.x + moved.y * moved.y > 16.0f)
            {
                s_Dragging = true;
                // a window manager that takes the drag over drives it itself
                s_System = OS::Get().BeginWindowDrag();
            }
            if(s_Dragging && !s_System)
                OS::Get().UpdateWindowDrag();
#else
            (void)onBar;
#endif
        }
    }

    void EditorUI::DrawMenuStrip(float y, float height)
    {
        UI_State* ui  = GetUIState();
        const f32 dpi = ui->DPIScale;

        const UIToolTheme& theme = UIGetToolTheme();
        const f32 margin         = floorf(theme.OuterMargin * dpi);
        Application& app         = Application::Get();
        const EditorState state  = app.GetEditorState();
        const bool playing       = state != EditorState::Preview;

        UIPushStyle(StyleVar_FontSize, theme.FontSize * dpi);

        // as an overlay it starts under the notch: the fill covers that, the buttons sit below it
        const bool overlay = m_Touch || m_Focus;
        const f32 safeTop  = overlay ? m_Safe.y : 0.0f;
        // a touch layout has no canvas showing anywhere else: the bar is the colour of the panels
        const Vec4 chrome  = m_Touch ? theme.Panel : theme.Canvas;
        if(safeTop > 0.0f)
            FillRect("##editorstripsafe", Vec4(m_Rect.x, y, m_Rect.z, safeTop + 1.0f), chrome);

        // clickable: a press on the bar is not a press on the view under it
        UIBeginChild("##editorstrip", { SizeKind_Pixels, m_Rect.z }, { SizeKind_Pixels, height },
                     WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY | WidgetFlags_DrawBackground | WidgetFlags_Clickable
                         | WidgetFlags_Floating_X | WidgetFlags_Floating_Y,
                     true);
        UI_Widget* bar = ui->parents.Back();
        {
            bar->relative_position                = Vec2(m_Rect.x, y + safeTop) - ui->root_parent.position;
            bar->style_vars[StyleVar_Padding]     = Vec4(margin + 4.0f * dpi, 0.0f, 0.0f, 0.0f);
            bar->style_vars[StyleVar_ItemSpacing] = Vec4(4.0f * dpi, 0.0f, 0.0f, 0.0f);
            // a bar, not a card: square on every platform
            bar->style_vars[StyleVar_CornerRadius] = Vec4(0.0f);
            UIClearBorders(bar);
            UISetBackground(bar, chrome, chrome, chrome);
        }
        if(m_Safe.x > 0.0f)
            UISpacer(m_Safe.x / dpi);

        // ---- left: everything-else menu, where you are ----
#ifdef LUMOS_PLATFORM_MACOS
        // the window's own buttons sit over this corner unless it is fullscreen
        if(!OS::Get().IsWindowFullscreen())
            UISpacer(78.0f);
#endif
        // the logo is the menu button
        if(Graphics::Texture2D* logo = m_Editor->GetLogoTexture())
        {
            if(UIImageButton("##editormenubutton", logo, Vec2(24.0f * dpi, 24.0f * dpi)).clicked)
                UIOpenPopup("##editormenu");
        }
        else if(UIIconButton(ICON_MDI_MENU "###editormenubutton", 28.0f).clicked)
            UIOpenPopup("##editormenu");
        UITooltip("Menu");

        // project / scene: each is a way to change it
        {
            Scene* scene = CurrentScene();
            auto crumb   = [&](const char* label)
            {
                u64 hash;
                String8 text      = HandleUIString(label, &hash);
                UI_Widget* button = PushWidget(WidgetFlags_DrawText | WidgetFlags_DrawBackground | WidgetFlags_Clickable, text, hash,
                                               { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
                UIClearBorders(button);
                button->style_vars[StyleVar_Padding]      = Vec4(7.0f * dpi, 4.0f * dpi, 0.0f, 0.0f);
                button->style_vars[StyleVar_CornerRadius] = Vec4(theme.WidgetRounding * dpi + 2.0f * dpi, 0.0f, 0.0f, 0.0f);
                UISetBackground(button, Vec4(0.0f), theme.Hover, theme.Selection);
                UISetTextColour(button, theme.TextDim);
                return HandleWidgetInteraction(button).clicked;
            };

            char label[128];
            // a narrow window keeps the scene and drops the project (it is in the menu)
            if(!m_Touch)
            {
                snprintf(label, sizeof(label), "%s###crumbproject", app.GetProjectSettings().m_ProjectName.c_str());
                if(crumb(label))
                    ShowProjectHub(false);
                UITooltip("Projects");

                UIPushStyle(StyleVar_TextColor, theme.TextDim);
                UILabelCStr("##crumbslash", "/");
                UIPopStyle(StyleVar_TextColor);
            }

            snprintf(label, sizeof(label), "%s  " ICON_MDI_CHEVRON_DOWN "###crumbscene", scene ? scene->GetSceneName().c_str() : "-");
            if(crumb(label))
                UIOpenPopup("##crumbscenes");
            UITooltip("Switch scene");
            if(UIBeginPopup("##crumbscenes"))
            {
                SceneManager* manager = app.GetSceneManager();
                const auto& scenes    = manager->GetScenes();
                i32 switchTo          = -1;
                for(u32 i = 0; i < (u32)scenes.Size() && i < 64; i++)
                {
                    if(!scenes[i])
                        continue;
                    bool current = scenes[i].get() == scene;
                    char item[128];
                    snprintf(item, sizeof(item), "%s###crumbscene%u", scenes[i]->GetSceneName().c_str(), i);
                    if(UIMenuItem(item, nullptr, &current) && scenes[i].get() != scene)
                        switchTo = (i32)i;
                }
                UIMenuSeparator();
                if(UIMenuItem(ICON_MDI_CONTENT_SAVE "  Save Scene", "Ctrl S"))
                    m_Editor->SaveCurrentScene();
                if(UIMenuItem("Scene Settings..."))
                    UIDockFocusPanel(m_Dock, "Scene Settings");
                // the file stays in Assets/Scenes: only the project's list changes
                i32 removeAt = -1;
                if(scenes.Size() > 1 && UIBeginMenu(ICON_MDI_CLOSE "  Remove from Project"))
                {
                    for(u32 i = 0; i < (u32)scenes.Size() && i < 64; i++)
                    {
                        if(!scenes[i])
                            continue;
                        char item[128];
                        snprintf(item, sizeof(item), "%s###crumbremove%u", scenes[i]->GetSceneName().c_str(), i);
                        if(UIMenuItem(item))
                            removeAt = (i32)i;
                    }
                    UIEndMenu();
                }
                UITooltip("Takes the scene out of the project's list. The .lsn file is kept.");
                UIEndPopup();
                // after the menu: the switch tears down what was just listed
                if(switchTo >= 0)
                    manager->SwitchScene(switchTo);
                if(removeAt >= 0)
                {
                    manager->RemoveScene((u32)removeAt);
                    m_Editor->Serialise(); // the .lmproj keeps the list
                }
            }
        }

        UISpacer(10.0f);

        // ---- middle: the way into everything ----
        UIFlexSpacer();
        // whatever is left between the two ends on a narrow window
        const bool narrow  = m_Rect.z / dpi < 520.0f;
        const f32 barWidth = m_Touch ? Maths::Clamp(m_Rect.z / dpi - (narrow ? 247.0f : 362.0f), 80.0f, 460.0f) : 460.0f;
        CommandPalette::DrawTrigger("##editorcommand", barWidth, m_Touch ? "Search" : "Search commands, panels, entities");
        UIFlexSpacer();

        // ---- right: history and play ----
        // (a phone held upright has no room for undo / redo: they are in the menu)
        if(!narrow)
        {
        UIPushStyle(StyleVar_TextColor, UndoStepCount() > 0 ? theme.Text : theme.TextDim);
        if(UIIconButton(ICON_MDI_ROTATE_LEFT "###editorundo", 28.0f).clicked)
            Lumos::Undo();
        UIPopStyle(StyleVar_TextColor);
        UITooltip("Undo");
        UIPushStyle(StyleVar_TextColor, RedoStepCount() > 0 ? theme.Text : theme.TextDim);
        if(UIIconButton(ICON_MDI_ROTATE_RIGHT "###editorredo", 28.0f).clicked)
            Lumos::Redo();
        UIPopStyle(StyleVar_TextColor);
        UITooltip("Redo");

        UISpacer(10.0f);
        }
        if(UIIconButton(playing ? ICON_MDI_STOP "###editorplay" : ICON_MDI_PLAY "###editorplay", 28.0f, playing).clicked)
            m_Editor->TogglePlay();
        UITooltip(playing ? "Stop" : "Play");
        if(UIIconButton(ICON_MDI_PAUSE "###editorpause", 28.0f, state == EditorState::Paused).clicked)
            m_Editor->TogglePause();
        UITooltip("Pause");

        if(UIIconButton(ICON_MDI_SETTINGS "###editorsettings", 28.0f).clicked)
            UIOpenPopup("##editorsettingsmenu");
        UITooltip("Settings");
        if(UIBeginPopup("##editorsettingsmenu"))
        {
            if(UIMenuItem(ICON_MDI_WRENCH "  Editor Settings"))
                UIDockFocusPanel(m_Dock, "Editor Settings");
            if(UIMenuItem(ICON_MDI_FOLDER "  Project Settings"))
                UIDockFocusPanel(m_Dock, "Project Settings");
            if(UIMenuItem(ICON_MDI_IMAGE "  Scene Settings"))
                UIDockFocusPanel(m_Dock, "Scene Settings");
            UIEndPopup();
        }

        DrawWindowButtons();
        if(m_Safe.z > 0.0f)
            UISpacer(m_Safe.z / dpi);

        UIEndChild();
        HandleWindowDrag(ui->hot_widget == bar->hash);

        // ---- the menu behind the button: the less common things, as lists ----
        if(!UIBeginPopup("##editormenu"))
        {
            UIPopStyle(StyleVar_FontSize);
            return;
        }

        if(UIBeginMenu("File"))
        {
            if(UIMenuItem(ICON_MDI_PLUS "  New Project..."))
                m_Editor->ShowNewProjectDialog();
            if(UIMenuItem(ICON_MDI_FOLDER_OPEN "  Open Project..."))
                m_Editor->BrowseForProject();
            const auto& recent = m_Editor->GetSettings().m_RecentProjects;
            if(UIBeginMenu("Open Recent", !recent.empty()))
            {
                // newest last in the list: show it first
                for(i32 i = (i32)recent.size() - 1; i >= 0 && i >= (i32)recent.size() - 10; i--)
                {
                    const char* path = recent[i].c_str();
                    const char* name = strrchr(path, '/');
                    char item[160];
                    snprintf(item, sizeof(item), "%s###recent%d", name ? name + 1 : path, i);
                    if(UIMenuItem(item))
                    {
                        const std::string open = recent[i];
                        m_Editor->ProjectOpenCallback(open);
                        break;
                    }
                }
                UIEndMenu();
            }
            if(UIMenuItem(ICON_MDI_VIEW_GRID "  Projects..."))
                ShowProjectHub(false);
            UIMenuSeparator();
            if(UIMenuItem(ICON_MDI_CONTENT_SAVE "  Save Scene", "Ctrl S"))
                m_Editor->SaveCurrentScene();
            if(UIMenuItem("Save All Scripts", nullptr, nullptr, AnyScriptUnsaved()))
                SaveAllScripts();
            UIMenuSeparator();
            if(UIMenuItem(ICON_MDI_EXPORT "  Export..."))
                UIDockFocusPanel(m_Dock, "Export");
            if(UIMenuItem("Close Project"))
                m_Editor->CloseProject();
#if !defined(LUMOS_PLATFORM_IOS) && !defined(LUMOS_PLATFORM_ANDROID)
            UIMenuSeparator();
            if(UIMenuItem(ICON_MDI_EXIT_TO_APP "  Quit"))
                RequestQuit();
#endif
            UIEndMenu();
        }

        if(UIBeginMenu("Edit"))
        {
            const char* undoLabel = UndoStepLabel(0);
            const char* redoLabel = RedoStepLabel(0);
            char item[96];
            snprintf(item, sizeof(item), "Undo %s###undo", undoLabel ? undoLabel : "");
            if(UIMenuItem(item, "Ctrl Z", nullptr, UndoStepCount() > 0))
                Lumos::Undo();
            snprintf(item, sizeof(item), "Redo %s###redo", redoLabel ? redoLabel : "");
            if(UIMenuItem(item, "Ctrl Shift Z", nullptr, RedoStepCount() > 0))
                Lumos::Redo();
            UIMenuSeparator();
            if(UIMenuItem("Editor Settings"))
                UIDockFocusPanel(m_Dock, "Editor Settings");
            if(UIMenuItem("Project Settings"))
                UIDockFocusPanel(m_Dock, "Project Settings");
            if(UIMenuItem("Scene Settings"))
                UIDockFocusPanel(m_Dock, "Scene Settings");
            UIEndMenu();
        }

        if(UIBeginMenu("View"))
        {
            bool minimal = m_Minimal;
            if(UIMenuItem("Minimal", nullptr, &minimal))
                SetMinimal(minimal);
            bool focus = m_Focus;
            if(UIMenuItem("Focus Mode", "Ctrl Shift F", &focus))
                SetFocusMode(focus);
            UIMenuItem("Maximise Game on Play", nullptr, &m_Editor->FullScreenOnPlay());
            UIMenuItem("Play Workspace While Playing", nullptr, &m_AutoPlayWorkspace);
            UIMenuSeparator();
            if(UIBeginMenu("Layout"))
            {
                for(i32 i = 0; i < Workspace_Count; i++)
                {
                    bool current = i == m_Workspace;
                    char item[48];
                    snprintf(item, sizeof(item), "%s###workspace%d", kWorkspaceNames[i], i);
                    if(UIMenuItem(item, nullptr, &current))
                        SetWorkspace(i);
                }
                UIMenuSeparator();
                if(UIMenuItem("Reset Layout"))
                    UIDockResetLayout(m_Dock);
                if(UIMenuItem("Save Layout"))
                    SaveLayout();
                UIEndMenu();
            }
            if(UIBeginMenu("Theme"))
            {
                const i32 current = UIGetToolThemePreset();
                for(i32 i = 0; i < UIGetToolThemePresetCount(); i++)
                {
                    bool selected = (i == current);
                    char item[64];
                    snprintf(item, sizeof(item), "%s###theme%d", UIGetToolThemePresetName(i), i);
                    if(UIMenuItem(item, nullptr, &selected))
                        UISetToolThemePreset(i);
                }
                UIMenuSeparator();
                if(UIMenuItem("Edit Theme..."))
                    UIDockFocusPanel(m_Dock, "Theme");
                UIEndMenu();
            }
            UIEndMenu();
        }

        if(UIBeginMenu("Window"))
        {
            for(u32 i = 0; i < UIDockGetPanelCount(m_Dock); i++)
            {
                const char* name = UIDockGetPanelName(m_Dock, i);
                bool open        = UIDockIsPanelOpen(m_Dock, name);
                char item[96];
                snprintf(item, sizeof(item), "%s###panel%u", name, i);
                if(UIMenuItem(item, nullptr, &open))
                    UIDockSetPanelOpen(m_Dock, name, open);
            }
            UIMenuSeparator();
            if(UIMenuItem("Console Drawer", "Ctrl Space"))
                UIDockToggleDrawer(m_Dock, "Console");
            UIEndMenu();
        }

        if(UIBeginMenu("Help"))
        {
            if(UIMenuItem("Editor Tour"))
                StartDefaultTour();
            if(UIMenuItem("Packaging Guide"))
                UIDockFocusPanel(m_Dock, "Packaging Guide");
            if(UIMenuItem(ICON_MDI_HEART "  Support Lumos"))
                UIDockFocusPanel(m_Dock, "Support Lumos");
            if(UIMenuItem(ICON_MDI_GITHUB_CIRCLE "  GitHub"))
                OS::Get().OpenURL("https://www.github.com/jmorton06/Lumos");
            UIMenuSeparator();
            if(UIMenuItem(ICON_MDI_INFORMATION_OUTLINE "  About Lumos"))
                m_AboutOpen = true;
            UIEndMenu();
        }

        UIEndPopup();
        UIPopStyle(StyleVar_FontSize);
    }

    void EditorUI::OnScreenUI()
    {
        if(!m_Enabled || !m_Dock || m_Rect.z < 16.0f || m_Rect.w < 16.0f)
            return;

        Timer buildTimer;
        UI_State* ui = GetUIState();
        TipsPanel::Update();
        {
            // kept with the editor's settings (written when it saves them)
            auto& settings              = m_Editor->GetSettings();
            settings.m_UIMinimal        = m_Minimal;
            settings.m_UISceneBarHidden = m_SceneBarHidden;
            settings.m_UIGameBarHidden  = m_GameBarHidden;
            settings.m_UIPlayWorkspace  = m_AutoPlayWorkspace;
            settings.m_UIAssetTileScale = ToolPanels::GetAssetTileScale();

            // a scene just opened has nothing to save
            Scene* scene = CurrentScene();
            if((void*)scene != m_SavedScene)
            {
                m_SavedScene         = scene;
                m_SavedUndoCount     = UndoStepCount();
                m_SceneFileTime      = 0;
                m_SceneChangedOnDisk = false;
            }
            CheckSceneFile();
        }
        // LUMOS_EDITOR_RECT="w,h" in points: the editor in a corner of the window, to try a phone's shape on a desktop
        {
            static const char* s_Rect = getenv("LUMOS_EDITOR_RECT");
            float w = 0.0f, h = 0.0f;
            if(s_Rect && sscanf(s_Rect, "%f,%f", &w, &h) == 2)
                m_Rect = Vec4(0.0f, 0.0f, Maths::Min(w * ui->DPIScale, m_Rect.z), Maths::Min(h * ui->DPIScale, m_Rect.w));
        }
        // the on-screen keyboard: lay the editor out in what it leaves (eased, so the layout does not jump)
        {
            const f32 cover = Maths::Clamp(UIKeyboardCover(), 0.0f, 0.7f);
            m_KeyboardCover += (cover - m_KeyboardCover) * Maths::Min(1.0f, (f32)Engine::GetTimeStep().GetSeconds() * 14.0f);
            if(Maths::Abs(cover - m_KeyboardCover) < 0.002f)
                m_KeyboardCover = cover;
            m_Rect.w *= 1.0f - m_KeyboardCover;
        }
        // fingers need bigger targets than a pointer: everything a size up (set again from the window each frame)
        if(m_Touch)
            ui->DPIScale *= 1.22f;
        const f32 dpi = ui->DPIScale;

        // dark / light and the accent, for the core widgets
        UIApplyToolThemeToContext();

        // status line
        {
            const auto& stats = Engine::Get().Statistics();
            Scene* scene      = CurrentScene();
            const bool portraitStatus = m_Rect.w > m_Rect.z;
            char status[160];
            // a narrow bar has room for the drawers and little else
            if(m_Touch && portraitStatus)
                status[0] = 0;
            else if(m_Touch)
                snprintf(status, sizeof(status), "%u fps  ", stats.FramesPerSecond);
            else
                snprintf(status, sizeof(status), "%u fps    %.1f ms    ui %.1f ms    %u draws    %u tris    %u entities  ",
                         stats.FramesPerSecond, (float)Engine::GetTimeStep().GetRawMillis(), m_BuildMs, stats.NumDrawCalls, stats.TriangleCount,
                         scene ? (u32)scene->GetRegistry().storage<entt::entity>().in_use() : 0u);
            UIDockSetBarText(m_Dock, status);
        }

        // Screenshot / test hooks, applied once the scene has settled:
        // LUMOS_EDITOR_SCENE=<name>, LUMOS_EDITOR_WORKSPACE=<name>, LUMOS_EDITOR_SELECT=<entity name>, LUMOS_EDITOR_DRAWER=<panel>, LUMOS_EDITOR_PANEL=<panel>
        {
            static int s_Frames = 0;
            s_Frames++;
            // LUMOS_EDITOR_SCENE=<scene name>, switched first so the rest see it
            if(s_Frames == 5)
                if(const char* sceneName = getenv("LUMOS_EDITOR_SCENE"))
                    Application::Get().GetSceneManager()->SwitchScene(sceneName);
            // LUMOS_EDITOR_TYPE=<text>: typed into whatever has the keyboard, a little later
            if(s_Frames == 70)
                if(const char* typed = getenv("LUMOS_EDITOR_TYPE"))
                    for(const char* c = typed; *c; c++)
                        UIProcessKeyTyped(*c);
            if(s_Frames == 40)
            {
                if(const char* workspace = getenv("LUMOS_EDITOR_WORKSPACE"))
                    for(i32 i = 0; i < Workspace_Count; i++)
                        if(strcmp(workspace, kWorkspaceNames[i]) == 0)
                            SetWorkspace(i);
                if(const char* select = getenv("LUMOS_EDITOR_SELECT"))
                {
                    if(Scene* scene = CurrentScene())
                    {
                        auto view = scene->GetRegistry().view<NameComponent>();
                        for(auto e : view)
                            if(view.get<NameComponent>(e).name == select)
                                SelectionSet((u32)entt::to_integral(e), false);
                    }
                }
                // LUMOS_EDITOR_CAMERA="x,y,z,pitch,yaw": the scene view's camera (degrees)
                if(const char* cam = getenv("LUMOS_EDITOR_CAMERA"))
                {
                    float v[5] = {};
                    if(sscanf(cam, "%f,%f,%f,%f,%f", &v[0], &v[1], &v[2], &v[3], &v[4]) == 5)
                    {
                        Maths::Transform& t = m_Editor->GetEditorCameraTransform();
                        t.SetLocalPosition(Vec3(v[0], v[1], v[2]));
                        t.SetLocalOrientation(Quat::EulerAnglesToQuaternion(v[3], v[4], 0.0f));
                        t.SetWorldMatrix(Mat4());
                    }
                }
                // LUMOS_EDITOR_GI=1: global illumination on in the open scene (not saved unless the scene is)
                if(getenv("LUMOS_EDITOR_GI"))
                    if(Scene* scene = CurrentScene())
                        scene->GetSettings().RenderSettings.GIEnabled = atoi(getenv("LUMOS_EDITOR_GI")) != 0;
                // LUMOS_EDITOR_LUA="<line>": run like the palette's = mode, e.g. Render.SetBool("SSAO", true)
                if(const char* line = getenv("LUMOS_EDITOR_LUA"))
                    EditorScripts::RunLine(line);
                // LUMOS_EDITOR_SCENEOFF=<SceneViewOff bits>: features left out of the scene view (255 = all)
                if(const char* sceneOff = getenv("LUMOS_EDITOR_SCENEOFF"))
                    m_Editor->GetSettings().m_SceneViewOff = (uint32_t)atoi(sceneOff) & SceneViewOff_All;
                // LUMOS_EDITOR_ASPECT=<width / height>: the game view at that shape
                if(const char* aspect = getenv("LUMOS_EDITOR_ASPECT"))
                {
                    m_Editor->GetSettings().m_FreeAspect  = false;
                    m_Editor->GetSettings().m_FixedAspect = (float)atof(aspect);
                }
                // LUMOS_EDITOR_TOUR=<step number>: the tour, at that stop
                if(const char* tour = getenv("LUMOS_EDITOR_TOUR"))
                {
                    StartDefaultTour();
                    m_TourAt = Maths::Clamp(atoi(tour), 0, (int)m_TourCount - 1);
                }
                // LUMOS_EDITOR_MINIMAL / _FOCUS / _STRIP=1: those layouts, and the pulled-down strip
                if(getenv("LUMOS_EDITOR_MINIMAL"))
                    SetMinimal(true);
                if(getenv("LUMOS_EDITOR_FOCUS"))
                    SetFocusMode(true);
                // LUMOS_EDITOR_MAXGAME=1: the game view maximised
                if(getenv("LUMOS_EDITOR_MAXGAME"))
                    ToggleMaximised("Game");
                if(getenv("LUMOS_EDITOR_STRIP"))
                    m_StripShown = true;
                if(const char* panel = getenv("LUMOS_EDITOR_PANEL"))
                    UIDockFocusPanel(m_Dock, panel);
                // LUMOS_EDITOR_SPRITE=<picture path>[:shape]: open in the Sprite Editor (":shape" = its collision shape)
                if(const char* sprite = getenv("LUMOS_EDITOR_SPRITE"))
                {
                    char path[512];
                    snprintf(path, sizeof(path), "%s", sprite);
                    char* mode = strstr(path, ":shape");
                    if(mode)
                        *mode = 0;
                    if(SpriteEditor::Open(path))
                        SpriteEditor::SetShapeMode(mode != nullptr);
                }
                // LUMOS_EDITOR_SCRIPT=<text file path>[:<another>]: open in the Script panel
                if(const char* scriptPaths = getenv("LUMOS_EDITOR_SCRIPT"))
                {
                    char paths[1024];
                    snprintf(paths, sizeof(paths), "%s", scriptPaths);
                    for(char* path = strtok(paths, ":"); path; path = strtok(nullptr, ":"))
                        OpenScript(path);
                }
                // LUMOS_EDITOR_BAR=1: keyboard into the command bar; LUMOS_EDITOR_OPENPROJECT=1: the project picker
                if(getenv("LUMOS_EDITOR_BAR"))
                    CommandPalette::FocusBar();
                if(getenv("LUMOS_EDITOR_OPENPROJECT"))
                    m_Editor->BrowseForProject();
                // LUMOS_EDITOR_HUB=recent|new: the project hub
                if(const char* hub = getenv("LUMOS_EDITOR_HUB"))
                    ShowProjectHub(strcmp(hub, "new") == 0);
                // LUMOS_EDITOR_CLOSEPROJECT=1: as if nothing was open (the hub fills the window)
                if(getenv("LUMOS_EDITOR_CLOSEPROJECT"))
                    m_Editor->CloseProject();
                // LUMOS_EDITOR_IMPORT=<model path>: the import form for it
                if(const char* importPath = getenv("LUMOS_EDITOR_IMPORT"))
                {
                    m_Editor->GetImportPanel()->Show(importPath);
                    UIDockFocusPanel(m_Dock, "Import");
                }
                if(const char* folder = getenv("LUMOS_EDITOR_ASSETS"))
                    ToolPanels::SetAssetFolder(folder);
                if(const char* drawer = getenv("LUMOS_EDITOR_DRAWER"))
                    UIDockFocusPanel(m_Dock, drawer);
            }
        }

        // the Game panel says so again each frame it is drawn
        if(m_GameView)
            m_GameView->SetExternalVisible(false);
        // ...and only it gives the game the input; otherwise the scene camera would wait for it to be shown once
        Application::Get().SetSceneActive(false);

        // playing has its own workspace: go there on start, come back on stop
        const bool playing = Application::Get().GetEditorState() != EditorState::Preview;
        if(playing != m_WasPlaying)
        {
            m_WasPlaying = playing;
            // (out of a play-time maximise before the layout it came from is put back)
            if(!playing && m_MaximisedForPlay)
            {
                m_MaximisedForPlay = false;
                SetFocusMode(false);
            }
            if(m_AutoPlayWorkspace)
            {
                if(playing)
                {
                    m_ReturnWorkspace = m_Workspace;
                    SetWorkspace(Workspace_Play);
                }
                else
                    SetWorkspace(m_ReturnWorkspace);
            }
            UIDockFocusPanel(m_Dock, playing ? "Game" : "Scene");
            // the game takes the whole window while it runs, when asked to
            if(playing && m_Editor->FullScreenOnPlay() && !m_Focus && !m_Touch)
            {
                ToggleMaximised("Game");
                m_MaximisedForPlay = true;
            }
        }

        // the inspector comes in with a selection and goes when there is none
        if(m_Editor->InspectorFollowsSelection() && !playing && !UIDockIsPanelDrawer(m_Dock, "Inspector"))
        {
            const bool hasSelection = SelectionGet() != kNoEntity;
            if(hasSelection != m_HadSelection)
            {
                m_HadSelection = hasSelection;
                UIDockSetPanelOpen(m_Dock, "Inspector", hasSelection);
            }
        }

        // the project's own editor scripts: (re)loaded when the project changes or one asks
        {
            static std::string s_ScriptRoot;
            const std::string& root = Application::Get().GetProjectSettings().m_ProjectRoot;
            if(root != s_ScriptRoot || EditorScripts::TakeReloadRequest())
            {
                s_ScriptRoot = root;
                EditorScripts::Load(root.c_str());
            }
        }

        // the picture runs to the edges of the screen; only bars and buttons keep out of the notch and corners
        ui->FullBleed = true;
        {
            const SafeAreaInsets sa = UIRawSafeAreaInsets();
            // the home indicator needs less room than the OS asks for
            // (the OS asks for more than the notch and the home indicator take)
            m_Safe = Vec4(floorf(sa.left * 0.9f), floorf(sa.top * 0.9f), floorf(sa.right * 0.9f), floorf(sa.bottom * 0.5f));
        }

        Input& input = Input::Get();
        // Ctrl / Cmd + Shift + F anywhere, Shift + Space when not typing
        {
            const bool ctrl  = input.GetKeyHeldRaw(InputCode::Key::LeftControl) || input.GetKeyHeldRaw(InputCode::Key::RightControl)
                || input.GetKeyHeldRaw(InputCode::Key::LeftSuper) || input.GetKeyHeldRaw(InputCode::Key::RightSuper);
            const bool shift = input.GetKeyHeldRaw(InputCode::Key::LeftShift) || input.GetKeyHeldRaw(InputCode::Key::RightShift);
            const bool typing = ui->FocusedTextInput != 0 || ui->KeySink != 0 || CommandPalette::IsOpen();
            if((ctrl && shift && input.GetKeyPressedRaw(InputCode::Key::F))
               || (!typing && !ctrl && shift && input.GetKeyPressedRaw(InputCode::Key::Space)))
                SetFocusMode(!m_Focus);
            // Escape is the way out of a full-window view, when nothing else wants it
            else if(m_Focus && !typing && !m_HubOpen && !m_SwitcherOpen && !UIFileDialogIsOpen() && UIDockGetOpenDrawer(m_Dock) == nullptr
                    && !UIIsPopupOpen("##editormenu") && !UIIsPopupOpen("##focuspanels") && input.GetKeyPressedRaw(InputCode::Key::Escape))
                SetFocusMode(false);
        }

        const bool portrait  = m_Rect.w > m_Rect.z;
        const bool overlay   = m_Touch || m_Focus;
        const f32 stripBody  = floorf(44.0f * dpi);
        const f32 stripFull  = stripBody + (overlay ? m_Safe.y : 0.0f);

        // touch: pulled down from the top edge, put away by a tap elsewhere. Focus mode: the pointer at the top edge.
        if(!overlay)
        {
            m_StripShown = true;
            m_StripT     = 1.0f;
        }
        else
        {
            const Vec2 mouse   = UIMousePosition();
            const bool pressed = input.GetMouseClicked(InputCode::MouseKey::ButtonLeft);
            const bool held    = input.GetMouseHeld(InputCode::MouseKey::ButtonLeft);
            const bool busy    = CommandPalette::IsOpen() || UIIsPopupOpen("##editormenu") || UIIsPopupOpen("##crumbscenes") || m_HubOpen;
            if(!m_StripShown)
            {
                if(m_Touch)
                {
                    // starts well below the screen edge too: right at the edge the OS takes the swipe for its own pull-down
                    if(pressed)
                    {
                        m_StripPull  = (mouse.y - m_Rect.y <= m_Safe.y + 56.0f * dpi) ? mouse.y : -1.0f;
                        m_StripPullX = mouse.x;
                    }
                    const f32 down = mouse.y - m_StripPull;
                    if(held && m_StripPull >= 0.0f && down > 18.0f * dpi && Maths::Abs(mouse.x - m_StripPullX) < down)
                        m_StripShown = true;
                    if(!held)
                        m_StripPull = -1.0f;
                }
                // (a band, not the very edge: a fullscreen Mac window shares that edge with the system menu bar)
                // (and not a pointer sitting at exactly 0,0: that is where it reads before it has ever been over the window)
                else if(!held && (mouse.x > 0.5f || mouse.y > 0.5f) && mouse.y >= m_Rect.y && mouse.y <= m_Rect.y + 10.0f * dpi && mouse.x >= m_Rect.x && mouse.x <= m_Rect.x + m_Rect.z)
                    m_StripShown = true;
            }
            else if(!busy)
            {
                m_StripPull       = -1.0f;
                const bool inside = mouse.y <= m_Rect.y + stripFull + (m_Touch ? 0.0f : 40.0f * dpi);
                if(m_Touch ? (pressed && !inside) : !inside)
                    m_StripShown = false;
            }
            const f32 dt = Maths::Clamp((f32)Engine::GetTimeStep().GetRawMillis() * 0.001f, 0.0f, 0.1f);
            m_StripT += ((m_StripShown ? 1.0f : 0.0f) - m_StripT) * (1.0f - expf(-dt * 18.0f));
            if(Maths::Abs((m_StripShown ? 1.0f : 0.0f) - m_StripT) < 0.004f)
                m_StripT = m_StripShown ? 1.0f : 0.0f;
        }

        UpdateSolo();

        // the panel buttons: down the left, or (a phone held upright) the drawer bar along the bottom
        const bool rail     = !m_Focus && !m_Touch;
        const f32 railWidth = rail ? floorf(40.0f * dpi) + m_Safe.x : 0.0f;
        const f32 top       = overlay ? 0.0f : stripBody;
        const Vec4 dockRect = Vec4(m_Rect.x + railWidth, m_Rect.y + top, m_Rect.z - railWidth, m_Rect.w - top);

        UIDockSetDrawerBarHidden(m_Dock, false);
        UIDockSetBarInSolo(m_Dock, m_Touch && !m_Focus);
        UIDockSetSafeInsets(m_Dock, Vec4(rail ? 0.0f : m_Safe.x, overlay ? m_Safe.y : 0.0f, m_Safe.z, m_Safe.w), m_Touch);
        // a view that is not on screen is not rendered: each says so again when it is drawn
        // (the Game panel's was cleared above; this is the scene view's renderer)
        Application::Get().SetDisableMainSceneRenderer(true);
        m_FrameExplorerDrawn   = false;
        m_PhysicsExplorerDrawn = false;
        UIDockSpaceDraw(m_Dock, dockRect);
        // an explorer that is no longer on screen gives back what it was holding (a cut-off frame, a stepped simulation)
        if(!m_FrameExplorerDrawn)
            DebugMenu::Get().CloseFrameExplorer();
        if(!m_PhysicsExplorerDrawn)
            DebugMenu::Get().ClosePhysicsExplorer();
        if(rail)
            DrawRail(Vec4(m_Rect.x, m_Rect.y + top, railWidth, m_Rect.w - top));

        // after the dock: as an overlay it slides down over it
        if(!overlay)
            DrawMenuStrip(m_Rect.y, stripBody);
        else if(m_StripT > 0.0f)
            DrawMenuStrip(m_Rect.y - floorf((1.0f - m_StripT) * (stripFull + 2.0f)), stripBody);
        if(m_Touch && m_StripT < 1.0f)
        {
            // something to pull
            Vec4 grip = UIGetToolTheme().TextDim;
            grip.w    = 0.45f * (1.0f - m_StripT);
            const f32 gripY = m_Rect.y + Maths::Max(4.0f * dpi, m_Safe.y - 7.0f * dpi);
            FillRect("##editorstripgrip", Vec4(m_Rect.x + floorf((m_Rect.z - 40.0f * dpi) * 0.5f), gripY, 40.0f * dpi, 4.0f * dpi), grip, 2.0f * dpi);

            // ...or tap: a bigger target than it looks
            UI_Interaction tap = UIBeginChild("##editorstriptap", { SizeKind_Pixels, floorf(96.0f * dpi) }, { SizeKind_Pixels, floorf(26.0f * dpi) },
                                              WidgetFlags_Clickable | WidgetFlags_Floating_X | WidgetFlags_Floating_Y);
            UI_Widget* target         = ui->parents.Back();
            target->relative_position = Vec2(m_Rect.x + floorf((m_Rect.z - 96.0f * dpi) * 0.5f), gripY - 11.0f * dpi) - ui->root_parent.position;
            UIClearBorders(target);
            UISetBackground(target, Vec4(0.0f), Vec4(0.0f), Vec4(0.0f));
            UIEndChild();
            if(tap.clicked && !m_StripShown)
                m_StripShown = true;
        }

        // focus mode keeps one small control in the corner (bottom right: the views put their status bottom left):
        // the way back, and which panel
        if(m_Focus)
        {
            const UIToolTheme& theme = UIGetToolTheme();
            UIPushStyle(StyleVar_FontSize, theme.FontSize * dpi);
            UIBeginChild("##focusexit", { SizeKind_ChildSum, 1.0f }, { SizeKind_Pixels, floorf(34.0f * dpi) },
                         WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY | WidgetFlags_DrawBackground | WidgetFlags_Clickable | WidgetFlags_Floating_X | WidgetFlags_Floating_Y);
            UI_Widget* pill         = ui->parents.Back();
            pill->relative_position = Vec2(m_Rect.x + m_Rect.z - m_Safe.z - pill->size.x - 10.0f * dpi, m_Rect.y + m_Rect.w - m_Safe.w - pill->size.y - 10.0f * dpi) - ui->root_parent.position;
            Vec4 glass              = theme.Popup;
            glass.w                 = 0.9f;
            UIClearBorders(pill);
            UISetBackground(pill, glass, glass, glass);
            pill->style_vars[StyleVar_Padding]      = Vec4(4.0f * dpi, 0.0f, 0.0f, 0.0f);
            pill->style_vars[StyleVar_ItemSpacing]  = Vec4(2.0f * dpi, 0.0f, 0.0f, 0.0f);
            pill->style_vars[StyleVar_CornerRadius] = Vec4((theme.WidgetRounding + 4.0f) * dpi, 0.0f, 0.0f, 0.0f);

            if(UIIconButton(ICON_MDI_FULLSCREEN_EXIT "###focusleave", 26.0f).clicked)
                SetFocusMode(false);
            UITooltip("Leave focus mode  (Esc)");
            if(UIIconButton(ICON_MDI_VIEW_GRID "###focusswitch", 26.0f).clicked)
                UIOpenPopup("##focuspanels");
            UITooltip("Switch panel  (Ctrl Tab)");
            UIEndChild();

            if(UIBeginPopup("##focuspanels"))
            {
                const char* solo = UIDockGetSolo(m_Dock);
                for(u32 i = 0; i < UIDockGetPanelCount(m_Dock); i++)
                {
                    const char* name = UIDockGetPanelName(m_Dock, i);
                    if(!UIDockIsPanelOpen(m_Dock, name))
                        continue;
                    bool current = solo && strcmp(solo, name) == 0;
                    char item[96];
                    snprintf(item, sizeof(item), "%s###focuspanel%u", UIDockGetPanelTitle(m_Dock, i), i);
                    if(UIMenuItem(item, nullptr, &current))
                        UIDockFocusPanel(m_Dock, name);
                }
                UIMenuSeparator();
                if(UIMenuItem(ICON_MDI_FULLSCREEN_EXIT "  Leave Focus Mode", "Esc"))
                    SetFocusMode(false);
                UIEndPopup();
            }
            UIPopStyle(StyleVar_FontSize);
        }

        DrawSwitcher();

        // the scene file was written by something else: offer to take that version
        if(m_SceneChangedOnDisk)
        {
            const UIToolTheme& theme = UIGetToolTheme();
            UIPushStyle(StyleVar_FontSize, theme.FontSize * dpi);
            UIBeginChild("##scenechanged", { SizeKind_ChildSum, 1.0f }, { SizeKind_Pixels, floorf(38.0f * dpi) },
                         WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY | WidgetFlags_DrawBackground | WidgetFlags_Clickable | WidgetFlags_Floating_X | WidgetFlags_Floating_Y);
            UI_Widget* banner         = ui->parents.Back();
            banner->relative_position = Vec2(m_Rect.x + floorf((m_Rect.z - banner->size.x) * 0.5f), m_Rect.y + top + m_Safe.y + 52.0f * dpi) - ui->root_parent.position;
            UIClearBorders(banner);
            UISetBackground(banner, theme.Popup, theme.Popup, theme.Popup);
            banner->style_vars[StyleVar_Padding]      = Vec4(12.0f * dpi, 0.0f, 0.0f, 0.0f);
            banner->style_vars[StyleVar_ItemSpacing]  = Vec4(8.0f * dpi, 0.0f, 0.0f, 0.0f);
            banner->style_vars[StyleVar_CornerRadius] = Vec4(12.0f * dpi, 0.0f, 0.0f, 0.0f);
            UIPushStyle(StyleVar_TextColor, theme.Warning);
            UILabelCStr("##scenechangedtext", ICON_MDI_ALERT "  The scene changed on disk");
            UIPopStyle(StyleVar_TextColor);
            const bool reload = UIButton("Reload###scenechangedreload").clicked;
            const bool keep   = UIButton("Keep Mine###scenechangedkeep").clicked;
            UIEndChild();
            UIPopStyle(StyleVar_FontSize);
            if(reload)
            {
                SceneManager* manager = Application::Get().GetSceneManager();
                manager->SwitchScene((int)manager->GetCurrentSceneIndex());
            }
            if(reload || keep)
                m_SceneChangedOnDisk = false;
        }

        if(m_TourAt >= 0)
            DrawTour();

        // last, so it sits over everything
        UIPushStyle(StyleVar_FontSize, UIGetToolTheme().FontSize * dpi);
        if(m_HubOpen)
            DrawProjectHub(false);
        CommandPalette::Draw(!UIFileDialogIsOpen() && !m_HubOpen && !m_QuitAsk);
        UIDrawFileDialog();
        if(m_AboutOpen)
            DrawAbout();
        if(m_QuitAsk)
            DrawQuitPrompt();
        UIPopStyle(StyleVar_FontSize);
        m_BuildMs += (buildTimer.GetElapsedMS() - m_BuildMs) * 0.1f;
    }

    void EditorUI::StartTour(const TourStep* steps, u32 count)
    {
        m_TourCount = Maths::Min(count, 16u);
        for(u32 i = 0; i < m_TourCount; i++)
        {
            snprintf(m_Tour[i].Panel, sizeof(m_Tour[i].Panel), "%s", steps[i].Panel ? steps[i].Panel : "");
            snprintf(m_Tour[i].Title, sizeof(m_Tour[i].Title), "%s", steps[i].Title ? steps[i].Title : "");
            snprintf(m_Tour[i].Text, sizeof(m_Tour[i].Text), "%s", steps[i].Text ? steps[i].Text : "");
        }
        m_TourAt = m_TourCount > 0 ? 0 : -1;
    }

    void EditorUI::StartDefaultTour()
    {
        static const TourStep kSteps[] = {
            { "", "A quick look round", "Eight short stops. Everything stays usable while this is up: try things as you go. Escape leaves at any point." },
            { "Scene", "Scene", "The world as the editor sees it. Hold the right mouse button and use W A S D to fly; scroll while holding it to change speed. Click something to select it. The bar at the top has the move, rotate and scale tools, the camera and the view options." },
            { "Hierarchy", "Hierarchy", "Every entity in the scene, as a tree. Drag one onto another to parent it. Right-click for create, rename, copy and delete; F2 renames in place." },
            { "Inspector", "Inspector", "What the selected entity is made of. Each block is a component; drag a number to scrub it, double-click to type. Add Component at the bottom is searchable. Every edit can be undone." },
            { "Assets", "Assets", "The project's files. Double-click to open, drag a model or picture into the scene, drag a file onto a folder to move it. The + button makes folders and scripts." },
            { "Command Bar", "The command bar", "The fastest way to anything: Cmd / Ctrl + P, then type. Start with @ for entities, # for files, : to set a value on the selection, = to run a line of Lua on it." },
            { "Game", "Game", "What the player sees. Play starts the game in here; the speed button slows it down or stops time, and the bug button draws physics shapes and other debug views over it." },
            { "Frame Explorer", "Frame Explorer", "How a frame is built. Pick a render pass to stop the frame right after it and read what that pass does, what it reads and writes, and its settings - on your own scene. Physics Explorer does the same for one physics step." },
            { "", "Make it yours", "Lua files in the project's Editor folder add commands and panels to the editor itself (see Editor/Example.lua). Layout > Minimal and Focus Mode cut the interface down when you want the room." },
        };
        StartTour(kSteps, (u32)(sizeof(kSteps) / sizeof(kSteps[0])));
    }

    void EditorUI::DrawTour()
    {
        if(m_TourAt < 0 || m_TourAt >= (i32)m_TourCount)
        {
            m_TourAt = -1;
            return;
        }
        UI_State* ui             = GetUIState();
        const UIToolTheme& theme = UIGetToolTheme();
        const f32 dpi            = ui->DPIScale;
        const StoredTourStep& step = m_Tour[m_TourAt];

        // what to light up: a panel (brought forward if it is behind another), the strip, or nothing
        Vec4 target   = Vec4(0.0f);
        bool lit      = false;
        if(strcmp(step.Panel, "Command Bar") == 0)
        {
            m_StripShown = true;
            target       = Vec4(m_Rect.x, m_Rect.y, m_Rect.z, floorf(44.0f * dpi) + ((m_Touch || m_Focus) ? m_Safe.y : 0.0f));
            lit          = true;
        }
        else if(step.Panel[0])
        {
            if(!UIDockIsPanelVisible(m_Dock, step.Panel))
                UIDockFocusPanel(m_Dock, step.Panel);
            lit = UIDockGetPanelRect(m_Dock, step.Panel, &target);
        }

        // dim everything else: four quads round the lit one, so what is lit can still be used
        Vec4 dim = Vec4(0.0f, 0.0f, 0.0f, theme.Dark ? 0.55f : 0.35f);
        if(lit)
        {
            const f32 right  = target.x + target.z;
            const f32 bottom = target.y + target.w;
            FillRect("##tourdimtop", Vec4(m_Rect.x, m_Rect.y, m_Rect.z, Maths::Max(0.0f, target.y - m_Rect.y)), dim);
            FillRect("##tourdimbottom", Vec4(m_Rect.x, bottom, m_Rect.z, Maths::Max(0.0f, m_Rect.y + m_Rect.w - bottom)), dim);
            FillRect("##tourdimleft", Vec4(m_Rect.x, target.y, Maths::Max(0.0f, target.x - m_Rect.x), target.w), dim);
            FillRect("##tourdimright", Vec4(right, target.y, Maths::Max(0.0f, m_Rect.x + m_Rect.z - right), target.w), dim);
            // an outline, drawn as four thin bars
            const f32 line = Maths::Max(2.0f, floorf(2.0f * dpi));
            FillRect("##tourlinetop", Vec4(target.x, target.y, target.z, line), theme.Accent);
            FillRect("##tourlinebottom", Vec4(target.x, bottom - line, target.z, line), theme.Accent);
            FillRect("##tourlineleft", Vec4(target.x, target.y, line, target.w), theme.Accent);
            FillRect("##tourlineright", Vec4(right - line, target.y, line, target.w), theme.Accent);
        }
        else
            FillRect("##tourdimall", m_Rect, dim);

        // the card: beside what is lit when there is room, else over the middle
        const f32 width = floorf(Maths::Min(340.0f * dpi, m_Rect.z * 0.9f));
        UIPushStyle(StyleVar_FontSize, theme.FontSize * dpi);
        UIPushStyle(StyleVar_BackgroundColor, theme.Popup);
        UIPushStyle(StyleVar_Padding, Vec2(16.0f * dpi, 14.0f * dpi));
        UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 7.0f * dpi));
        UIPushStyle(StyleVar_CornerRadius, 12.0f * dpi);
        UIPushStyle(StyleVar_TextColor, theme.Text);
        UIBeginOverlay("##tourcard", SizeKind_Pixels, width, SizeKind_ChildSum, 1.0f, WidgetFlags_StackVertically | WidgetFlags_Floating_X | WidgetFlags_Floating_Y);
        {
            UI_Widget* card  = ui->parents.Back();
            const f32 height = card->size.y;
            const f32 gap    = 12.0f * dpi;
            Vec2 at          = Vec2(m_Rect.x + (m_Rect.z - width) * 0.5f, m_Rect.y + (m_Rect.w - height) * 0.5f);
            if(lit)
            {
                const f32 roomRight = m_Rect.x + m_Rect.z - (target.x + target.z);
                const f32 roomLeft  = target.x - m_Rect.x;
                const f32 roomBelow = m_Rect.y + m_Rect.w - (target.y + target.w);
                if(roomRight > width + gap * 2.0f)
                    at = Vec2(target.x + target.z + gap, target.y + gap);
                else if(roomLeft > width + gap * 2.0f)
                    at = Vec2(target.x - width - gap, target.y + gap);
                else if(roomBelow > height + gap * 2.0f)
                    at = Vec2(target.x + (target.z - width) * 0.5f, target.y + target.w + gap);
                else
                    at = Vec2(target.x + (target.z - width) * 0.5f, target.y + target.w - height - gap * 2.0f); // inside, along its bottom
            }
            at.x = Maths::Clamp(at.x, m_Rect.x + m_Safe.x + gap, Maths::Max(m_Rect.x + m_Safe.x + gap, m_Rect.x + m_Rect.z - m_Safe.z - width - gap));
            at.y = Maths::Clamp(at.y, m_Rect.y + m_Safe.y + gap, Maths::Max(m_Rect.y + m_Safe.y + gap, m_Rect.y + m_Rect.w - m_Safe.w - height - gap));
            card->relative_position = Vec2(floorf(at.x), floorf(at.y)) - ui->root_parent.position;
        }
        UIPopStyle(StyleVar_TextColor);
        UIPopStyle(StyleVar_CornerRadius);
        UIPopStyle(StyleVar_ItemSpacing);
        UIPopStyle(StyleVar_Padding);
        UIPopStyle(StyleVar_BackgroundColor);

        UIPushStyle(StyleVar_FontSize, theme.FontSize * 1.2f * dpi);
        UILabelCStr("##tourtitle", step.Title);
        UIPopStyle(StyleVar_FontSize);
        UIPushStyle(StyleVar_TextColor, theme.TextDim);
        UILabelWrapped("##tourtext", Str8C((char*)step.Text), width / dpi - 36.0f);
        UIPopStyle(StyleVar_TextColor);
        UISpacer(2.0f);

        UIBeginRowFullWidth();
        ui->parents.Back()->style_vars[StyleVar_ItemSpacing] = Vec4(6.0f * dpi, 0.0f, 0.0f, 0.0f);
        ui->parents.Back()->flags |= WidgetFlags_CentreChildrenY;
        char count[24];
        snprintf(count, sizeof(count), "%d / %u", m_TourAt + 1, m_TourCount);
        UIPushStyle(StyleVar_TextColor, theme.TextDim);
        UILabelCStr("##tourcount", count);
        UIPopStyle(StyleVar_TextColor);
        UIFlexSpacer();
        const bool last  = m_TourAt + 1 >= (i32)m_TourCount;
        const bool leave = UIButton("Close###tourclose").clicked || Input::Get().GetKeyPressedRaw(InputCode::Key::Escape);
        const bool back  = m_TourAt > 0 && UIButton("Back###tourback").clicked;
        const bool next  = UIButton(last ? "Done###tournext" : "Next###tournext").clicked;
        UIEndRow();
        UIEndPanel();
        UIPopStyle(StyleVar_FontSize);

        if(leave || (next && last))
            m_TourAt = -1;
        else if(next)
            m_TourAt++;
        else if(back)
            m_TourAt--;
    }

    // The scene file on disk is newer than what was loaded or saved here: something else wrote it
    // (the same project open on another device through iCloud Drive, a script, a pull).
    void EditorUI::CheckSceneFile()
    {
        Scene* scene = CurrentScene();
        if(!scene || Application::Get().GetEditorState() != EditorState::Preview)
            return;
        if(++m_SceneFileCheck % 45 != 0)
            return;
        const std::string path = Application::Get().GetProjectSettings().m_ProjectRoot + "Assets/Scenes/" + scene->GetSceneName() + ".lsn";
        const u64 time         = FileSystem::GetFileModifiedTime(Str8StdS(path));
        if(m_SceneFileTime == 0 || time == 0)
            m_SceneFileTime = time; // first look at this scene
        else if(time != m_SceneFileTime)
        {
            m_SceneFileTime      = time;
            m_SceneChangedOnDisk = true;
        }
    }

    Vec2 EditorUI::ViewTarget::Settle(const Vec2& wanted)
    {
        if(Current.x < 2.0f || Current.y < 2.0f)
            Current = wanted; // first time: nothing to keep
        else if(wanted.x != Current.x || wanted.y != Current.y)
        {
            if(wanted.x == Pending.x && wanted.y == Pending.y)
                Frames++;
            else
            {
                Pending = wanted;
                Frames  = 0;
            }
            // still for a few frames: the animation or the drag is over
            if(Frames >= 6)
                Current = wanted;
        }
        else
            Frames = 0;
        return Current;
    }

    bool EditorUI::SceneUnsaved() const
    {
        // every edit the tools make goes through undo: a different count than at the last save means something changed
        // (not while playing: that scene is a copy that is thrown away)
        return Application::Get().GetEditorState() == EditorState::Preview && m_SavedScene == (void*)CurrentScene() && UndoStepCount() != m_SavedUndoCount;
    }

    bool EditorUI::HasUnsavedWork() const
    {
        return AnyScriptUnsaved() || SceneUnsaved();
    }

    void EditorUI::NoteSceneSaved()
    {
        m_SavedScene     = CurrentScene();
        m_SavedUndoCount = UndoStepCount();
        // our own write: not a change from elsewhere
        m_SceneFileTime      = 0;
        m_SceneChangedOnDisk = false;
    }

    void EditorUI::RequestQuit()
    {
        if(HasUnsavedWork())
            m_QuitAsk = true;
        else
            Application::Get().SetAppState(AppState::Closing);
    }

    void EditorUI::DrawQuitPrompt()
    {
        UI_State* ui             = GetUIState();
        const UIToolTheme& theme = UIGetToolTheme();
        const f32 dpi            = ui->DPIScale;
        const bool scripts       = AnyScriptUnsaved();
        const bool scene         = SceneUnsaved();

        UIDimBackdrop("##quitdim", theme.Dark ? 0.5f : 0.3f);
        const f32 width = floorf(Maths::Min(420.0f * dpi, m_Rect.z * 0.92f));
        UIPushStyle(StyleVar_BackgroundColor, theme.Popup);
        UIPushStyle(StyleVar_Padding, Vec2(20.0f * dpi, 18.0f * dpi));
        UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 8.0f * dpi));
        UIPushStyle(StyleVar_CornerRadius, 14.0f * dpi);
        UIPushStyle(StyleVar_TextColor, theme.Text);
        UIBeginOverlay("##quitprompt", SizeKind_Pixels, width, SizeKind_ChildSum, 1.0f, WidgetFlags_StackVertically | WidgetFlags_Floating_X | WidgetFlags_Floating_Y);
        {
            UI_Widget* card         = ui->parents.Back();
            card->relative_position = Vec2(m_Rect.x + floorf((m_Rect.z - width) * 0.5f), m_Rect.y + floorf((m_Rect.w - card->size.y) * 0.4f)) - ui->root_parent.position;
        }
        UIPopStyle(StyleVar_TextColor);
        UIPopStyle(StyleVar_CornerRadius);
        UIPopStyle(StyleVar_ItemSpacing);
        UIPopStyle(StyleVar_Padding);
        UIPopStyle(StyleVar_BackgroundColor);

        UIPushStyle(StyleVar_FontSize, theme.FontSize * 1.2f * dpi);
        UILabelCStr("##quittitle", "Quit without saving?");
        UIPopStyle(StyleVar_FontSize);

        u32 unsaved = 0;
        for(i32 i = 0; i < m_ScriptCount; i++)
            unsaved += UICodeEditorIsDirty(m_Scripts[i].Code) ? 1u : 0u;
        char what[160];
        if(scripts && scene)
            snprintf(what, sizeof(what), "The scene and %u script%s have changes that were not saved.", unsaved, unsaved == 1 ? "" : "s");
        else if(scripts)
            snprintf(what, sizeof(what), "%u script%s changes that were not saved.", unsaved, unsaved == 1 ? " has" : "s have");
        else
            snprintf(what, sizeof(what), "The scene has changes that were not saved.");
        UIPushStyle(StyleVar_TextColor, theme.TextDim);
        UILabelWrapped("##quitwhat", Str8C(what), width / dpi - 44.0f);
        UIPopStyle(StyleVar_TextColor);
        UISpacer(4.0f);

        UIBeginRowFullWidth();
        ui->parents.Back()->style_vars[StyleVar_ItemSpacing] = Vec4(6.0f * dpi, 0.0f, 0.0f, 0.0f);
        const bool save    = UIButton("Save and Quit###quitsave").clicked;
        const bool discard = UIButton("Quit###quitdiscard").clicked;
        UIFlexSpacer();
        const bool cancel = UIButton("Cancel###quitcancel").clicked || Input::Get().GetKeyPressedRaw(InputCode::Key::Escape);
        UIEndRow();
        UIEndPanel();

        if(save)
        {
            SaveAllScripts();
            if(scene)
                m_Editor->SaveCurrentScene();
        }
        if(save || discard)
            Application::Get().SetAppState(AppState::Closing);
        if(save || discard || cancel)
            m_QuitAsk = false;
    }

    void EditorUI::DrawAbout()
    {
        UI_State* ui             = GetUIState();
        const UIToolTheme& theme = UIGetToolTheme();
        const f32 dpi            = ui->DPIScale;

        const bool away = UIDimBackdrop("##aboutdim", theme.Dark ? 0.5f : 0.3f).clicked;
        const f32 width = floorf(Maths::Min(380.0f * dpi, m_Rect.z * 0.92f));
        UIPushStyle(StyleVar_BackgroundColor, theme.Popup);
        UIPushStyle(StyleVar_Padding, Vec2(22.0f * dpi, 20.0f * dpi));
        UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 6.0f * dpi));
        UIPushStyle(StyleVar_CornerRadius, Maths::Min(14.0f, theme.PanelRounding + 4.0f) * dpi);
        UIPushStyle(StyleVar_TextColor, theme.Text);
        UIBeginOverlay("##aboutcard", SizeKind_Pixels, width, SizeKind_ChildSum, 1.0f,
                       WidgetFlags_StackVertically | WidgetFlags_Floating_X | WidgetFlags_Floating_Y | WidgetFlags_DrawBorder);
        {
            UI_Widget* card                           = ui->parents.Back();
            card->relative_position                   = Vec2(m_Rect.x + floorf((m_Rect.z - width) * 0.5f), m_Rect.y + floorf((m_Rect.w - card->size.y) * 0.4f)) - ui->root_parent.position;
            card->style_vars[StyleVar_Border]         = Vec4(Maths::Max(1.0f, dpi), Maths::Max(1.0f, dpi), 0.0f, 0.0f);
            card->style_vars[StyleVar_BorderColor]    = theme.Border;
            card->style_vars[StyleVar_HotBorderColor] = theme.Border;
        }
        UIPopStyle(StyleVar_TextColor);
        UIPopStyle(StyleVar_CornerRadius);
        UIPopStyle(StyleVar_ItemSpacing);
        UIPopStyle(StyleVar_Padding);
        UIPopStyle(StyleVar_BackgroundColor);

        // logo, name, version
        UIBeginRowFullWidth();
        {
            UI_Widget* row                        = ui->parents.Back();
            row->style_vars[StyleVar_ItemSpacing] = Vec4(12.0f * dpi, 0.0f, 0.0f, 0.0f);
            row->flags |= WidgetFlags_CentreChildrenY;
        }
        if(Graphics::Texture2D* logo = m_Editor->GetLogoTexture())
            UIImageSized("##aboutlogo", logo, Vec2(44.0f * dpi, 44.0f * dpi));
        UIPushStyle(StyleVar_FontSize, theme.FontSize * 1.5f * dpi);
        UILabelCStr("##abouttitle", "Lumos Engine");
        UIPopStyle(StyleVar_FontSize);
        UIFlexSpacer();
        const bool close = UIIconButton(ICON_MDI_CLOSE "###aboutclose", 26.0f).clicked;
        UIEndRow();

        const auto& version = LumosVersion;
        char line[160];
        snprintf(line, sizeof(line), "Version %d.%d.%d", version.major, version.minor, version.patch);
        UIPushStyle(StyleVar_TextColor, theme.TextDim);
        UILabelCStr("##aboutversion", line);
        UILabelWrapped("##aboutblurb", Str8Lit("A cross-platform 2D / 3D game engine and editor."), width / dpi - 48.0f);
        UIPopStyle(StyleVar_TextColor);

        // a row that opens a page
        auto link = [&](const char* label, const char* note, const char* url)
        {
            char id[96];
            snprintf(id, sizeof(id), "###aboutlink%s", label);
            const bool pressed = UIBeginSelectableRow(id, false, 26.0f).clicked;
            UILabelCStr("###name", label);
            UIFlexSpacer();
            if(note[0])
            {
                UIPushStyle(StyleVar_TextColor, theme.TextDim);
                UILabelCStr("###note", note);
                UIPopStyle(StyleVar_TextColor);
            }
            UIEndSelectableRow();
            if(pressed)
                OS::Get().OpenURL(url);
        };
        auto heading = [&](const char* id, const char* text)
        {
            UISpacer(6.0f);
            UIPushStyle(StyleVar_FontSize, theme.FontSize * 0.86f * dpi);
            UIPushStyle(StyleVar_TextColor, theme.TextDim);
            UILabelCStr(id, text);
            UIPopStyle(StyleVar_TextColor);
            UIPopStyle(StyleVar_FontSize);
        };

        UISpacer(4.0f);
        link(ICON_MDI_GITHUB_CIRCLE "  GitHub Repository", ICON_MDI_OPEN_IN_NEW, "https://www.github.com/jmorton06/Lumos");
        // a store build tips through the App Store (no outside payment links there)
        if(Store::IsSupported())
        {
            if(UIBeginSelectableRow("##abouttips", false).clicked)
            {
                m_AboutOpen = false;
                UIDockFocusPanel(m_Dock, "Support Lumos");
            }
            UILabelCStr("###name", ICON_MDI_HEART "  Support Lumos");
            UIEndSelectableRow();
        }
        else
            link(ICON_MDI_HEART "  Support Development", ICON_MDI_OPEN_IN_NEW, "https://github.com/sponsors/jmorton06");

        heading("##aboutpeople", "CONTRIBUTORS");
        link("Joe Morton", "", "https://github.com/jmorton06");
        link("RuanLucasGD", "", "https://github.com/RuanLucasGD");
        link("adriengivry", "", "https://github.com/adriengivry");

        // everything built into this binary
        heading("##aboutthird", "THIRD-PARTY");
        u32 libraryCount                   = 0;
        const ThirdPartyLicence* libraries = GetThirdPartyLicences(libraryCount);
        static f32 s_LibraryScroll         = 0.0f;
        UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 2.0f * dpi));
        UIBeginScrollArea("##aboutlibraries", floorf(Maths::Clamp(m_Rect.w * 0.3f, 120.0f * dpi, 260.0f * dpi)), &s_LibraryScroll);
        for(u32 i = 0; i < libraryCount; i++)
        {
            // the ones with a version to show
            const char* note = libraries[i].Licence;
            if(strcmp(libraries[i].Name, "EnTT") == 0)
                note = ENTT_VERSION;
            else if(strcmp(libraries[i].Name, "cereal") == 0)
            {
                snprintf(line, sizeof(line), "%i.%i.%i", CEREAL_VERSION_MAJOR, CEREAL_VERSION_MINOR, CEREAL_VERSION_PATCH);
                note = line;
            }
            link(libraries[i].Name, note, libraries[i].URL);
        }
        // shipped beside the editor, not in the engine's table
        link("Cousine (font)", "Apache-2.0", "https://fonts.google.com/specimen/Cousine");
        UIEndScrollArea();
        UIPopStyle(StyleVar_ItemSpacing);
        UIEndPanel();

        if(close || away || Input::Get().GetKeyPressedRaw(InputCode::Key::Escape))
            m_AboutOpen = false;
    }

    void EditorUI::UpdateSolo()
    {
        // a phone has no room to spare for tabs: the rail (on its side) or the bar (upright) picks the panel
        const bool solo = m_Focus || m_Minimal || m_Touch;
        if(!solo)
        {
            if(UIDockGetSolo(m_Dock))
                UIDockSetSolo(m_Dock, nullptr);
            return;
        }
        if(UIDockGetSolo(m_Dock))
            return;
        // whatever was being worked in, if it is a view; else the scene (or the game while it runs)
        const char* panel  = UIDockGetFocusedPanel(m_Dock);
        const bool playing = Application::Get().GetEditorState() != EditorState::Preview;
        if(!panel || UIDockIsPanelDrawer(m_Dock, panel) || !UIDockIsPanelVisible(m_Dock, panel))
            panel = playing ? "Game" : "Scene";
        UIDockSetSolo(m_Dock, panel);
    }

    void EditorUI::SetFocusMode(bool focus)
    {
        if(focus == m_Focus || !m_Dock)
            return;
        m_Focus = focus;
        UpdateSolo();
        if(focus)
            m_StripShown = false;
    }

    bool EditorUI::IsMaximised(const char* panel) const
    {
        const char* solo = m_Dock ? UIDockGetSolo(m_Dock) : nullptr;
        return m_Focus && solo && strcmp(solo, panel) == 0;
    }

    void EditorUI::ToggleMaximised(const char* panel)
    {
        if(!m_Dock)
            return;
        if(IsMaximised(panel))
        {
            SetFocusMode(false);
            return;
        }
        m_Focus      = true;
        m_StripShown = false;
        UIDockSetSolo(m_Dock, panel);
    }

    void EditorUI::SetMinimal(bool minimal)
    {
        if(minimal == m_Minimal || !m_Dock)
            return;
        m_Minimal = minimal;
        UISetToolThemePlain(minimal);
        UpdateSolo();
    }

    Vec4 EditorUI::ViewInsets(const Vec2& position, const Vec2& size) const
    {
        return Vec4(Maths::Max(0.0f, m_Rect.x + m_Safe.x - position.x),
                    Maths::Max(0.0f, m_Rect.y + m_Safe.y - position.y),
                    Maths::Max(0.0f, position.x + size.x - (m_Rect.x + m_Rect.z - m_Safe.z)),
                    Maths::Max(0.0f, position.y + size.y - (m_Rect.y + m_Rect.w - m_Safe.w)));
    }

    // One button per open panel, down the left edge.
    // ---- rail tools: what the panel being worked in can do ----
    // Each panel gets the handful of actions you reach for while in it; panels whose
    // controls all live inside them (settings, explorers, docs) get none.
    namespace
    {
        struct RailTool
        {
            const char* Icon;
            const char* Tip;
            void (*Run)();
            bool (*On)(); // null = a plain action
        };

        bool GizmoIs(u32 op) { return s_Editor->GetGizmoOperation() == op; }
        bool IsPlaying() { return Application::Get().GetEditorState() != EditorState::Preview; }

        const RailTool kSceneTools[] = {
            { ICON_MDI_CURSOR_DEFAULT, "Select  (Q)", [] { s_Editor->SetGizmoOperation(GizmoOperation_None); }, [] { return GizmoIs(GizmoOperation_None); } },
            { ICON_MDI_ARROW_ALL, "Move  (W)", [] { s_Editor->SetGizmoOperation(GizmoOperation_Translate); }, [] { return GizmoIs(GizmoOperation_Translate); } },
            { ICON_MDI_ROTATE_RIGHT, "Rotate  (E)", [] { s_Editor->SetGizmoOperation(GizmoOperation_Rotate); }, [] { return GizmoIs(GizmoOperation_Rotate); } },
            { ICON_MDI_VECTOR_SQUARE, "Scale  (R)", [] { s_Editor->SetGizmoOperation(GizmoOperation_Scale); }, [] { return GizmoIs(GizmoOperation_Scale); } },
            { ICON_MDI_AXIS_ARROW, "Universal  (T)", [] { s_Editor->SetGizmoOperation(GizmoOperation_Universal); }, [] { return GizmoIs(GizmoOperation_Universal); } },
            { nullptr, nullptr, nullptr, nullptr },
            { ICON_MDI_CUBE_OUTLINE, "Local axes", [] { s_Self->GizmoLocal() = !s_Self->GizmoLocal(); }, [] { return s_Self->GizmoLocal(); } },
            { ICON_MDI_RULER, "Snap", [] { s_Editor->SnapGuizmo() = !s_Editor->SnapGuizmo(); }, [] { return s_Editor->SnapGuizmo(); } },
            { ICON_MDI_GRID, "Grid", [] { s_Editor->ShowGrid() = !s_Editor->ShowGrid(); }, [] { return s_Editor->ShowGrid(); } },
            { nullptr, nullptr, nullptr, nullptr },
            { ICON_MDI_PLUS, "New entity", [] { CommandPalette::Run("New Entity", "Create"); }, nullptr },
            { ICON_MDI_CROSSHAIRS, "Focus selected  (F)", [] { CommandPalette::Run("Focus Selected", "View"); }, nullptr },
        };
        const RailTool kGameTools[] = {
            { ICON_MDI_PLAY, "Play / Stop", [] { s_Editor->TogglePlay(); }, [] { return IsPlaying(); } },
            { ICON_MDI_PAUSE, "Pause", [] { s_Editor->TogglePause(); }, [] { return Application::Get().GetEditorState() == EditorState::Paused; } },
            { ICON_MDI_FULLSCREEN, "Maximise", [] { s_Self->ToggleMaximised("Game"); }, [] { return s_Self->IsMaximised("Game"); } },
        };
        const RailTool kEntityTools[] = {
            { ICON_MDI_PLUS, "New entity", [] { CommandPalette::Run("New Entity", "Create"); }, nullptr },
            { ICON_MDI_CONTENT_COPY, "Duplicate  (Ctrl D)", [] { CommandPalette::Run("Duplicate Selected", "Edit"); }, nullptr },
            { ICON_MDI_DELETE_OUTLINE, "Delete", [] { CommandPalette::Run("Delete Selected", "Edit"); }, nullptr },
            { ICON_MDI_CROSSHAIRS, "Focus selected  (F)", [] { CommandPalette::Run("Focus Selected", "View"); }, nullptr },
        };
        const RailTool kAssetTools[] = {
            { ICON_MDI_REFRESH, "Rescan assets", [] { ToolPanels::RefreshAssetIndex(); }, nullptr },
            { ICON_MDI_IMAGE, "New sprite", [] { SpriteEditor::New(32, 32); }, nullptr },
        };
        const RailTool kScriptTools[] = {
            { ICON_MDI_CONTENT_SAVE, "Save  (Ctrl S)", [] { s_Self->SaveScript(); }, nullptr },
            { ICON_MDI_FILE, "Save all", [] { s_Self->SaveAllScripts(); }, nullptr },
        };

        struct RailToolSet
        {
            const char* Panel;
            const RailTool* Tools;
            u32 Count;
        };
        const RailToolSet kRailToolSets[] = {
            { "Scene", kSceneTools, (u32)(sizeof(kSceneTools) / sizeof(kSceneTools[0])) },
            { "Game", kGameTools, (u32)(sizeof(kGameTools) / sizeof(kGameTools[0])) },
            { "Hierarchy", kEntityTools, (u32)(sizeof(kEntityTools) / sizeof(kEntityTools[0])) },
            { "Inspector", kEntityTools, (u32)(sizeof(kEntityTools) / sizeof(kEntityTools[0])) },
            { "Assets", kAssetTools, (u32)(sizeof(kAssetTools) / sizeof(kAssetTools[0])) },
            { "Script", kScriptTools, (u32)(sizeof(kScriptTools) / sizeof(kScriptTools[0])) },
        };

        // the panel last worked in: pressing a rail button must not make the rail forget it
        char s_RailContext[48] = "Scene";
    }

    // Tool buttons for the panel being worked in. False = nothing for that panel.
    bool EditorUI::DrawRailTools()
    {
        const UIToolTheme& theme = UIGetToolTheme();
        if(const char* focused = UIDockGetFocusedPanel(m_Dock))
            if(UIDockIsPanelOpen(m_Dock, focused))
                snprintf(s_RailContext, sizeof(s_RailContext), "%s", focused);

        u32 drawn = 0;
        char id[64];
        // the Sprite Editor's own tools, straight from it
        if(strcmp(s_RailContext, "Sprite Editor") == 0)
        {
            for(u32 t = 0; t < SpriteEditor::GetToolCount(); t++)
            {
                const bool on = SpriteEditor::GetTool() == t && !SpriteEditor::GetShapeMode();
                snprintf(id, sizeof(id), "%s###railsprite%u", SpriteEditor::GetToolIcon(t), t);
                UIPushStyle(StyleVar_TextColor, on ? theme.Text : theme.TextDim);
                if(UIIconButton(id, 30.0f, on).clicked)
                {
                    SpriteEditor::SetShapeMode(false);
                    SpriteEditor::SetTool(t);
                }
                UIPopStyle(StyleVar_TextColor);
                UITooltip(SpriteEditor::GetToolName(t));
                drawn++;
            }
            const bool shape = SpriteEditor::GetShapeMode();
            UIPushStyle(StyleVar_TextColor, shape ? theme.Text : theme.TextDim);
            if(UIIconButton(ICON_MDI_VECTOR_POLYGON "###railspriteshape", 30.0f, shape).clicked)
                SpriteEditor::SetShapeMode(!shape);
            UIPopStyle(StyleVar_TextColor);
            UITooltip("Collision shape  (C)");
            drawn++;
        }
        else
        {
            for(const RailToolSet& set : kRailToolSets)
            {
                if(strcmp(set.Panel, s_RailContext) != 0)
                    continue;
                for(u32 i = 0; i < set.Count; i++)
                {
                    const RailTool& tool = set.Tools[i];
                    if(!tool.Icon)
                    {
                        UISpacer(6.0f);
                        continue;
                    }
                    const bool on = tool.On && tool.On();
                    snprintf(id, sizeof(id), "%s###railtool%u", tool.Icon, i);
                    UIPushStyle(StyleVar_TextColor, (on || !tool.On) ? theme.Text : theme.TextDim);
                    if(UIIconButton(id, 30.0f, on).clicked)
                        tool.Run();
                    UIPopStyle(StyleVar_TextColor);
                    UITooltip(tool.Tip);
                    drawn++;
                }
                break;
            }
        }
        return drawn > 0;
    }

    void EditorUI::DrawRail(const Vec4& rect)
    {
        UI_State* ui             = GetUIState();
        const UIToolTheme& theme = UIGetToolTheme();
        const f32 dpi            = ui->DPIScale;

        FillRect("##editorrailfill", rect, theme.Canvas);

        UIPushStyle(StyleVar_FontSize, theme.FontSize * dpi);
        UIBeginChild("##editorrail", { SizeKind_Pixels, rect.z - m_Safe.x }, { SizeKind_Pixels, rect.w - m_Safe.w },
                     WidgetFlags_StackVertically | WidgetFlags_CentreChildrenX | WidgetFlags_Clickable | WidgetFlags_Floating_X | WidgetFlags_Floating_Y, true);
        {
            UI_Widget* bar                        = ui->parents.Back();
            bar->relative_position                = Vec2(rect.x + m_Safe.x, rect.y) - ui->root_parent.position;
            bar->style_vars[StyleVar_Padding]     = Vec4(0.0f, 6.0f * dpi, 0.0f, 0.0f);
            bar->style_vars[StyleVar_ItemSpacing] = Vec4(0.0f, 4.0f * dpi, 0.0f, 0.0f);
            UIClearBorders(bar);
            UISetBackground(bar, Vec4(0.0f), Vec4(0.0f), Vec4(0.0f));
        }

        // only the tools of the panel being worked in: panels are reached from their tabs, the menu and the palette
        DrawRailTools();

        UIFlexSpacer();
        UIPushStyle(StyleVar_TextColor, theme.TextDim);
        if(!m_Touch)
        {
            if(UIIconButton(ICON_MDI_FULLSCREEN "###railfocus", 30.0f).clicked)
                SetFocusMode(true);
            UITooltip("Focus mode  (Esc to leave)");
        }
        UIPopStyle(StyleVar_TextColor);
        UIEndChild();

        UIPopStyle(StyleVar_FontSize);
    }

    // Ctrl + Tab: hold Ctrl, Tab steps through the open panels, letting go picks.
    void EditorUI::DrawSwitcher()
    {
        Input& input     = Input::Get();
        const bool ctrl  = input.GetKeyHeldRaw(InputCode::Key::LeftControl) || input.GetKeyHeldRaw(InputCode::Key::RightControl);
        const bool shift = input.GetKeyHeldRaw(InputCode::Key::LeftShift) || input.GetKeyHeldRaw(InputCode::Key::RightShift);
        const bool tab   = input.GetKeyPressedRaw(InputCode::Key::Tab);

        u32 open[48];
        u32 count = 0;
        for(u32 i = 0; i < UIDockGetPanelCount(m_Dock) && count < 48; i++)
            if(UIDockIsPanelOpen(m_Dock, UIDockGetPanelName(m_Dock, i)))
                open[count++] = i;
        if(count == 0)
        {
            m_SwitcherOpen = false;
            return;
        }

        if(!m_SwitcherOpen)
        {
            if(!(ctrl && tab))
                return;
            m_SwitcherOpen      = true;
            m_SwitcherIndex     = 0;
            const char* focused = UIDockGetFocusedPanel(m_Dock);
            for(u32 i = 0; i < count; i++)
                if(focused && strcmp(focused, UIDockGetPanelName(m_Dock, open[i])) == 0)
                    m_SwitcherIndex = (i32)i;
        }
        if(tab)
            m_SwitcherIndex = (m_SwitcherIndex + (shift ? (i32)count - 1 : 1)) % (i32)count;
        m_SwitcherIndex = Maths::Clamp(m_SwitcherIndex, 0, (i32)count - 1);

        if(input.GetKeyPressedRaw(InputCode::Key::Escape))
        {
            m_SwitcherOpen = false;
            return;
        }
        if(!ctrl)
        {
            m_SwitcherOpen   = false;
            const char* name = UIDockGetPanelName(m_Dock, open[m_SwitcherIndex]);
            UIDockFocusPanel(m_Dock, name);
            return;
        }

        UI_State* ui             = GetUIState();
        const UIToolTheme& theme = UIGetToolTheme();
        const f32 dpi            = ui->DPIScale;
        UIPushStyle(StyleVar_FontSize, theme.FontSize * dpi);
        UIBeginChild("##panelswitcher", { SizeKind_Pixels, floorf(240.0f * dpi) }, { SizeKind_ChildSum, 1.0f },
                     WidgetFlags_StackVertically | WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y | WidgetFlags_TopMost);
        UI_Widget* card                            = ui->parents.Back();
        card->relative_position                    = Vec2(m_Rect.x + floorf((m_Rect.z - card->size.x) * 0.5f), m_Rect.y + floorf((m_Rect.w - card->size.y) * 0.4f)) - ui->root_parent.position;
        card->style_vars[StyleVar_BackgroundColor] = theme.Popup;
        card->style_vars[StyleVar_CornerRadius]    = Vec4(theme.PanelRounding * dpi, 0.0f, 0.0f, 0.0f);
        card->style_vars[StyleVar_Padding]         = Vec4(6.0f * dpi, 6.0f * dpi, 0.0f, 0.0f);
        card->style_vars[StyleVar_ItemSpacing]     = Vec4(0.0f, 2.0f * dpi, 0.0f, 0.0f);
        card->style_vars[StyleVar_ShadowColor]     = Vec4(0.0f, 0.0f, 0.0f, 0.45f);
        card->style_vars[StyleVar_ShadowBlur]      = Vec4(18.0f * dpi, 0.0f, 0.0f, 0.0f);
        for(u32 i = 0; i < count; i++)
        {
            const bool current = (i32)i == m_SwitcherIndex;
            u64 hash;
            char id[96];
            snprintf(id, sizeof(id), "%s###switch%u", UIDockGetPanelTitle(m_Dock, open[i]), open[i]);
            String8 text   = HandleUIString(id, &hash);
            UI_Widget* row = PushWidget(WidgetFlags_DrawText | WidgetFlags_DrawBackground, text, hash,
                                        { SizeKind_PercentOfParent, 1.0f }, { SizeKind_TextContent, 1.0f });
            UIClearBorders(row);
            row->style_vars[StyleVar_Padding]         = Vec4(10.0f * dpi, 6.0f * dpi, 0.0f, 0.0f);
            row->style_vars[StyleVar_CornerRadius]    = Vec4(theme.WidgetRounding * dpi, 0.0f, 0.0f, 0.0f);
            row->style_vars[StyleVar_BackgroundColor] = current ? theme.Selection : Vec4(0.0f);
            UISetTextColour(row, current ? theme.Text : theme.TextDim);
        }
        UIEndChild();
        UIPopStyle(StyleVar_FontSize);
    }

    namespace
    {
        // A text button inside one of the floating view pills.
        bool ViewPillButton(const char* label, bool lit = false)
        {
            const UIToolTheme& theme = UIGetToolTheme();
            const f32 dpi            = GetUIState()->DPIScale;
            u64 hash;
            String8 text      = HandleUIString(label, &hash);
            UI_Widget* button = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_DrawText | WidgetFlags_Clickable, text, hash,
                                           { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
            UIClearBorders(button);
            button->style_vars[StyleVar_FontSize]     = Vec4(theme.FontSize * 0.92f * dpi, 0.0f, 0.0f, 1.0f);
            button->style_vars[StyleVar_Padding]      = Vec4(8.0f * dpi, 4.0f * dpi, 0.0f, 0.0f);
            button->style_vars[StyleVar_CornerRadius] = Vec4(11.0f * dpi, 0.0f, 0.0f, 0.0f);
            UISetBackground(button, lit ? theme.Selection : Vec4(0.0f), theme.Selection, theme.Selection);
            UISetTextColour(button, theme.Text);
            return HandleWidgetInteraction(button).clicked;
        }

        // The pill itself: glass, rounded, placed by the caller once its size is known.
        UI_Widget* BeginViewPill(const char* id)
        {
            const UIToolTheme& theme = UIGetToolTheme();
            const f32 dpi            = GetUIState()->DPIScale;
            UIBeginChild(id, { SizeKind_ChildSum, 1.0f }, { SizeKind_Pixels, floorf(30.0f * dpi) },
                         WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY | WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y);
            UI_Widget* pill                            = GetUIState()->parents.Back();
            Vec4 glass                                 = theme.Popup;
            glass.w                                    = 0.9f;
            pill->style_vars[StyleVar_Padding]         = Vec4(4.0f * dpi, 0.0f, 0.0f, 0.0f);
            pill->style_vars[StyleVar_ItemSpacing]     = Vec4(2.0f * dpi, 0.0f, 0.0f, 0.0f);
            pill->style_vars[StyleVar_CornerRadius]    = Vec4(15.0f * dpi, 0.0f, 0.0f, 0.0f);
            pill->style_vars[StyleVar_BackgroundColor] = glass;
            return pill;
        }

        // What to draw over the picture to see what the engine is doing. Rows of an open popup.
        void DrawViewDebugMenu(Editor* editor, bool sceneView)
        {
            Scene* scene = CurrentScene();
            if(!scene)
                return;
            Scene::SceneRenderSettings& render = scene->GetSettings().RenderSettings;

            UIMenuItem("Show buffer###debugbufferhead", nullptr, nullptr, false);
            static const char* kBuffers[] = { "Final image", "SSAO", "SSAO (raw)", "Normals", "Bloom", "Noise", "Post process" };
            for(int i = 0; i < 7; i++)
            {
                bool on = render.DebugMode == i;
                char item[48];
                snprintf(item, sizeof(item), "%s###debugbuffer%d", kBuffers[i], i);
                if(UIMenuItem(item, nullptr, &on))
                    render.DebugMode = i;
            }
            // forward shading view: what the GI probes alone light each surface with
            if(Graphics::SceneRenderer* renderer = Application::Get().GetSceneRenderer())
            {
                bool giOnly = renderer->GetForwardData().m_RenderMode == 9;
                if(UIMenuItem("GI only###debugbuffergi", nullptr, &giOnly))
                    renderer->GetForwardData().m_RenderMode = giOnly ? 9 : 0;
            }

            if(sceneView)
            {
                UIMenuSeparator();
                UIMenuItem("Editor###debugeditorhead", nullptr, nullptr, false);
                uint32_t& flags = editor->GetSettings().m_DebugDrawFlags;
                auto flag       = [&](const char* label, uint32_t bit)
                {
                    bool on = (flags & bit) != 0;
                    if(UIMenuItem(label, nullptr, &on))
                        flags = on ? (flags | bit) : (flags & ~bit);
                };
                flag("Mesh bounds", EditorDebugFlags::MeshBoundingBoxes);
                flag("Sprite boxes", EditorDebugFlags::SpriteBoxes);
                flag("Camera frustums", EditorDebugFlags::CameraFrustum);
                flag("Entity names", EditorDebugFlags::EntityNames);
            }

            UIMenuSeparator();
            UIMenuItem("Physics###debugphysicshead", nullptr, nullptr, false);
            UIMenuItem("Debug drawing", nullptr, &render.DebugRenderEnabled);
            if(LumosPhysicsEngine* physics = Application::Get().GetSystem<LumosPhysicsEngine>())
            {
                uint32_t flags = physics->GetDebugDrawFlags();
                auto flag      = [&](const char* label, uint32_t bit)
                {
                    bool on = (flags & bit) != 0;
                    if(UIMenuItem(label, nullptr, &on))
                    {
                        flags = on ? (flags | bit) : (flags & ~bit);
                        physics->SetDebugDrawFlags(flags);
                        // a shape asked for is no use with the drawing off
                        if(on)
                            render.DebugRenderEnabled = true;
                    }
                };
                flag("Collision shapes", PhysicsDebugFlags::COLLISIONVOLUMES);
                flag("Bounding boxes", PhysicsDebugFlags::AABB);
                flag("Contacts", PhysicsDebugFlags::MANIFOLD);
                flag("Contact normals", PhysicsDebugFlags::COLLISIONNORMALS);
                flag("Constraints", PhysicsDebugFlags::CONSTRAINT);
                flag("Velocities", PhysicsDebugFlags::LINEARVELOCITY);
                flag("Broadphase", PhysicsDebugFlags::BROADPHASE);
            }
        }

        // A view with its overlays put away keeps one small handle to bring them back.
        bool ViewBarHandle(const char* id, const Vec2& frameSize, f32 edge, f32 top)
        {
            UI_Widget* pill = BeginViewPill(id);
            pill->relative_position = Vec2(frameSize.x - edge - pill->size.x, top);
            const bool show = ViewPillButton(ICON_MDI_CHEVRON_DOWN "###show");
            UITooltip("Show the toolbar");
            UIEndChild();
            return show;
        }
    }

    bool EditorUI::UpdateTouchCamera(const UIDockPanelContext& context, const Vec2& imageOrigin, const Vec2& imageSize, const Vec4& barRect,
                                           bool primaryOnView, bool gizmoHasIt, Vec2& outTap)
    {
        UI_State* ui                       = GetUIState();
        const f32 dpi                      = context.DPIScale;
        Input& input                       = Input::Get();
        TouchCamera& cam                   = m_TouchCamera;
        EditorCameraController& controller = m_Editor->GetEditorCameraController();
        Maths::Transform& transform        = m_Editor->GetEditorCameraTransform();
        const f32 dt                       = Maths::Clamp((f32)Engine::GetTimeStep().GetRawMillis() * 0.001f, 0.0f, 0.1f);

        // ---- fingers down, in the UI's space. A mouse stands in for one on a desktop. ----
        struct Point
        {
            u64 Id;
            Vec2 At;
        };
        Point points[Input::MaxTouchPoints];
        u32 count = 0;
        for(u32 i = 0; i < input.GetTouchPointCount() && count < Input::MaxTouchPoints; i++)
        {
            const Input::TouchPoint& touch = input.GetTouchPoint(i);
            // (ids can be 0: keep 0 for "none")
            points[count++] = { touch.Id + 1, touch.Position * ui->InputScale - ui->InputOffset };
        }
        if(count == 0 && input.GetTouchCount() == 0 && input.GetMouseHeld(InputCode::MouseKey::ButtonLeft))
            points[count++] = { 0xFFFFull, UIMousePosition() };

        auto find = [&](u64 id) -> const Point*
        {
            for(u32 i = 0; i < count; i++)
                if(id && points[i].Id == id)
                    return &points[i];
            return nullptr;
        };

        // ---- the stick: bottom left, clear of the status chip ----
        const Vec4 safe        = ViewInsets(context.Position, context.Size);
        const f32 stickRadius  = floorf(46.0f * dpi);
        const Vec2 stickCentre = Vec2(context.Position.x + safe.x + 22.0f * dpi + stickRadius,
                                      context.Position.y + context.Size.y - safe.w - 58.0f * dpi - stickRadius);

        // ---- fingers that left ----
        bool tapped = false;
        if(cam.StickId && !find(cam.StickId))
        {
            cam.StickId = 0;
            cam.Stick   = Vec2(0.0f);
        }
        if(cam.SecondId && !find(cam.SecondId))
            cam.SecondId = 0;
        if(cam.LookId && !find(cam.LookId))
        {
            if(!cam.Moved && !cam.OnGizmo && !cam.SecondId)
            {
                tapped = true;
                outTap = cam.LookLast - imageOrigin;
            }
            // the one left carries on as the look finger, without a jump
            cam.LookId   = cam.SecondId;
            cam.LookLast = cam.SecondLast;
            cam.SecondId = 0;
            cam.Moved    = true;
            cam.OnGizmo  = false;
        }

        // ---- fingers that arrived ----
        auto inRect = [](const Vec2& p, const Vec4& r) { return p.x >= r.x && p.x <= r.x + r.z && p.y >= r.y && p.y <= r.y + r.w; };
        const Vec4 imageRect = Vec4(imageOrigin.x, imageOrigin.y, imageSize.x, imageSize.y);
        for(u32 i = 0; i < count; i++)
        {
            const Point& point = points[i];
            if(point.Id == cam.LookId || point.Id == cam.SecondId || point.Id == cam.StickId)
                continue;
            // only ones that came down this frame: a finger that slid in from a panel or a bar is not ours
            const bool first = count == 1;
            bool wasDown     = false;
            for(u32 k = 0; k < cam.SeenCount; k++)
                wasDown |= cam.Seen[k] == point.Id;
            if(wasDown)
                continue;
            if(!inRect(point.At, imageRect) || inRect(point.At, barRect))
                continue;
            // the band along the top is for pulling the strip down
            if(!m_StripShown && point.At.y < m_Rect.y + m_Safe.y + 56.0f * dpi)
                continue;
            // the first finger is the UI's pointer: it must really be on the view (no drawer or popup over it)
            if(first && !primaryOnView)
                continue;

            const Vec2 fromStick = point.At - stickCentre;
            if(!cam.StickId && fromStick.x * fromStick.x + fromStick.y * fromStick.y < stickRadius * stickRadius * 1.6f)
                cam.StickId = point.Id;
            else if(!cam.LookId)
            {
                // a second finger never starts a look on its own while the stick has the first: that is move + look
                cam.LookId    = point.Id;
                cam.LookStart = cam.LookLast = point.At;
                cam.Moved     = false;
                cam.OnGizmo   = first && gizmoHasIt;
            }
            else if(!cam.SecondId)
            {
                cam.SecondId   = point.Id;
                cam.SecondLast = point.At;
                cam.Moved      = true;
            }
        }

        cam.SeenCount = 0;
        for(u32 i = 0; i < count && cam.SeenCount < 10; i++)
            cam.Seen[cam.SeenCount++] = points[i].Id;

        bool changed = false;

        // ---- fly ----
        if(const Point* stick = find(cam.StickId))
        {
            Vec2 offset    = (stick->At - stickCentre) * (1.0f / stickRadius);
            const f32 size = sqrtf(offset.x * offset.x + offset.y * offset.y);
            if(size > 1.0f)
                offset = offset * (1.0f / size);
            cam.Stick = offset;
            // a dead middle, then faster the further out
            const f32 push = Maths::Max(0.0f, Maths::Min(size, 1.0f) - 0.12f) / 0.88f;
            if(push > 0.0f && size > 0.0001f)
            {
                const Vec2 direction = offset * (1.0f / Maths::Max(Maths::Min(size, 1.0f), 0.0001f));
                const Vec3 move      = transform.GetForwardDirection() * -direction.y + transform.GetRightDirection() * direction.x;
                transform.SetLocalPosition(transform.GetLocalPosition() + move * (controller.GetSpeed() * push * push * dt));
                changed = true;
            }
        }

        const Point* look   = find(cam.LookId);
        const Point* second = find(cam.SecondId);
        if(look && second && !cam.OnGizmo)
        {
            // two fingers: the middle of them drags the view sideways, the gap between them moves it in and out
            const Vec2 centreNow    = (look->At + second->At) * 0.5f;
            const Vec2 centreBefore = (cam.LookLast + cam.SecondLast) * 0.5f;
            const Vec2 drag         = (centreNow - centreBefore) * (1.0f / dpi);
            if(drag.x != 0.0f || drag.y != 0.0f)
            {
                controller.MousePan(transform, drag * 6.0f);
                changed = true;
            }
            const Vec2 gapNow    = look->At - second->At;
            const Vec2 gapBefore = cam.LookLast - cam.SecondLast;
            const f32 now        = sqrtf(gapNow.x * gapNow.x + gapNow.y * gapNow.y);
            const f32 before     = sqrtf(gapBefore.x * gapBefore.x + gapBefore.y * gapBefore.y);
            if(before > 8.0f * dpi && now > 8.0f * dpi && now != before)
            {
                controller.HandleGesturePinch(transform, now / before, 0.0f, dt);
                changed = true;
            }
            cam.LookLast   = look->At;
            cam.SecondLast = second->At;
        }
        else if(look && !cam.OnGizmo)
        {
            const Vec2 travelled = look->At - cam.LookStart;
            const f32 slop       = 9.0f * dpi;
            if(!cam.Moved && travelled.x * travelled.x + travelled.y * travelled.y > slop * slop)
            {
                cam.Moved    = true;
                cam.LookLast = look->At; // no jump for the slop already covered
            }
            if(cam.Moved)
            {
                const Vec2 delta = (look->At - cam.LookLast) * (1.0f / dpi);
                if(delta.x != 0.0f || delta.y != 0.0f)
                {
                    // the controller's two-finger branch is free look (and a pan in a 2D view)
                    controller.HandleGesturePan(transform, delta * 2.6f, Vec2(0.0f), 2);
                    changed = true;
                }
                cam.LookLast = look->At;
            }
        }
        else if(look)
            cam.LookLast = look->At;

        if(changed)
            transform.SetWorldMatrix(Mat4(1.0f));

        // ---- draw the stick ----
        {
            const UIToolTheme& theme = UIGetToolTheme();
            const bool held          = cam.StickId != 0;
            Vec4 ring                = theme.Text;
            ring.w                   = held ? 0.55f : 0.28f;
            Vec4 pad                 = theme.Popup;
            pad.w                    = held ? 0.5f : 0.3f;
            Vec4 knob                = held ? theme.Accent : theme.Text;
            knob.w                   = held ? 0.9f : 0.45f;
            const f32 r              = stickRadius / dpi;
            const Vec2 centre        = stickCentre * (1.0f / dpi);
            const f32 knobRadius     = 17.0f;
            const Vec2 knobAt        = centre + cam.Stick * (r - knobRadius);
            UIRect("##scenestickpad", centre.x - r, centre.y - r, r * 2.0f, r * 2.0f, pad, r);
            UIRing("##scenestickring", centre.x, centre.y, r, ring, 1.5f);
            UIRect("##scenestickknob", knobAt.x - knobRadius, knobAt.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f, knob, knobRadius);
        }
        return tapped;
    }

    void EditorUI::DrawSceneView(const UIDockPanelContext& context)
    {
        EditorUI* self = (EditorUI*)context.UserData;
        Editor* editor       = self->m_Editor;
        Application& app     = Application::Get();
        Scene* scene         = CurrentScene();
        SceneViewPanel* view = self->m_SceneView;
        const f32 dpi        = context.DPIScale;

        if(!scene || !view)
        {
            app.SetDisableMainSceneRenderer(true);
            return;
        }

        Camera* camera              = editor->GetCamera();
        Maths::Transform* transform = &editor->GetEditorCameraTransform();
        if(!camera)
            return;

        app.SetDisableMainSceneRenderer(false);
        app.GetSceneRenderer()->SetOverrideCamera(camera, transform);
        app.GetSceneRenderer()->m_EnableUIPass = false;

        const UIToolTheme& theme = UIGetToolTheme();
        const bool floating      = theme.FloatingToolbars;

        // Everything the view offers, in one bar: gizmo tool, axes + snap, camera, options.
        // narrow = a phone held upright: the tools fold into one button.
        auto drawBar = [&](bool narrow, bool canHide)
        {
            EditorCameraController& controller = editor->GetEditorCameraController();
            const u32 current                  = editor->GetGizmoOperation();
            struct Tool
            {
                const char* Icon;
                const char* Name;
                const char* Tip;
                const char* Id;
                u32 Operation;
            };
            static const Tool kTools[] = {
                { ICON_MDI_CURSOR_DEFAULT, "Select", "Select", "toolselect", (u32)GizmoOperation_None },
                { ICON_MDI_ARROW_ALL, "Move", "Move (W)", "toolmove", (u32)GizmoOperation_Translate },
                { ICON_MDI_ROTATE_RIGHT, "Rotate", "Rotate (E)", "toolrotate", (u32)GizmoOperation_Rotate },
                { ICON_MDI_VECTOR_SQUARE, "Scale", "Scale (R)", "toolscale", (u32)GizmoOperation_Scale },
                { ICON_MDI_AXIS_ARROW, "Move, rotate and scale", "Move, rotate and scale (T)", "tooluniversal", (u32)GizmoOperation_Universal },
            };
            char label[96];

            if(!narrow)
            {
                for(const Tool& tool : kTools)
                {
                    snprintf(label, sizeof(label), "%s###%s", tool.Icon, tool.Id);
                    if(UIIconButton(label, 26.0f, current == tool.Operation).clicked)
                        editor->SetGizmoOperation(tool.Operation);
                    UITooltip(tool.Tip);
                }
                UISpacer(6.0f);
                if(UIIconButton(ICON_MDI_CUBE_OUTLINE "###toollocal", 26.0f, self->m_GizmoLocal).clicked)
                    self->m_GizmoLocal = !self->m_GizmoLocal;
                UITooltip("Local axes");
                if(UIIconButton(ICON_MDI_RULER "###toolsnap", 26.0f, editor->SnapGuizmo()).clicked)
                    editor->ToggleSnap();
                UITooltip("Snap");
                UISpacer(6.0f);
            }
            else
            {
                // the tool in hand; press for the others
                const char* icon = kTools[0].Icon;
                for(const Tool& tool : kTools)
                    if(tool.Operation == current)
                        icon = tool.Icon;
                snprintf(label, sizeof(label), "%s###toolpick", icon);
                if(UIIconButton(label, 26.0f, true).clicked)
                    UIOpenPopup("##scenetoolmenu");
                if(UIBeginPopup("##scenetoolmenu"))
                {
                    for(const Tool& tool : kTools)
                    {
                        bool on = current == tool.Operation;
                        snprintf(label, sizeof(label), "%s  %s###pick%s", tool.Icon, tool.Name, tool.Id);
                        if(UIMenuItem(label, nullptr, &on))
                            editor->SetGizmoOperation(tool.Operation);
                    }
                    UIMenuSeparator();
                    UIMenuItem("Local axes", nullptr, &self->m_GizmoLocal);
                    bool snap = editor->SnapGuizmo();
                    if(UIMenuItem("Snap", nullptr, &snap))
                        editor->ToggleSnap();
                    UIEndPopup();
                }
            }

            // ---- camera ----
            const f32 speed = controller.GetSpeed();
            snprintf(label, sizeof(label), ICON_MDI_SPEEDOMETER "  %.*f###chipspeed", speed < 10.0f ? 1 : 0, speed);
            if(ViewPillButton(label))
                UIOpenPopup("##cameraspeed");
            UITooltip("Camera speed. Hold the right mouse button and scroll to change it while flying");
            if(UIBeginPopup("##cameraspeed"))
            {
                // the slider is in powers of ten: fine steps when slow, coarse when fast
                f32 power = log10f(Maths::Max(speed, 0.1f));
                UIToolSlider("###cameraspeedslider", &power, -1.0f, 3.0f, 170.0f);
                const f32 chosen = powf(10.0f, power);
                if(Maths::Abs(chosen - speed) > speed * 0.005f)
                    controller.SetSpeed(Maths::Clamp(chosen, 0.1f, 1000.0f));
                UIMenuSeparator();
                static const f32 kPresets[] = { 1.0f, 5.0f, 20.0f, 100.0f, 500.0f };
                for(f32 preset : kPresets)
                {
                    char item[48];
                    snprintf(item, sizeof(item), "%.0f%s###speed%.0f", preset, preset == 20.0f ? "   (default)" : "", preset);
                    if(UIMenuItem(item))
                        controller.SetSpeed(preset);
                }
                UIEndPopup();
            }

            const bool ortho = camera->IsOrthographic();
            if(narrow ? UIIconButton(ortho ? ICON_MDI_VECTOR_SQUARE "###chipprojection" : ICON_MDI_CAMERA "###chipprojection", 26.0f).clicked
                      : ViewPillButton(ortho ? ICON_MDI_VECTOR_SQUARE "  Ortho###chipprojection" : ICON_MDI_CAMERA "  Persp###chipprojection"))
                camera->SetIsOrthographic(!ortho);
            UITooltip("Perspective / orthographic");

            // ---- draw less, to work faster: this view only ----
            {
                uint32_t& off = editor->GetSettings().m_SceneViewOff;
                if(UIIconButton(ICON_MDI_FLASH "###chipfast", 26.0f, off != 0).clicked)
                    UIOpenPopup("##sceneviewfast");
                UITooltip(off ? "Some rendering is switched off in this view" : "Switch rendering features off in this view");
                if(UIBeginPopup("##sceneviewfast"))
                {
                    bool fast = off == SceneViewOff_All;
                    if(UIMenuItem(ICON_MDI_FLASH "  Fast preview", nullptr, &fast))
                        off = fast ? (uint32_t)SceneViewOff_All : 0u;
                    UIMenuSeparator();
                    static const struct
                    {
                        const char* Name;
                        uint32_t Bit;
                    } kFeatures[] = {
                        { "Shadows", SceneViewOff_Shadows }, { "Bloom", SceneViewOff_Bloom }, { "Ambient occlusion", SceneViewOff_SSAO },
                        { "Reflections", SceneViewOff_SSR }, { "Anti-aliasing", SceneViewOff_AntiAliasing }, { "Depth of field", SceneViewOff_DepthOfField },
                        { "Other post effects", SceneViewOff_PostEffects }, { "Sky", SceneViewOff_Skybox },
                    };
                    for(const auto& feature : kFeatures)
                    {
                        // ticked = drawn
                        bool on = (off & feature.Bit) == 0;
                        if(UIMenuItem(feature.Name, nullptr, &on))
                            off = on ? (off & ~feature.Bit) : (off | feature.Bit);
                    }
                    UIMenuSeparator();
                    UIMenuItem("This view only: scene settings are not changed###fastnote", nullptr, nullptr, false);
                    UIEndPopup();
                }
            }

            // ---- the rest: what is drawn over the scene ----
            if(UIIconButton(ICON_MDI_TUNE "###chipviewoptions", 26.0f).clicked)
                UIOpenPopup("##sceneviewoptions");
            UITooltip("View options and debug drawing");
            if(UIBeginPopup("##sceneviewoptions"))
            {
                bool grid = editor->ShowGrid();
                if(UIMenuItem("Grid", nullptr, &grid))
                    editor->ShowGrid() = grid;
                bool gizmos = editor->ShowGizmos();
                if(UIMenuItem("Gizmos", nullptr, &gizmos))
                    editor->ShowGizmos() = gizmos;
                if(UIBeginMenu(ICON_MDI_BUG "  Debug drawing"))
                {
                    DrawViewDebugMenu(editor, true);
                    UIEndMenu();
                }
                if(TerrainSculpt* sculpt = editor->GetTerrainSculpt())
                {
                    if(scene->GetRegistry().view<TerrainComponent>().size() > 0)
                    {
                        bool on = sculpt->IsActive();
                        if(UIMenuItem(ICON_MDI_TERRAIN "  Sculpt Terrain", "T", &on))
                            sculpt->SetEnabled(on);
                    }
                }
                if(scene->GetRegistry().view<VoxelWorldComponent>().size() > 0)
                    UIMenuItem(ICON_MDI_CUBE_OUTLINE "  Edit Voxels", "B", &self->m_VoxelEdit);
                UIMenuSeparator();
                if(UIMenuItem("Reset Camera"))
                {
                    transform->SetLocalPosition(Vec3(-31.0f, 12.0f, 51.0f));
                    transform->SetLocalOrientation(Quat(Vec3(-20.0f, -40.0f, 0.0f)));
                    controller.SetSpeed(20.0f);
                }
                if(UIMenuItem("Scene Settings..."))
                    UIDockFocusPanel(self->m_Dock, "Scene Settings");
                UIEndPopup();
            }

            if(canHide)
            {
                UIPushStyle(StyleVar_TextColor, theme.TextDim);
                if(UIIconButton(ICON_MDI_CHEVRON_UP "###chiphide", 26.0f).clicked)
                    self->m_SceneBarHidden = true;
                UIPopStyle(StyleVar_TextColor);
                UITooltip("Hide the bar");
            }
        };
        const bool narrowBar = context.Size.x < 500.0f * dpi;

        // Either a strip above the view, or (floating) the view fills the panel as a
        // rounded card and the tools sit on top of it.
        f32 toolbar = 0.0f;
        f32 inset   = 0.0f;
        if(!floating)
        {
            toolbar = floorf(32.0f * dpi);
            UIBeginChild("##scenetoolbar", { SizeKind_PercentOfParent, 1.0f }, { SizeKind_Pixels, toolbar },
                         WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY);
            UI_Widget* bar                        = GetUIState()->parents.Back();
            bar->style_vars[StyleVar_Padding]     = Vec4(6.0f * dpi, 0.0f, 0.0f, 0.0f);
            bar->style_vars[StyleVar_ItemSpacing] = Vec4(2.0f * dpi, 0.0f, 0.0f, 0.0f);
            drawBar(narrowBar, false);
            UIEndChild();
        }
        else
        {
            // (a touch layout has no cards: the picture runs to the edges)
            inset = self->m_Touch ? 0.0f : floorf(Maths::Min(theme.PanelGap, 6.0f) * dpi);
        }

        // overlays keep out of the notch; the picture does not
        const Vec4 safe      = self->ViewInsets(context.Position, context.Size);
        const f32 overlayTop = 10.0f * dpi + safe.y;

        // ---- the scene ----
        Vec2 size = Vec2(context.Size.x - inset * 2.0f, context.Size.y - toolbar - inset);
        size.x    = Maths::Max(floorf(size.x), 2.0f);
        size.y    = Maths::Max(floorf(size.y), 2.0f);
        size.x -= ((int)size.x % 2 != 0) ? 1.0f : 0.0f;
        size.y -= ((int)size.y % 2 != 0) ? 1.0f : 0.0f;

        // the picture is drawn at the panel's size straight away; what it is rendered at catches up when the panel settles
        const Vec2 target  = self->m_SceneTarget.Settle(size);
        const float aspect = target.x / target.y;
        if(!Maths::Equals(aspect, camera->GetAspectRatio()))
            camera->SetAspectRatio(aspect);

        view->SetViewScale(Maths::Clamp(editor->GetSettings().m_SceneViewScale, 0.25f, 2.0f));
        view->Resize((uint32_t)target.x, (uint32_t)target.y);

        // a frame the size of the rest of the panel: the image sits centred in it, the floating tools on top
        // (no centring flag on the frame: it would drag the floating chips to the middle as well)
        UIBeginChild("##sceneframe", { SizeKind_PercentOfParent, 1.0f }, { SizeKind_Pixels, context.Size.y - toolbar },
                     WidgetFlags_StackVertically);
        GetUIState()->parents.Back()->style_vars[StyleVar_Padding] = Vec4(floorf((context.Size.x - size.x) * 0.5f), 0.0f, 0.0f, 0.0f);

        const bool flip      = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();
        UI_Interaction image = UIImageSized("##sceneimage", view->GetGameViewTexture(), size, flip);
        if(image.widget && floating)
            image.widget->style_vars[StyleVar_CornerRadius] = Vec4(self->m_Touch ? 0.0f : Maths::Max(0.0f, theme.PanelRounding - 2.0f) * dpi, 0.0f, 0.0f, 0.0f);

        // a file dragged out of the asset browser lands in the scene
        {
            char dropped[256] = { 0 };
            if(UIDropTarget(ToolPanels::kAssetDragType, dropped, sizeof(dropped)) && dropped[0])
                editor->FileOpenCallback(dropped);
        }

        // ---- corner chips: what the view is doing, small and out of the way ----
        {
            const Vec2 frameSize = Vec2(context.Size.x, context.Size.y - toolbar);
            const f32 edge       = inset + 10.0f * dpi;
            const f32 edgeL      = edge + safe.x, edgeR = edge + safe.z, edgeB = edge + safe.w;
            Vec4 glass           = theme.Popup;
            glass.w              = 0.88f;

            // ---- things with no shape of their own: lights, cameras and sounds get a marker that can be pressed ----
            if(editor->ShowGizmos())
            {
                const Mat4 viewProj = camera->GetProjectionMatrix() * Mat4::Inverse(transform->GetWorldMatrix());
                const f32 offsetX   = floorf((context.Size.x - size.x) * 0.5f);
                const f32 markSize  = floorf(22.0f * dpi);
                auto& registry      = scene->GetRegistry();
                u32 drawn           = 0;
                Entity pressed;
                auto mark = [&](entt::entity handle, const char* icon, const Vec4& colour)
                {
                    if(drawn >= 64)
                        return;
                    Maths::Transform* at = registry.try_get<Maths::Transform>(handle);
                    if(!at)
                        return;
                    const Vec3 world = at->GetWorldPosition();
                    const Vec4 clip  = viewProj * Vec4(world, 1.0f);
                    if(clip.w <= 0.001f)
                        return; // behind the camera
                    const Vec2 screen = Maths::WorldToScreen(world, viewProj, size.x, size.y, 0.0f, 0.0f);
                    if(screen.x < 0.0f || screen.y < 0.0f || screen.x > size.x || screen.y > size.y)
                        return;
                    drawn++;

                    UI_Widget* widget = PushWidget(WidgetFlags_DrawText | WidgetFlags_DrawBackground | WidgetFlags_Clickable | WidgetFlags_Floating_X | WidgetFlags_Floating_Y,
                                                   Str8C((char*)icon), UIHashCombine(0x53434D41524Bull, (u64)entt::to_integral(handle)),
                                                   { SizeKind_Pixels, markSize }, { SizeKind_Pixels, markSize });
                    UIClearBorders(widget);
                    widget->TextAlignment                     = UI_Text_Alignment_Center_X | UI_Text_Alignment_Center_Y;
                    widget->relative_position                 = Vec2(offsetX + floorf(screen.x - markSize * 0.5f), floorf(screen.y - markSize * 0.5f));
                    widget->style_vars[StyleVar_FontSize]     = Vec4(theme.FontSize * 1.05f * dpi, 0.0f, 0.0f, 1.0f);
                    widget->style_vars[StyleVar_Padding]      = Vec4(0.0f);
                    widget->style_vars[StyleVar_CornerRadius] = Vec4(markSize * 0.5f, 0.0f, 0.0f, 0.0f);
                    Vec4 back                                 = theme.Popup;
                    back.w                                    = 0.72f;
                    UISetBackground(widget, back, theme.Selection, theme.Selection);
                    UISetTextColour(widget, colour);
                    if(HandleWidgetInteraction(widget).clicked)
                        pressed = Entity(handle, scene);
                };
                for(auto handle : registry.view<Graphics::Light>())
                    mark(handle, ICON_MDI_LIGHTBULB, Vec4(1.0f, 0.86f, 0.45f, 1.0f));
                for(auto handle : registry.view<Camera>())
                    mark(handle, ICON_MDI_CAMERA, theme.Text);
                for(auto handle : registry.view<SoundComponent>())
                    mark(handle, ICON_MDI_VOLUME_HIGH, Vec4(0.45f, 0.85f, 0.6f, 1.0f));
                if(pressed.Valid())
                {
                    editor->ClearSelected();
                    editor->SetSelected(pressed);
                }
            }

            // ---- a 2D view gets a flat grid in world units (the 3D one is the editor's ground grid) ----
            if(camera->IsOrthographic() && editor->ShowGrid())
            {
                const Vec3 at    = transform->GetWorldPosition();
                const f32 halfH  = Maths::Max(camera->GetScale(), 0.001f);
                const f32 halfW  = halfH * (size.x / Maths::Max(size.y, 1.0f));
                // whole units, coarser by tens until there are few enough lines to read
                f32 step = 1.0f;
                while(halfW * 2.0f / step > 60.0f)
                    step *= 10.0f;
                const Vec4 minor = Vec4(0.75f, 0.78f, 0.82f, 0.16f);
                const Vec4 major = Vec4(0.75f, 0.78f, 0.82f, 0.34f);
                const f32 left   = floorf((at.x - halfW) / step) * step;
                const f32 bottom = floorf((at.y - halfH) / step) * step;
                for(f32 x = left; x <= at.x + halfW; x += step)
                {
                    const bool tenth = Maths::Abs(fmodf(x, step * 10.0f)) < step * 0.5f;
                    DebugRenderer::DrawHairLine(Vec3(x, at.y - halfH, 0.0f), Vec3(x, at.y + halfH, 0.0f), false, x == 0.0f ? Vec4(0.45f, 0.85f, 0.5f, 0.6f) : (tenth ? major : minor));
                }
                for(f32 y = bottom; y <= at.y + halfH; y += step)
                {
                    const bool tenth = Maths::Abs(fmodf(y, step * 10.0f)) < step * 0.5f;
                    DebugRenderer::DrawHairLine(Vec3(at.x - halfW, y, 0.0f), Vec3(at.x + halfW, y, 0.0f), false, y == 0.0f ? Vec4(0.9f, 0.45f, 0.45f, 0.6f) : (tenth ? major : minor));
                }
                DebugRenderer::RequestPass();
            }

            // anchor: 0 = left / top, 1 = right / bottom. Returns true when pressed.
            auto chip = [&](const char* id, const char* text, f32 anchorX, f32 anchorY, bool clickable)
            {
                u64 hash;
                HandleUIString(id, &hash);
                UI_Widget* widget = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_DrawText | WidgetFlags_Floating_X | WidgetFlags_Floating_Y
                                                   | (clickable ? WidgetFlags_Clickable : 0),
                                               Str8C((char*)text), hash, { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
                UIClearBorders(widget);
                widget->style_vars[StyleVar_FontSize]     = Vec4(theme.FontSize * 0.92f * dpi, 0.0f, 0.0f, 1.0f);
                widget->style_vars[StyleVar_Padding]      = Vec4(9.0f * dpi, 4.0f * dpi, 0.0f, 0.0f);
                widget->style_vars[StyleVar_CornerRadius] = Vec4(11.0f * dpi, 0.0f, 0.0f, 0.0f);
                UISetBackground(widget, glass, clickable ? theme.Selection : glass, clickable ? theme.Selection : glass);
                UISetTextColour(widget, clickable ? theme.Text : theme.TextDim);
                // placed from its own size last frame
                widget->relative_position = Vec2(anchorX > 0.5f ? frameSize.x - edgeR - widget->size.x : edgeL,
                                                 anchorY > 0.5f ? frameSize.y - edgeB - widget->size.y : overlayTop);
                return clickable && HandleWidgetInteraction(widget).clicked;
            };

            // the bar put away leaves a handle to bring it back
            EditorCameraController& controller = editor->GetEditorCameraController();
            f32 speed                          = controller.GetSpeed();
            if(floating && self->m_SceneBarHidden && ViewBarHandle("##sceneviewhandle", frameSize, edgeR, overlayTop))
                self->m_SceneBarHidden = false;

            // ---- the speed, shown large for a moment whenever it changes ----
            speed = controller.GetSpeed();
            if(self->m_SpeedShown >= 0.0f && Maths::Abs(speed - self->m_SpeedShown) > 0.0001f)
                self->m_SpeedFlash = 1.4f;
            self->m_SpeedShown = speed;
            if(self->m_SpeedFlash > 0.0f)
            {
                self->m_SpeedFlash -= (f32)Engine::GetTimeStep().GetSeconds();
                const f32 fade = Maths::Clamp(self->m_SpeedFlash / 0.35f, 0.0f, 1.0f);

                UIPushStyle(StyleVar_Alpha, fade);
                UIBeginChild("##scenespeedhud", { SizeKind_Pixels, floorf(230.0f * dpi) }, { SizeKind_Pixels, floorf(54.0f * dpi) },
                             WidgetFlags_StackVertically | WidgetFlags_CentreChildrenX | WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y);
                UI_Widget* hud                            = GetUIState()->parents.Back();
                hud->relative_position                    = Vec2(floorf((frameSize.x - hud->size.x) * 0.5f), frameSize.y - edgeB - hud->size.y - 28.0f * dpi);
                hud->style_vars[StyleVar_Padding]         = Vec4(14.0f * dpi, 8.0f * dpi, 0.0f, 0.0f);
                hud->style_vars[StyleVar_ItemSpacing]     = Vec4(0.0f, 6.0f * dpi, 0.0f, 0.0f);
                hud->style_vars[StyleVar_CornerRadius]    = Vec4(12.0f * dpi, 0.0f, 0.0f, 0.0f);
                hud->style_vars[StyleVar_BackgroundColor] = glass;

                char label[64];
                snprintf(label, sizeof(label), ICON_MDI_SPEEDOMETER "   Camera speed  %.*f", speed < 10.0f ? 1 : 0, speed);
                UIPushStyle(StyleVar_TextColor, theme.Text);
                UILabelCStr("##scenespeedlabel", label);
                UIPopStyle(StyleVar_TextColor);
                // where it sits between slowest and fastest (log scale, like the slider)
                UIProgressBar("##scenespeedbar", Maths::Clamp((log10f(Maths::Max(speed, 0.1f)) + 1.0f) / 4.0f, 0.0f, 1.0f), 200.0f, 5.0f);
                UIEndChild();
                UIPopStyle(StyleVar_Alpha);
            }

            // one line of status: where the camera is, and what is selected
            char text[160];
            const Vec3 at         = transform->GetWorldPosition();
            const size_t selected = editor->GetSelected().size();
            Entity front          = selected >= 1 ? editor->GetSelected().front() : Entity();
            if(selected == 1 && front.Valid())
                snprintf(text, sizeof(text), "%.1f   %.1f   %.1f      " ICON_MDI_CUBE_OUTLINE "  %.60s", at.x, at.y, at.z, front.GetName().c_str());
            else if(selected > 1)
                snprintf(text, sizeof(text), "%.1f   %.1f   %.1f      " ICON_MDI_SELECTION "  %u selected", at.x, at.y, at.z, (u32)selected);
            else
                snprintf(text, sizeof(text), "%.1f   %.1f   %.1f", at.x, at.y, at.z);
            chip("##chipcamera", text, 0.0f, 1.0f, false);
        }

        Vec4 toolsRect = Vec4(0.0f);
        if(floating && !self->m_SceneBarHidden)
        {
            UIBeginChild("##scenetools", { SizeKind_ChildSum, 1.0f }, { SizeKind_Pixels, floorf(36.0f * dpi) },
                         WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY | WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y);
            UI_Widget* pill                         = GetUIState()->parents.Back();
            pill->relative_position                 = Vec2(floorf((context.Size.x - pill->size.x) * 0.5f), overlayTop);
            pill->style_vars[StyleVar_Padding]      = Vec4(5.0f * dpi, 0.0f, 0.0f, 0.0f);
            pill->style_vars[StyleVar_ItemSpacing]  = Vec4(2.0f * dpi, 0.0f, 0.0f, 0.0f);
            pill->style_vars[StyleVar_CornerRadius] = Vec4((theme.WidgetRounding + 4.0f) * dpi, 0.0f, 0.0f, 0.0f);
            Vec4 glass                              = theme.Popup;
            glass.w                                 = 0.92f;
            pill->style_vars[StyleVar_BackgroundColor] = glass;
            toolsRect                               = Vec4(pill->position.x, pill->position.y, pill->size.x, pill->size.y);
            drawBar(narrowBar, true);
            UIEndChild();
        }
        UIEndChild(); // frame

        // where the image actually sits (last frame's layout; the first frame falls back to where it should be)
        const Vec2 origin    = (image.widget && !image.widget->FirstFrame) ? image.widget->position
                                                                           : Vec2(context.Position.x, context.Position.y + toolbar);
        const Vec2 mouse     = UIMousePosition() - origin;
        const Vec2 pointer   = UIMousePosition();
        const bool onTools   = pointer.x >= toolsRect.x && pointer.x <= toolsRect.x + toolsRect.z
            && pointer.y >= toolsRect.y && pointer.y <= toolsRect.y + toolsRect.w;
        // hot = nothing drawn over it (a drawer, a popup, a bar) has the pointer
        UI_State* ui         = GetUIState();
        const bool onImage   = image.widget && (ui->hot_widget == image.widget->hash || ui->active_widget == image.widget->hash);
        const bool hovered   = image.hovering && onImage && !onTools;

        Input& input          = Input::Get();
        const bool leftDown   = input.GetMouseHeld(InputCode::MouseKey::ButtonLeft);
        const bool leftPress  = input.GetMouseClicked(InputCode::MouseKey::ButtonLeft);
        const bool rightDown  = input.GetMouseHeld(InputCode::MouseKey::ButtonRight);

        // the camera moves while the pointer is over the view, and keeps a drag that started there
        const bool middleDown = input.GetMouseHeld(InputCode::MouseKey::ButtonMiddle);
        if(hovered && (rightDown || middleDown))
            self->m_CameraDrag = true;
        else if(!rightDown && !middleDown)
            self->m_CameraDrag = false;
        editor->SetSceneViewActive((hovered || self->m_CameraDrag) && !app.GetSceneActive());

        // ---- terrain sculpt / voxel edit: they own the left button while on ----
        if(self->SceneTools(editor, scene, camera, transform, mouse, size, origin, hovered && !rightDown))
        {
            editor->SetHoveredEntity({});
            return;
        }

        // ---- gizmo ----
        bool usingGizmo = false;
        {
            constexpr u32 kMax = 64;
            entt::entity selected[kMax];
            u32 count = 0;
            for(Entity entity : editor->GetSelected())
            {
                if(count >= kMax)
                    break;
                if(entity.Valid())
                    selected[count++] = entity.GetHandle();
            }

            const u32 operation = editor->GetGizmoOperation();
            if(editor->ShowGizmos() && operation != GizmoOperation_None && count > 0)
            {
                GizmoView gizmoView = TransformGizmo::ViewFromCamera(camera, transform, size);

                GizmoInput gizmoInput;
                gizmoInput.Mouse   = mouse;
                gizmoInput.Down    = leftDown;
                gizmoInput.Pressed = leftPress;
                gizmoInput.Enabled = hovered && !rightDown && !UIIsDragDropActive() && !UIDockIsDragging(self->m_Dock);

                GizmoSettings settings;
                settings.DPIScale      = dpi;
                settings.Snap          = editor->SnapGuizmo();
                settings.SnapTranslate = editor->SnapAmount();

                usingGizmo = TransformGizmo::ManipulateEntities(scene, selected, count, gizmoView, gizmoInput, operation,
                                                       self->m_GizmoLocal ? GizmoSpace::Local : GizmoSpace::World, settings);
            }
        }

        // ---- touch: fingers drive the camera, and only a tap selects ----
        if(self->m_Touch)
        {
            Vec2 tap;
            const bool overGizmo = usingGizmo || TransformGizmo::IsOver();
            if(self->UpdateTouchCamera(context, origin, size, toolsRect, hovered, overGizmo, tap) && !overGizmo)
                editor->SelectObject(editor->GetScreenRay((int)tap.x, (int)tap.y, camera, (int)size.x, (int)size.y));
            editor->SetHoveredEntity({});
            return;
        }

        // ---- picking ----
        const bool canPick = hovered && !usingGizmo && !TransformGizmo::IsOver() && !rightDown && input.GetMouseMode() == MouseMode::Visible;
        if(canPick)
        {
            Maths::Ray ray = editor->GetScreenRay((int)mouse.x, (int)mouse.y, camera, (int)size.x, (int)size.y);
            if(leftPress)
                editor->SelectObject(ray);
            else if(!leftDown)
                editor->SelectObject(ray, true);
        }
        else
            editor->SetHoveredEntity({});
    }

    bool EditorUI::SceneTools(Editor* editor, Scene* scene, Camera* camera, Maths::Transform* transform, const Vec2& mouse, const Vec2& size, const Vec2& origin, bool hovered)
    {
        Input& input        = Input::Get();
        TerrainSculpt* sculpt = editor->GetTerrainSculpt();
        const bool typing   = GetUIState()->FocusedTextInput != 0;
        const bool shift    = input.GetKeyHeld(InputCode::Key::LeftShift) || input.GetKeyHeld(InputCode::Key::RightShift);
        const bool ctrl     = input.GetKeyHeld(InputCode::Key::LeftControl) || input.GetKeyHeld(InputCode::Key::LeftSuper);
        auto& registry      = scene->GetRegistry();

        if(sculpt)
            sculpt->FlushDirty(); // one mesh + collision rebuild per touched terrain a frame

        if(hovered && !typing && !ctrl && !shift)
        {
            if(sculpt && input.GetKeyPressed(InputCode::Key::T) && registry.view<TerrainComponent>().size() > 0)
                sculpt->SetEnabled(!sculpt->IsActive());
            if(input.GetKeyPressed(InputCode::Key::B) && registry.view<VoxelWorldComponent>().size() > 0)
                m_VoxelEdit = !m_VoxelEdit;
        }

        // voxels: click removes a block, Shift+click places one against the face
        Graphics::VoxelWorld* voxels = nullptr;
        if(m_VoxelEdit)
        {
            auto view = registry.view<VoxelWorldComponent>();
            if(view.begin() != view.end())
                voxels = registry.get<VoxelWorldComponent>(view.front()).World.get();
            if(!voxels)
                m_VoxelEdit = false;
        }
        if(voxels && hovered)
        {
            if(input.GetMouseClicked(InputCode::MouseKey::ButtonLeft))
            {
                Maths::Ray ray = editor->GetScreenRay((int)mouse.x, (int)mouse.y, camera, (int)size.x, (int)size.y);
                if(shift)
                    voxels->PlaceBlock(ray.Origin, ray.Direction, 256.0f, Graphics::Block_Solid);
                else
                    voxels->RemoveBlock(ray.Origin, ray.Direction, 256.0f);
            }
            if(input.GetKeyPressed(InputCode::Key::Escape))
                m_VoxelEdit = false;
            return true;
        }

        if(!sculpt || !sculpt->IsActive())
            return false;
        DrawSculptCard(sculpt, origin, size);
        if(!hovered)
        {
            if(!input.GetMouseHeld(InputCode::MouseKey::ButtonLeft))
                sculpt->EndStroke();
            return false;
        }

        Maths::Ray ray     = editor->GetScreenRay((int)mouse.x, (int)mouse.y, camera, (int)size.x, (int)size.y);
        entt::entity hitEntity = entt::null;
        Vec3 hit;
        const bool gotHit = sculpt->RaycastTerrain(ray, hitEntity, hit);
        if(gotHit)
        {
            Vec4 colour = Vec4(0.24f, 0.86f, 0.43f, 0.86f);
            switch(sculpt->CurrentTool())
            {
            case TerrainTool::Lower: colour = Vec4(0.9f, 0.35f, 0.24f, 0.86f); break;
            case TerrainTool::Smooth: colour = Vec4(0.35f, 0.63f, 0.9f, 0.86f); break;
            case TerrainTool::Flatten: colour = Vec4(0.9f, 0.78f, 0.24f, 0.86f); break;
            case TerrainTool::Noise: colour = Vec4(0.78f, 0.35f, 0.9f, 0.86f); break;
            default: break;
            }
            constexpr int kRing = 32;
            Vec3 ring[kRing];
            sculpt->SampleBrushRing(hit, kRing, ring);
            for(int i = 0; i < kRing; i++)
                DebugRenderer::DrawThickLine(ring[i], ring[(i + 1) % kRing], 0.08f, false, colour);
            DebugRenderer::DrawPoint(hit, 0.15f, false, colour);
            DebugRenderer::RequestPass();
        }

        const bool held = input.GetMouseHeld(InputCode::MouseKey::ButtonLeft);
        if(input.GetMouseClicked(InputCode::MouseKey::ButtonLeft) && gotHit)
            sculpt->BeginStroke();
        if(held && gotHit)
            sculpt->ApplyBrushAt(hit, hitEntity, (float)Engine::GetTimeStep().GetSeconds());
        if(!held)
            sculpt->EndStroke();

        if(!typing)
        {
            if(input.GetKeyPressed(InputCode::Key::G))
                sculpt->SetCurrentTool(shift ? TerrainTool::Lower : TerrainTool::Raise);
            else if(input.GetKeyPressed(InputCode::Key::S) && !ctrl)
                sculpt->SetCurrentTool(TerrainTool::Smooth);
            else if(input.GetKeyPressed(InputCode::Key::F))
                sculpt->SetCurrentTool(TerrainTool::Flatten);
            else if(input.GetKeyPressed(InputCode::Key::N) && !ctrl)
                sculpt->SetCurrentTool(TerrainTool::Noise);
            if(input.GetKeyPressed(InputCode::Key::LeftBracket))
            {
                float& v = shift ? sculpt->BrushStrength() : sculpt->BrushRadius();
                v        = Maths::Max(0.05f, v * 0.85f);
            }
            if(input.GetKeyPressed(InputCode::Key::RightBracket))
            {
                float& v = shift ? sculpt->BrushStrength() : sculpt->BrushRadius();
                v *= 1.15f;
            }
            if(input.GetKeyPressed(InputCode::Key::Escape))
                sculpt->SetEnabled(false);
        }
        return true;
    }

    void EditorUI::DrawSculptCard(TerrainSculpt* sculpt, const Vec2& origin, const Vec2& size)
    {
        const UIToolTheme& theme = UIGetToolTheme();
        const f32 dpi            = GetUIState()->DPIScale;
        UI_State* ui             = GetUIState();
        UIBeginChild("##sculptcard", { SizeKind_ChildSum, 1.0f }, { SizeKind_ChildSum, 1.0f },
                     WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y | WidgetFlags_StackVertically);
        UI_Widget* card                            = ui->parents.Back();
        card->relative_position                    = Vec2(origin.x + 10.0f * dpi, origin.y + size.y - 150.0f * dpi) - ui->root_parent.position;
        card->style_vars[StyleVar_BackgroundColor] = theme.Popup;
        card->style_vars[StyleVar_CornerRadius]    = Vec4(theme.PanelRounding * dpi, 0.0f, 0.0f, 0.0f);
        card->style_vars[StyleVar_Padding]         = Vec4(8.0f * dpi, 6.0f * dpi, 0.0f, 0.0f);

        static const TerrainTool kTools[] = { TerrainTool::Raise, TerrainTool::Lower, TerrainTool::Smooth, TerrainTool::Flatten, TerrainTool::Noise };
        static const char* kKeys[]        = { "G", "Shift G", "S", "F", "N" };
        UIBeginRow();
        char id[64];
        for(int i = 0; i < 5; i++)
        {
            snprintf(id, sizeof(id), "%s###sculpt%d", TerrainSculpt::ToolName(kTools[i]), i);
            const bool on = sculpt->CurrentTool() == kTools[i];
            UIPushStyle(StyleVar_TextColor, on ? theme.Accent : theme.Text);
            if(UIButton(id).clicked)
                sculpt->SetCurrentTool(kTools[i]);
            UIPopStyle(StyleVar_TextColor);
            snprintf(id, sizeof(id), "%s (%s)", TerrainSculpt::ToolName(kTools[i]), kKeys[i]);
            UITooltip(id);
        }
        if(UIIconButton(ICON_MDI_CLOSE "###sculptclose", 22.0f).clicked)
            sculpt->SetEnabled(false);
        UITooltip("Stop sculpting (Esc)");
        UIEndRow();
        UIBeginPropertyGrid("##sculptgrid", 0.4f);
        UIPropertySlider("Radius", &sculpt->BrushRadius(), 0.5f, 200.0f, "%.1f");
        UIPropertySlider("Strength", &sculpt->BrushStrength(), 0.05f, 40.0f, "%.2f");
        const char* falloffs[] = { "Linear", "Smooth", "Sharp" };
        int falloff            = (int)sculpt->Falloff();
        if(UIPropertyDropdown("Falloff", &falloff, falloffs, 3))
            sculpt->Falloff() = (TerrainFalloff)falloff;
        UIEndPropertyGrid();
        UIEndChild();
    }

    void EditorUI::DrawGameView(const UIDockPanelContext& context)
    {
        EditorUI* self = (EditorUI*)context.UserData;
        GameViewPanel* view  = self->m_GameView;
        Application& app     = Application::Get();
        const f32 dpi        = context.DPIScale;
        if(!view || !CurrentScene())
            return;

        const UIToolTheme& theme = UIGetToolTheme();
        const f32 inset          = (theme.FloatingToolbars && !self->m_Touch) ? floorf(Maths::Min(theme.PanelGap, 6.0f) * dpi) : 0.0f;

        Vec2 size = Vec2(context.Size.x - inset * 2.0f, context.Size.y - inset);
        size.x    = Maths::Max(floorf(size.x), 2.0f);
        size.y    = Maths::Max(floorf(size.y), 2.0f);
        // a fixed shape: the biggest picture of that shape that fits, in the middle of what is left
        const f32 roomY = size.y;
        {
            const auto& settings = self->m_Editor->GetSettings();
            // a fixed resolution brings its own shape
            const f32 shape = settings.m_GameFixedRes ? (f32)settings.m_GameResW / (f32)Maths::Max(settings.m_GameResH, 1)
                                                      : settings.m_FixedAspect;
            if((settings.m_GameFixedRes || !settings.m_FreeAspect) && shape > 0.01f)
            {
                if(size.x / shape > size.y)
                    size.x = floorf(size.y * shape);
                else
                    size.y = floorf(size.x / shape);
                size.x = Maths::Max(size.x, 2.0f);
                size.y = Maths::Max(size.y, 2.0f);
            }
        }
        size.x -= ((int)size.x % 2 != 0) ? 1.0f : 0.0f;
        size.y -= ((int)size.y % 2 != 0) ? 1.0f : 0.0f;
        const f32 above = floorf((roomY - size.y) * 0.5f);

        view->SetExternalVisible(true);
        // (the Scene panel shapes the editor camera when it is up as well)
        const Vec2 target = self->m_GameTarget.Settle(size);
        {
            const auto& settings = self->m_Editor->GetSettings();
            f32 viewScale        = settings.m_GameViewScale;
            if(settings.m_GameFixedRes)
            {
                // render at exactly the chosen width: undo the quality scale, then size the panel's picture to it
                const f32 qualityScale = Maths::Max(app.GetQualitySettings().RendererScale, 0.05f);
                viewScale              = (f32)settings.m_GameResW / (Maths::Max(target.x, 2.0f) * qualityScale);
            }
            view->SetViewScale(Maths::Clamp(viewScale, 0.05f, 8.0f));
        }
        Graphics::Texture2D* texture = view->PrepareExternal((uint32_t)target.x, (uint32_t)target.y, !UIDockIsPanelVisible(self->m_Dock, "Scene"));

        UIBeginChild("##gameframe", { SizeKind_PercentOfParent, 1.0f }, { SizeKind_Pixels, context.Size.y },
                     WidgetFlags_StackVertically);
        GetUIState()->parents.Back()->style_vars[StyleVar_Padding] = Vec4(floorf((context.Size.x - size.x) * 0.5f), 0.0f, 0.0f, 0.0f);

        if(above >= 1.0f)
            UISpacer(above / dpi);
        const bool flip      = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();
        UI_Interaction image = UIImageSized("##gameimage", texture, size, flip);
        if(image.widget)
        {
            image.widget->style_vars[StyleVar_CornerRadius] = Vec4(self->m_Touch ? 0.0f : Maths::Max(0.0f, theme.PanelRounding - 2.0f) * dpi, 0.0f, 0.0f, 0.0f);
            // the game's own UI and input are measured from where its picture sits
            if(!image.widget->FirstFrame)
                app.m_SceneViewPosition = image.widget->position;
        }

        // stopped, and the scene has no camera until its script makes one: it is shown through the editor's,
        // so say whose view this is
        if(app.GetEditorState() == EditorState::Preview && view->HasNoCamera())
        {
            u64 hash;
            HandleUIString("##gamenocamera", &hash);
            UI_Widget* note = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_DrawText | WidgetFlags_Floating_X | WidgetFlags_Floating_Y,
                                         context.Size.x < 560.0f * dpi ? Str8Lit(ICON_MDI_CAMERA "  No scene camera yet: editor's view")
                                                                       : Str8Lit(ICON_MDI_CAMERA "  No camera in the scene yet: the editor's view. Play to see the game's."),
                                         hash, { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
            UIClearBorders(note);
            const Vec4 noteSafe     = self->ViewInsets(context.Position, context.Size);
            note->relative_position = Vec2(floorf((context.Size.x - note->size.x) * 0.5f), context.Size.y - noteSafe.w - note->size.y - 14.0f * dpi);
            note->style_vars[StyleVar_FontSize]     = Vec4(theme.FontSize * 0.92f * dpi, 0.0f, 0.0f, 1.0f);
            note->style_vars[StyleVar_Padding]      = Vec4(10.0f * dpi, 5.0f * dpi, 0.0f, 0.0f);
            note->style_vars[StyleVar_CornerRadius] = Vec4(11.0f * dpi, 0.0f, 0.0f, 0.0f);
            Vec4 noteGlass                          = theme.Popup;
            noteGlass.w                             = 0.88f;
            note->style_vars[StyleVar_BackgroundColor] = noteGlass;
            UISetTextColour(note, theme.TextDim);
        }

        const EditorState state = app.GetEditorState();
        UI_State* ui       = GetUIState();
        const bool onImage = image.widget && (ui->hot_widget == image.widget->hash || ui->active_widget == image.widget->hash);
        app.SetSceneActive(image.hovering && onImage && state == EditorState::Play);

        // ---- the game's own bar: run it, slow it, see inside it ----
        {
            const Vec2 frameSize = Vec2(context.Size.x, context.Size.y);
            const Vec4 safe      = self->ViewInsets(context.Position, context.Size);
            const f32 edge       = inset + 10.0f * dpi + safe.z;
            const f32 overlayTop = 10.0f * dpi + safe.y;
            if(self->m_GameBarHidden)
            {
                if(ViewBarHandle("##gameviewhandle", frameSize, edge, overlayTop))
                    self->m_GameBarHidden = false;
            }
            else
            {
                UI_Widget* pill         = BeginViewPill("##gameviewpill");
                pill->relative_position = Vec2(floorf((frameSize.x - pill->size.x) * 0.5f), overlayTop);

                const bool playing = state != EditorState::Preview;
                if(ViewPillButton(playing ? ICON_MDI_STOP "  Stop###gameplay" : ICON_MDI_PLAY "  Play###gameplay", playing))
                    self->m_Editor->TogglePlay();
                if(ViewPillButton(ICON_MDI_PAUSE "###gamepause", state == EditorState::Paused))
                    self->m_Editor->TogglePause();
                UITooltip("Pause");

                // the shape of the picture: whatever the panel is, or a screen's
                {
                    struct Shape
                    {
                        const char* Name;
                        f32 Aspect; // 0 = free
                    };
                    static const Shape kShapes[] = {
                        { "Free", 0.0f }, { "16:9", 16.0f / 9.0f }, { "16:10", 16.0f / 10.0f }, { "4:3", 4.0f / 3.0f }, { "3:2", 3.0f / 2.0f },
                        { "21:9", 21.0f / 9.0f }, { "1:1", 1.0f }, { "9:16", 9.0f / 16.0f }, { "Phone", 19.5f / 9.0f }, { "Phone upright", 9.0f / 19.5f },
                    };
                    auto& settings      = self->m_Editor->GetSettings();
                    const char* current = "Free";
                    if(!settings.m_FreeAspect)
                    {
                        current = "Custom";
                        for(const Shape& shape : kShapes)
                            if(shape.Aspect > 0.0f && Maths::Abs(shape.Aspect - settings.m_FixedAspect) < 0.005f)
                                current = shape.Name;
                    }
                    char shapeLabel[48];
                    snprintf(shapeLabel, sizeof(shapeLabel), ICON_MDI_MONITOR "  %s###gameaspect", current);
                    if(ViewPillButton(shapeLabel, !settings.m_FreeAspect))
                        UIOpenPopup("##gameaspectmenu");
                    UITooltip("Aspect ratio");
                    if(UIBeginPopup("##gameaspectmenu"))
                    {
                        for(const Shape& shape : kShapes)
                        {
                            bool on = shape.Aspect == 0.0f ? settings.m_FreeAspect : (!settings.m_FreeAspect && Maths::Abs(shape.Aspect - settings.m_FixedAspect) < 0.005f);
                            char item[48];
                            snprintf(item, sizeof(item), "%s###gameaspect%s", shape.Name, shape.Name);
                            if(UIMenuItem(item, nullptr, &on))
                            {
                                settings.m_FreeAspect = shape.Aspect == 0.0f;
                                if(shape.Aspect > 0.0f)
                                    settings.m_FixedAspect = shape.Aspect;
                            }
                        }
                        UIEndPopup();
                    }
                }

                // how fast the game's clock runs
                TimeStep& time  = Engine::GetTimeStep();
                f32 scale       = (f32)time.GetTimeScale();
                char scaleLabel[48];
                snprintf(scaleLabel, sizeof(scaleLabel), ICON_MDI_TIMER "  %.2gx###gamespeed", scale);
                if(ViewPillButton(scaleLabel, Maths::Abs(scale - 1.0f) > 0.001f))
                    UIOpenPopup("##gamespeedmenu");
                UITooltip("Game speed");
                if(UIBeginPopup("##gamespeedmenu"))
                {
                    if(UIToolSlider("###gamespeedslider", &scale, 0.0f, 2.0f, 170.0f))
                        time.SetTimeScale(scale);
                    UIMenuSeparator();
                    static const f32 kSpeeds[] = { 0.1f, 0.25f, 0.5f, 1.0f, 2.0f };
                    for(f32 preset : kSpeeds)
                    {
                        char item[32];
                        snprintf(item, sizeof(item), "%gx###gamespeed%g", preset, preset);
                        if(UIMenuItem(item))
                            time.SetTimeScale(preset);
                    }
                    UIEndPopup();
                }

                if(ViewPillButton(ICON_MDI_BUG "###gamedebug"))
                    UIOpenPopup("##gameviewdebug");
                UITooltip("Debug drawing");
                if(UIBeginPopup("##gameviewdebug"))
                {
                    DrawViewDebugMenu(self->m_Editor, false);
                    UIEndPopup();
                }

                // the game over the whole window, and back (Layout > Maximise on Play does it by itself)
                if(!self->m_Touch)
                {
                    const bool maximised = self->IsMaximised("Game");
                    if(ViewPillButton(maximised ? ICON_MDI_FULLSCREEN_EXIT "###gamemax" : ICON_MDI_FULLSCREEN "###gamemax", maximised))
                    {
                        self->m_MaximisedForPlay = false;
                        self->ToggleMaximised("Game");
                    }
                    UITooltip(maximised ? "Restore" : "Maximise");
                }
                if(ViewPillButton(ICON_MDI_CHEVRON_UP "###gamehide"))
                    self->m_GameBarHidden = true;
                UITooltip("Hide the toolbar");
                UIEndChild();
            }
        }

        // says what state the game is in when that changes, then gets out of the way; stays while paused
        if((i32)state != self->m_PlayChipState)
        {
            self->m_PlayChipState = (i32)state;
            self->m_PlayChipTime  = 2.5f;
        }
        self->m_PlayChipTime     = Maths::Max(0.0f, self->m_PlayChipTime - (f32)Engine::GetTimeStep().GetSeconds());
        const f32 chipFade       = state == EditorState::Paused ? 1.0f : Maths::Clamp(self->m_PlayChipTime / 0.4f, 0.0f, 1.0f);
        if(state != EditorState::Preview && chipFade > 0.0f)
        {
            u64 hash;
            HandleUIString("##gamechip", &hash);
            UI_Widget* chip = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_DrawText | WidgetFlags_Floating_X | WidgetFlags_Floating_Y,
                                         state == EditorState::Paused ? Str8Lit(ICON_MDI_PAUSE "  Paused") : Str8Lit(ICON_MDI_PLAY "  Playing"),
                                         hash, { SizeKind_TextContent, 1.0f }, { SizeKind_TextContent, 1.0f });
            UIClearBorders(chip);
            chip->relative_position                    = Vec2(inset + 10.0f * dpi, 10.0f * dpi);
            chip->style_vars[StyleVar_Padding]         = Vec4(9.0f * dpi, 4.0f * dpi, 0.0f, 0.0f);
            chip->style_vars[StyleVar_CornerRadius]    = Vec4(11.0f * dpi, 0.0f, 0.0f, 0.0f);
            chip->style_vars[StyleVar_BackgroundColor] = Vec4(theme.Accent.x, theme.Accent.y, theme.Accent.z, theme.Accent.w * chipFade);
            UISetTextColour(chip, Vec4(theme.AccentText.x, theme.AccentText.y, theme.AccentText.z, chipFade));
        }

        UIEndChild();
    }

    void EditorUI::DrawEditorSettings(const UIDockPanelContext& context)
    {
        EditorUI* self = (EditorUI*)context.UserData;
        Editor* editor       = self->m_Editor;
        auto& settings       = editor->GetSettings();

        if(UISectionHeader("Appearance###editorappearance", true))
        {
            UIBeginPropertyGrid("##editorappearancegrid", 0.6f);
            static const char* s_Themes[40];
            const i32 themeCount = Maths::Min(UIGetToolThemePresetCount(), 40);
            for(i32 i = 0; i < themeCount; i++)
                s_Themes[i] = UIGetToolThemePresetName(i);
            int theme = UIGetToolThemePreset();
            if(UIPropertyDropdown("Theme", &theme, s_Themes, themeCount) && theme >= 0)
                UISetToolThemePreset(theme);
            UIBeginPropertyRow("");
            if(UIButton("Edit Theme...###editoredittheme").clicked)
                UIDockFocusPanel(self->m_Dock, "Theme");
            UIEndPropertyRow();
            bool minimal = self->m_Minimal;
            if(UIPropertyBool("Minimal (plain black and white)", &minimal))
                self->SetMinimal(minimal);
            UIEndPropertyGrid();
        }

        UISpacer(4.0f);
        if(UISectionHeader("Layout###editorlayout", true))
        {
            UIBeginPropertyGrid("##editorlayoutgrid", 0.6f);
            int workspace = self->m_Workspace;
            if(UIPropertyDropdown("Workspace", &workspace, (const char**)kWorkspaceNames, Workspace_Count) && workspace != self->m_Workspace)
                self->SetWorkspace(workspace);
            bool focus = self->m_Focus;
            if(UIPropertyBool("Focus Mode", &focus))
                self->SetFocusMode(focus);
            UIPropertyBool("Maximise Game on Play", &editor->FullScreenOnPlay());
            UIPropertyBool("Hide Scene Bar", &self->m_SceneBarHidden);
            UIPropertyBool("Hide Game Bar", &self->m_GameBarHidden);
            UIBeginPropertyRow("");
            if(UIButton("Reset Layout###editorresetlayout").clicked)
                UIDockResetLayout(self->m_Dock);
            UIEndPropertyRow();
            UIEndPropertyGrid();
        }

        UISpacer(4.0f);
        if(UISectionHeader("Workspace###editorworkspace", true))
        {
            UIBeginPropertyGrid("##editorworkspacegrid", 0.6f);
            bool touch = self->m_Touch;
            if(UIPropertyBool("Touch Layout", &touch))
                self->SetTouch(touch);
            UIPropertyBool("Inspector Follows Selection", &settings.m_InspectorFollows);
            UIPropertyBool("Play Workspace While Playing", &self->m_AutoPlayWorkspace);
            UIPropertyBool("Sleep When Unfocused", &settings.m_SleepOutofFocus);
            UIEndPropertyGrid();
        }

        UISpacer(4.0f);
        if(UISectionHeader("Viewport###editorviewport", true))
        {
            UIBeginPropertyGrid("##editorviewportgrid", 0.6f);
            UIPropertyBool("Grid", &settings.m_ShowGrid);
            UIPropertyFloat("Grid Size", &settings.m_GridSize, 0.1f, 0.1f, 1000.0f, "%.1f");
            UIPropertyBool("Gizmos", &settings.m_ShowGizmos);
            UIPropertyBool("Snap", &settings.m_SnapQuizmo);
            UIPropertyFloat("Snap Amount", &settings.m_SnapAmount, 0.01f, 0.001f, 1000.0f, "%.2f");
            UIPropertyBool("Local Axes", &self->m_GizmoLocal);
            UIPropertySlider("Resolution Scale", &settings.m_SceneViewScale, 0.25f, 2.0f, "%.2fx");
            if(self->m_SceneView)
            {
                char readout[48];
                snprintf(readout, sizeof(readout), "%u x %u", self->m_SceneView->GetRenderWidth(), self->m_SceneView->GetRenderHeight());
                UIPropertyLabel("Renders At", Str8C(readout));
            }
            UIBeginPropertyRow("");
            if(UIButton("Reset Resolution###sceneresreset").clicked)
                settings.m_SceneViewScale = 1.0f;
            UIEndPropertyRow();
            UIEndPropertyGrid();
        }

        UISpacer(4.0f);
        if(UISectionHeader("Game View###editorgameview", true))
        {
            UIBeginPropertyGrid("##editorgameviewgrid", 0.6f);

            struct Shape { const char* Name; f32 Aspect; };
            static const Shape kShapes[] = {
                { "Free", 0.0f }, { "16:9", 16.0f / 9.0f }, { "16:10", 16.0f / 10.0f }, { "4:3", 4.0f / 3.0f }, { "3:2", 1.5f },
                { "21:9", 21.0f / 9.0f }, { "1:1", 1.0f }, { "9:16", 9.0f / 16.0f }, { "Phone", 19.5f / 9.0f }, { "Phone upright", 9.0f / 19.5f },
            };
            static const char* kShapeNames[10];
            int shapeIndex = 0;
            for(int i = 0; i < (int)(sizeof(kShapes) / sizeof(kShapes[0])); i++)
            {
                kShapeNames[i] = kShapes[i].Name;
                if(!settings.m_FreeAspect && kShapes[i].Aspect > 0.0f && Maths::Abs(kShapes[i].Aspect - settings.m_FixedAspect) < 0.005f)
                    shapeIndex = i;
            }
            const bool fixedRes = settings.m_GameFixedRes;
            if(!fixedRes && UIPropertyDropdown("Aspect Ratio", &shapeIndex, kShapeNames, (int)(sizeof(kShapes) / sizeof(kShapes[0]))))
            {
                settings.m_FreeAspect = kShapes[shapeIndex].Aspect == 0.0f;
                if(!settings.m_FreeAspect)
                    settings.m_FixedAspect = kShapes[shapeIndex].Aspect;
            }

            UIPropertyBool("Fixed Resolution", &settings.m_GameFixedRes);
            if(settings.m_GameFixedRes)
            {
                struct Res { const char* Name; int W, H; };
                static const Res kRes[] = {
                    { "Custom", 0, 0 }, { "640 x 360", 640, 360 }, { "1280 x 720", 1280, 720 }, { "1920 x 1080", 1920, 1080 },
                    { "2560 x 1440", 2560, 1440 }, { "3840 x 2160", 3840, 2160 }, { "1080 x 1920", 1080, 1920 }, { "1170 x 2532", 1170, 2532 },
                };
                static const char* kResNames[sizeof(kRes) / sizeof(kRes[0])];
                int resIndex = 0;
                for(int i = 0; i < (int)(sizeof(kRes) / sizeof(kRes[0])); i++)
                {
                    kResNames[i] = kRes[i].Name;
                    if(kRes[i].W == settings.m_GameResW && kRes[i].H == settings.m_GameResH)
                        resIndex = i;
                }
                if(UIPropertyDropdown("Preset", &resIndex, kResNames, (int)(sizeof(kRes) / sizeof(kRes[0]))) && resIndex > 0)
                {
                    settings.m_GameResW = kRes[resIndex].W;
                    settings.m_GameResH = kRes[resIndex].H;
                }
                UIPropertyInt("Width", &settings.m_GameResW, 1.0f, 64, 8192);
                UIPropertyInt("Height", &settings.m_GameResH, 1.0f, 64, 8192);
            }
            else
                UIPropertySlider("Resolution Scale", &settings.m_GameViewScale, 0.25f, 2.0f, "%.2fx");

            if(self->m_GameView)
            {
                char readout[48];
                snprintf(readout, sizeof(readout), "%u x %u", self->m_GameView->GetRenderWidth(), self->m_GameView->GetRenderHeight());
                UIPropertyLabel("Renders At", Str8C(readout));
            }
            UIBeginPropertyRow("");
            if(UIButton("Reset Game View###gameresreset").clicked)
            {
                settings.m_GameFixedRes   = false;
                settings.m_GameViewScale  = 1.0f;
                settings.m_FreeAspect     = true;
            }
            UIEndPropertyRow();
            UIEndPropertyGrid();
        }

        UISpacer(4.0f);
        if(UISectionHeader("Camera###editorcamera", true))
        {
            UIBeginPropertyGrid("##editorcameragrid", 0.6f);
            if(UIPropertyFloat("Speed", &settings.m_CameraSpeed, 1.0f, 1.0f, 100000.0f, "%.0f"))
                editor->GetEditorCameraController().SetSpeed(settings.m_CameraSpeed);
            if(Camera* camera = editor->GetCamera())
            {
                if(UIPropertyFloat("Near", &settings.m_CameraNear, 0.001f, 0.001f, 100.0f, "%.3f"))
                    camera->SetNear(settings.m_CameraNear);
                if(UIPropertyFloat("Far", &settings.m_CameraFar, 1.0f, 1.0f, 1000000.0f, "%.0f"))
                    camera->SetFar(settings.m_CameraFar);
                float fov = camera->GetFOV();
                if(UIPropertyFloat("Field of View", &fov, 0.2f, 1.0f, 179.0f, "%.1f"))
                    camera->SetFOV(fov);
            }
            UIEndPropertyGrid();
        }
    }

    // ---- script panel --------------------------------------------------------

    void EditorUI::OpenScript(const char* physicalPath)
    {
        if(!physicalPath || !physicalPath[0])
            return;

        // already open: bring it forward
        for(i32 i = 0; i < m_ScriptCount; i++)
        {
            if(strcmp(m_Scripts[i].Path, physicalPath) == 0)
            {
                m_ScriptCurrent = i;
                UICodeEditorFocus(m_Scripts[i].Code);
                UIDockFocusPanel(m_Dock, "Script");
                return;
            }
        }
        if(m_ScriptCount >= kMaxScripts)
        {
            LWARN("Script panel is full (%d files): close one first", kMaxScripts);
            UIDockFocusPanel(m_Dock, "Script");
            return;
        }

        ScriptTab& tab = m_Scripts[m_ScriptCount];
        tab.Code       = UICodeEditorCreate();
        snprintf(tab.Path, sizeof(tab.Path), "%s", physicalPath);

        ArenaTemp scratch   = ScratchBegin(0, 0);
        const String8 text  = FileSystem::ReadTextFile(scratch.arena, Str8C((char*)physicalPath));
        const char* extension = strrchr(physicalPath, '.');
        auto isOneOf = [extension](std::initializer_list<const char*> list)
        {
            for(const char* item : list)
                if(extension && strcmp(extension, item) == 0)
                    return true;
            return false;
        };
        UICodeLanguage language = UICodeLanguage_Plain;
        if(isOneOf({ ".lua", ".Lua" }))
            language = UICodeLanguage_Lua;
        else if(isOneOf({ ".json", ".ini", ".cfg", ".conf", ".toml", ".yaml", ".yml", ".lmproj", ".meta", ".lmat", ".lprefab", ".plist", ".xcprivacy" }))
            language = UICodeLanguage_Config;
        else if(isOneOf({ ".glsl", ".glslh", ".vert", ".frag", ".comp", ".shader" }))
            language = UICodeLanguage_Shader;
        UICodeEditorSetLanguage(tab.Code, language);
        UICodeEditorSetText(tab.Code, text);
        UICodeEditorFocus(tab.Code);
        ScratchEnd(scratch);

        m_ScriptCurrent = m_ScriptCount++;
        UIDockFocusPanel(m_Dock, "Script");
    }

    bool EditorUI::SaveScript(i32 index)
    {
        if(index < 0)
            index = m_ScriptCurrent;
        if(index < 0 || index >= m_ScriptCount)
            return false;
        ScriptTab& tab = m_Scripts[index];
        if(!FileSystem::WriteTextFile(Str8C(tab.Path), UICodeEditorGetText(tab.Code)))
        {
            LERROR("Could not save %s", tab.Path);
            return false;
        }
        // the engine reloads a script when its file changes
        UICodeEditorMarkSaved(tab.Code);
        m_CodeSavedFlash = 1.5f;
        return true;
    }

    void EditorUI::SaveAllScripts()
    {
        for(i32 i = 0; i < m_ScriptCount; i++)
            if(UICodeEditorIsDirty(m_Scripts[i].Code))
                SaveScript(i);
    }

    bool EditorUI::AnyScriptUnsaved() const
    {
        for(i32 i = 0; i < m_ScriptCount; i++)
            if(UICodeEditorIsDirty(m_Scripts[i].Code))
                return true;
        return false;
    }

    void EditorUI::CloseScript(i32 index)
    {
        if(index < 0 || index >= m_ScriptCount)
            return;
        UICodeEditorDestroy(m_Scripts[index].Code);
        for(i32 i = index; i + 1 < m_ScriptCount; i++)
            m_Scripts[i] = m_Scripts[i + 1];
        m_ScriptCount--;
        m_Scripts[m_ScriptCount] = ScriptTab();
        if(m_ScriptCurrent > index || m_ScriptCurrent >= m_ScriptCount)
            m_ScriptCurrent = Maths::Max(0, m_ScriptCurrent - 1);
        if(m_ScriptCount > 0)
            UICodeEditorFocus(m_Scripts[m_ScriptCurrent].Code);
    }

    void EditorUI::DrawScript(const UIDockPanelContext& context)
    {
        EditorUI* self     = (EditorUI*)context.UserData;
        const UIToolTheme& theme = UIGetToolTheme();
        const f32 dpi            = context.DPIScale;

        if(self->m_ScriptCount == 0)
        {
            UIPushStyle(StyleVar_TextColor, theme.TextDim);
            UILabelCStr("##scriptnone", "   Open a script from Assets (double-click), or search for one with # in the command bar.");
            UIPopStyle(StyleVar_TextColor);
            return;
        }
        self->m_ScriptCurrent = Maths::Clamp(self->m_ScriptCurrent, 0, self->m_ScriptCount - 1);

        const f32 toolbar      = floorf(32.0f * dpi);
        self->m_CodeSavedFlash = Maths::Max(0.0f, self->m_CodeSavedFlash - (f32)Engine::GetTimeStep().GetSeconds());
        i32 closeRequest       = -1;

        UIBeginChild("##scriptbar", { SizeKind_PercentOfParent, 1.0f }, { SizeKind_Pixels, toolbar }, WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY, true);
        {
            UI_Widget* bar                        = GetUIState()->parents.Back();
            bar->style_vars[StyleVar_Padding]     = Vec4(6.0f * dpi, 0.0f, 0.0f, 0.0f);
            bar->style_vars[StyleVar_ItemSpacing] = Vec4(2.0f * dpi, 0.0f, 0.0f, 0.0f);

            // ---- one tab per open file ----
            for(i32 i = 0; i < self->m_ScriptCount; i++)
            {
                ScriptTab& tab     = self->m_Scripts[i];
                const bool current = i == self->m_ScriptCurrent;
                const bool dirty   = UICodeEditorIsDirty(tab.Code);
                const char* name   = strrchr(tab.Path, '/');
                name               = name ? name + 1 : tab.Path;

                char id[64];
                snprintf(id, sizeof(id), "###scripttab%d", i);
                UI_Interaction pill = UIBeginChild(id, { SizeKind_ChildSum, 1.0f }, { SizeKind_Pixels, floorf(24.0f * dpi) },
                                                   WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY | WidgetFlags_DrawBackground | WidgetFlags_Clickable);
                UI_Widget* widget = GetUIState()->parents.Back();
                UIClearBorders(widget);
                widget->style_vars[StyleVar_Padding]      = Vec4(8.0f * dpi, 0.0f, 0.0f, 0.0f);
                widget->style_vars[StyleVar_ItemSpacing]  = Vec4(4.0f * dpi, 0.0f, 0.0f, 0.0f);
                widget->style_vars[StyleVar_CornerRadius] = Vec4(theme.WidgetRounding * dpi + 2.0f * dpi, 0.0f, 0.0f, 0.0f);
                UISetBackground(widget, current ? theme.Selection : Vec4(0.0f), current ? theme.Selection : theme.Hover, theme.Selection);

                UIPushStyle(StyleVar_TextColor, dirty ? theme.Warning : (current ? theme.Text : theme.TextDim));
                UILabelCStr("###name", name);
                UIPopStyle(StyleVar_TextColor);
                // unsaved shows as a dot where the close cross is, until the pointer is over the tab
                UIPushStyle(StyleVar_TextColor, dirty && !pill.hovering ? theme.Warning : theme.TextDim);
                const bool close = UIIconButton(dirty && !pill.hovering ? ICON_MDI_RECORD "###close" : ICON_MDI_CLOSE "###close", 18.0f).clicked;
                UIPopStyle(StyleVar_TextColor);
                UIEndChild();

                if(close)
                    closeRequest = i;
                else if(pill.clicked && !current)
                {
                    self->m_ScriptCurrent = i;
                    UICodeEditorFocus(tab.Code);
                }
            }

            UIFlexSpacer();

            if(self->m_CodeSavedFlash > 0.0f)
            {
                UIPushStyle(StyleVar_TextColor, theme.Success);
                UILabelCStr("##scriptsaved", "saved  ");
                UIPopStyle(StyleVar_TextColor);
            }

            u32 line = 1, column = 1;
            UICodeEditorGetCursor(self->m_Scripts[self->m_ScriptCurrent].Code, &line, &column);
            char where[48];
            snprintf(where, sizeof(where), "Ln %u, Col %u  ", line, column);
            UIPushStyle(StyleVar_TextColor, theme.TextDim);
            UILabelCStr("##scriptwhere", where);
            UIPopStyle(StyleVar_TextColor);

            // touch: the keyboard on demand (a tap in the text brings it up too)
            if(self->m_Touch)
            {
                const bool up = UIKeyboardCover() > 0.01f;
                if(UIIconButton(ICON_MDI_KEYBOARD "###scriptkeyboard", 24.0f, up).clicked)
                {
                    if(!up)
                    {
                        UICodeEditorFocus(self->m_Scripts[self->m_ScriptCurrent].Code);
                        UISetKeyboardStyle(UIKeyboardStyle_Code);
                    }
                    UIShowKeyboard(!up);
                }
                UITooltip("Keyboard");
            }

            if(UIIconButton(ICON_MDI_CONTENT_SAVE "###scriptsave", 24.0f).clicked)
                self->SaveScript();
            UITooltip("Save (Ctrl S)");
        }
        UIEndChild();

        // closing a file with changes asks first
        if(closeRequest >= 0)
        {
            if(UICodeEditorIsDirty(self->m_Scripts[closeRequest].Code))
            {
                self->m_ScriptClosing = closeRequest;
                UIOpenPopupAt("##scriptclose", UIMousePosition());
            }
            else
                self->CloseScript(closeRequest);
        }
        if(UIBeginPopup("##scriptclose"))
        {
            const i32 closing = self->m_ScriptClosing;
            if(closing >= 0 && closing < self->m_ScriptCount)
            {
                const char* name = strrchr(self->m_Scripts[closing].Path, '/');
                char title[160];
                snprintf(title, sizeof(title), "%s has unsaved changes", name ? name + 1 : self->m_Scripts[closing].Path);
                UIMenuItem(title, nullptr, nullptr, false);
                UIMenuSeparator();
                if(UIMenuItem(ICON_MDI_CONTENT_SAVE "  Save and Close"))
                {
                    if(self->SaveScript(closing))
                        self->CloseScript(closing);
                    self->m_ScriptClosing = -1;
                }
                if(UIMenuItem(ICON_MDI_DELETE_OUTLINE "  Discard Changes"))
                {
                    self->CloseScript(closing);
                    self->m_ScriptClosing = -1;
                }
                if(UIMenuItem("Cancel"))
                    self->m_ScriptClosing = -1;
            }
            UIEndPopup();
        }

        if(self->m_ScriptCount == 0)
            return;
        self->m_ScriptCurrent = Maths::Clamp(self->m_ScriptCurrent, 0, self->m_ScriptCount - 1);
        ScriptTab& tab        = self->m_Scripts[self->m_ScriptCurrent];

        // an id per file: each keeps its own widgets (and so its own focus)
        char id[32];
        snprintf(id, sizeof(id), "##scriptcode%p", (void*)tab.Code);
        UICodeEditorDraw(id, tab.Code, Vec2(context.Size.x, Maths::Max(context.Size.y - toolbar, 40.0f * dpi)));
        if(UICodeEditorTakeSaveRequest(tab.Code))
            self->SaveScript();
    }

    // ---- project hub -----------------------------------------------------------

    void EditorUI::ShowProjectHub(bool newProject)
    {
        m_HubOpen      = true;
        m_HubNew       = newProject;
        m_HubFilter[0] = 0;
        m_HubScroll    = 0.0f;
        if(!m_NewLocation[0])
            snprintf(m_NewLocation, sizeof(m_NewLocation), "%s", m_Editor->DefaultProjectLocation().c_str());
    }

    void EditorUI::OnScreenUINoProject()
    {
        if(!m_Enabled || m_Rect.z < 16.0f || m_Rect.w < 16.0f)
            return;
        {
            UI_State* ui = GetUIState();
            static const char* s_Rect = getenv("LUMOS_EDITOR_RECT");
            float w = 0.0f, h = 0.0f;
            if(s_Rect && sscanf(s_Rect, "%f,%f", &w, &h) == 2)
                m_Rect = Vec4(0.0f, 0.0f, Maths::Min(w * ui->DPIScale, m_Rect.z), Maths::Min(h * ui->DPIScale, m_Rect.w));
            if(m_Touch)
                ui->DPIScale *= 1.22f;
            ui->FullBleed           = true;
            const SafeAreaInsets sa = UIRawSafeAreaInsets();
            m_Safe                  = Vec4(floorf(sa.left * 0.9f), floorf(sa.top * 0.9f), floorf(sa.right * 0.9f), floorf(sa.bottom * 0.5f));
        }
        UIApplyToolThemeToContext();
        if(!m_NewLocation[0])
            snprintf(m_NewLocation, sizeof(m_NewLocation), "%s", m_Editor->DefaultProjectLocation().c_str());
        DrawProjectHub(true);
#ifdef LUMOS_EDITOR_WINDOW_BUTTONS
        {
            // no strip here: the window's buttons on their own, and the top of the window moves it
            UI_State* ui  = GetUIState();
            const f32 dpi = ui->DPIScale;
            const f32 bar = floorf(40.0f * dpi);
            UIPushStyle(StyleVar_FontSize, UIGetToolTheme().FontSize * dpi);
            UIBeginChild("##hubwindowbar", { SizeKind_Pixels, m_Rect.z }, { SizeKind_Pixels, bar },
                         WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY | WidgetFlags_Clickable | WidgetFlags_Floating_X | WidgetFlags_Floating_Y);
            UI_Widget* strip         = ui->parents.Back();
            strip->relative_position = Vec2(m_Rect.x, m_Rect.y) - ui->root_parent.position;
            strip->style_vars[StyleVar_Padding] = Vec4(6.0f * dpi, 0.0f, 0.0f, 0.0f);
            UIClearBorders(strip);
            UISetBackground(strip, Vec4(0.0f), Vec4(0.0f), Vec4(0.0f));
            UIFlexSpacer();
            DrawWindowButtons();
            UIEndChild();
            UIPopStyle(StyleVar_FontSize);
            HandleWindowDrag(ui->hot_widget == strip->hash);
        }
#endif
        UIDrawFileDialog();
    }

    void EditorUI::DrawProjectHub(bool standalone)
    {
        UI_State* ui             = GetUIState();
        const UIToolTheme& theme = UIGetToolTheme();
        const f32 dpi            = ui->DPIScale;
        const Vec2 view          = Vec2(m_Rect.z, m_Rect.w);
        Editor* editor           = m_Editor;
        // a touch screen gets it as a page, not a card in the middle; narrow = one column
        const bool page   = m_Touch;
        const bool narrow = view.x < 620.0f * dpi;

        const bool dialogUp = UIFileDialogIsOpen();
        bool close          = false;

        if(standalone)
        {
            // nothing behind it: fill the window
            UI_Widget* fill = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_Floating_X | WidgetFlags_Floating_Y, Str8Lit(""), 0x48554246494C4Cull,
                                         { SizeKind_Pixels, view.x }, { SizeKind_Pixels, view.y });
            fill->relative_position = Vec2(m_Rect.x, m_Rect.y) - ui->root_parent.position;
            UIClearBorders(fill);
            fill->style_vars[StyleVar_CornerRadius] = Vec4(0.0f);
            const Vec4 back = page ? theme.Panel : theme.Canvas;
            UISetBackground(fill, back, back, back);
        }
        else
        {
            if(page)
                FillRect("##hubpagefill", m_Rect, theme.Panel);
            else if(UIDimBackdrop("##hubdim", theme.Dark ? 0.5f : 0.3f).clicked && !dialogUp)
                close = true;
            if(!dialogUp && ui->FocusedTextInput == 0 && Input::Get().GetKeyPressedRaw(InputCode::Key::Escape))
                close = true;
        }

        // a page fills what the notch and corners leave; the fill behind it runs to the edges
        const f32 width   = page ? view.x - m_Safe.x - m_Safe.z : floorf(Maths::Min(920.0f * dpi, view.x * 0.94f));
        const f32 height  = page ? view.y - m_Safe.y - m_Safe.w : floorf(Maths::Min(580.0f * dpi, view.y * 0.86f));
        const f32 pad     = floorf((page ? 14.0f : 22.0f) * dpi);
        const f32 sideW   = narrow ? width - pad * 2.0f : floorf(Maths::Min(270.0f * dpi, width * 0.36f));
        const f32 listW   = narrow ? width - pad * 2.0f : width - sideW - pad * 3.0f;
        // what the one-column head takes: logo row, the three ways in, the gaps
        const f32 headH   = narrow ? floorf((40.0f + 10.0f + 36.0f) * dpi) + pad : 0.0f;

        UIPushStyle(StyleVar_FontSize, theme.FontSize * dpi);
        UIPushStyle(StyleVar_BackgroundColor, (standalone || page) ? theme.Panel : theme.Popup);
        UIPushStyle(StyleVar_Padding, Vec2(pad, pad));
        UIPushStyle(StyleVar_ItemSpacing, narrow ? Vec2(0.0f, pad) : Vec2(pad, 0.0f));
        UIPushStyle(StyleVar_CornerRadius, page ? 0.0f : 16.0f * dpi);
        UIPushStyle(StyleVar_TextColor, theme.Text);
        UIBeginOverlay("##projecthub", SizeKind_Pixels, width, SizeKind_Pixels, height,
                       (narrow ? WidgetFlags_StackVertically : WidgetFlags_StackHorizontally) | (page ? 0 : (WidgetFlags_AnimateAppear | WidgetFlags_DrawBorder))
                           | WidgetFlags_Floating_X | WidgetFlags_Floating_Y);
        {
            UI_Widget* panel         = ui->parents.Back();
            const Vec2 at            = page ? Vec2(m_Rect.x + m_Safe.x, m_Rect.y + m_Safe.y)
                                            : Vec2(m_Rect.x + floorf((view.x - width) * 0.5f), m_Rect.y + floorf((view.y - height) * 0.5f));
            panel->relative_position = at - ui->root_parent.position;
            const f32 line                             = page ? 0.0f : Maths::Max(1.0f, dpi);
            panel->style_vars[StyleVar_Border]         = Vec4(line, line, 0.0f, 0.0f);
            panel->style_vars[StyleVar_BorderColor]    = page ? Vec4(0.0f) : theme.Border;
            panel->style_vars[StyleVar_HotBorderColor] = page ? Vec4(0.0f) : theme.Border;
            if(page)
                panel->style_vars[StyleVar_ShadowColor] = Vec4(0.0f);
        }
        UIPopStyle(StyleVar_TextColor);
        UIPopStyle(StyleVar_CornerRadius);
        UIPopStyle(StyleVar_ItemSpacing);
        UIPopStyle(StyleVar_Padding);
        UIPopStyle(StyleVar_BackgroundColor);

        // a wide button in the side column
        auto sideButton = [&](const char* label, bool primary, bool current, f32 fixedWidth = 0.0f)
        {
            u64 hash;
            String8 text      = HandleUIString(label, &hash);
            UI_Widget* button = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_DrawText | WidgetFlags_Clickable, text, hash,
                                           fixedWidth > 0.0f ? UI_Size { SizeKind_Pixels, fixedWidth } : UI_Size { SizeKind_PercentOfParent, 1.0f },
                                           { SizeKind_Pixels, floorf(36.0f * dpi) });
            UIClearBorders(button);
            button->TextAlignment                     = fixedWidth > 0.0f ? UI_Text_Alignment_Center_X | UI_Text_Alignment_Center_Y : UI_Text_Alignment_Center_Y;
            button->style_vars[StyleVar_Padding]      = Vec4(14.0f * dpi, 0.0f, 0.0f, 0.0f);
            button->style_vars[StyleVar_CornerRadius] = Vec4(9.0f * dpi, 0.0f, 0.0f, 0.0f);
            const Vec4 fill = primary ? theme.Accent : (current ? theme.Selection : theme.Field);
            UISetBackground(button, fill, primary ? theme.Accent : theme.Selection, theme.Selection);
            UISetTextColour(button, primary ? theme.AccentText : theme.Text);
            return HandleWidgetInteraction(button).clicked;
        };

        char openPath[1024] = { 0 };
        i32 removeAt        = -1;
        bool create         = false;

        // ---- one column: a head across the top (what this is, the ways in) ----
        if(narrow)
        {
            UIBeginChild("##hubhead", { SizeKind_Pixels, sideW }, { SizeKind_Pixels, floorf(40.0f * dpi) }, WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY);
            {
                UI_Widget* head                        = ui->parents.Back();
                head->style_vars[StyleVar_Padding]     = Vec4(0.0f);
                head->style_vars[StyleVar_ItemSpacing] = Vec4(10.0f * dpi, 0.0f, 0.0f, 0.0f);
                if(Graphics::Texture2D* logo = editor->GetLogoTexture())
                    UIImageSized("##hublogo", logo, Vec2(34.0f * dpi, 34.0f * dpi));
                UIPushStyle(StyleVar_FontSize, theme.FontSize * 1.5f * dpi);
                UILabelCStr("##hubtitle", "Lumos");
                UIPopStyle(StyleVar_FontSize);
                char version[48];
                snprintf(version, sizeof(version), "%d.%d.%d", LumosVersion.major, LumosVersion.minor, LumosVersion.patch);
                UIPushStyle(StyleVar_TextColor, theme.TextDim);
                UILabelCStr("##hubversion", version);
                UIFlexSpacer();
                // back to the project that is open (nothing to go back to otherwise, and a phone app is not quit from inside)
                if(!standalone && UIIconButton(ICON_MDI_CLOSE "###hubclose", 30.0f).clicked)
                    close = true;
                UIPopStyle(StyleVar_TextColor);
            }
            UIEndChild();

            UIBeginChild("##hubways", { SizeKind_Pixels, sideW }, { SizeKind_Pixels, floorf(36.0f * dpi) }, WidgetFlags_StackHorizontally);
            {
                const f32 gap                          = floorf(8.0f * dpi);
                UI_Widget* ways                        = ui->parents.Back();
                ways->style_vars[StyleVar_Padding]     = Vec4(0.0f);
                ways->style_vars[StyleVar_ItemSpacing] = Vec4(gap, 0.0f, 0.0f, 0.0f);
                const f32 each                         = floorf((sideW - gap * 2.0f) / 3.0f);
                if(sideButton("Recent###hubrecent", false, !m_HubNew, each))
                    m_HubNew = false;
                if(sideButton("New###hubnew", false, m_HubNew, each))
                    m_HubNew = true;
                if(sideButton("Open...###hubopen", false, false, each))
                    editor->BrowseForProject();
            }
            UIEndChild();
        }
        else
        {
        // ---- side: what this is, and the ways in ----
        UIBeginChild("##hubside", { SizeKind_Pixels, sideW }, { SizeKind_PercentOfParent, 1.0f }, WidgetFlags_StackVertically);
        {
            UI_Widget* side                        = ui->parents.Back();
            side->style_vars[StyleVar_Padding]     = Vec4(0.0f);
            side->style_vars[StyleVar_ItemSpacing] = Vec4(0.0f, 8.0f * dpi, 0.0f, 0.0f);

            char version[48];
            snprintf(version, sizeof(version), "Engine %d.%d.%d", LumosVersion.major, LumosVersion.minor, LumosVersion.patch);
            if(page)
            {
                // a phone on its side is short: logo and name on one line
                UIBeginRowFullWidth();
                ui->parents.Back()->flags |= WidgetFlags_CentreChildrenY;
                ui->parents.Back()->style_vars[StyleVar_ItemSpacing] = Vec4(10.0f * dpi, 0.0f, 0.0f, 0.0f);
                if(Graphics::Texture2D* logo = editor->GetLogoTexture())
                    UIImageSized("##hublogo", logo, Vec2(34.0f * dpi, 34.0f * dpi));
                UIPushStyle(StyleVar_FontSize, theme.FontSize * 1.5f * dpi);
                UILabelCStr("##hubtitle", "Lumos");
                UIPopStyle(StyleVar_FontSize);
                UIEndRow();
                UISpacer(6.0f);
            }
            else
            {
                if(Graphics::Texture2D* logo = editor->GetLogoTexture())
                    UIImageSized("##hublogo", logo, Vec2(56.0f * dpi, 56.0f * dpi));
                UIPushStyle(StyleVar_FontSize, theme.FontSize * 1.9f * dpi);
                UILabelCStr("##hubtitle", "Lumos");
                UIPopStyle(StyleVar_FontSize);
                UIPushStyle(StyleVar_TextColor, theme.TextDim);
                UILabelCStr("##hubversion", version);
                UIPopStyle(StyleVar_TextColor);
                UISpacer(14.0f);
            }

            if(sideButton(ICON_MDI_VIEW_GRID "   Recent Projects###hubrecent", false, !m_HubNew))
                m_HubNew = false;
            if(sideButton(ICON_MDI_PLUS "   New Project###hubnew", false, m_HubNew))
                m_HubNew = true;
            if(sideButton(ICON_MDI_FOLDER_OPEN "   Open Project...###hubopen", false, false))
                editor->BrowseForProject();

            UIFlexSpacer();

            if(!editor->GetProjectLoadError().empty())
            {
                UIPushStyle(StyleVar_TextColor, theme.Error);
                UILabelWrapped("##huberror", Str8C((char*)editor->GetProjectLoadError().c_str()), sideW / dpi - 8.0f);
                UIPopStyle(StyleVar_TextColor);
            }
            if(standalone)
            {
                // (a phone app is not quit from inside)
                if(!m_Touch && sideButton(ICON_MDI_CLOSE "   Quit###hubquit", false, false))
                    Application::Get().SetAppState(AppState::Closing);
            }
            else if(sideButton(ICON_MDI_CLOSE "   Close###hubclose", false, false))
                close = true;
        }
        UIEndChild();

        }

        // ---- main: the list, or the new-project form ----
        UIBeginChild("##hubmain", { SizeKind_Pixels, listW }, narrow ? UI_Size { SizeKind_Pixels, height - pad * 2.0f - headH } : UI_Size { SizeKind_PercentOfParent, 1.0f },
                     WidgetFlags_StackVertically);
        {
            UI_Widget* main                        = ui->parents.Back();
            main->style_vars[StyleVar_Padding]     = Vec4(0.0f);
            main->style_vars[StyleVar_ItemSpacing] = Vec4(0.0f, 10.0f * dpi, 0.0f, 0.0f);

            if(!m_HubNew)
            {
                const auto& recent = editor->GetSettings().m_RecentProjects;
                if(narrow && !editor->GetProjectLoadError().empty())
                {
                    UIPushStyle(StyleVar_TextColor, theme.Error);
                    UILabelWrapped("##huberrornarrow", Str8C((char*)editor->GetProjectLoadError().c_str()), listW / dpi - 8.0f);
                    UIPopStyle(StyleVar_TextColor);
                }

                UIBeginRowFullWidth();
                ui->parents.Back()->flags |= WidgetFlags_CentreChildrenY;
                UIPushStyle(StyleVar_FontSize, theme.FontSize * 1.25f * dpi);
                UILabelCStr("##hubrecenttitle", "Recent projects");
                UIPopStyle(StyleVar_FontSize);
                UIFlexSpacer();
                UI_Interaction filter = UITextField("###hubfilter", m_HubFilter, sizeof(m_HubFilter), "Filter", narrow ? 120.0f : 200.0f);
                if(filter.widget)
                    filter.widget->style_vars[StyleVar_Padding] = Vec4(8.0f * dpi, 5.0f * dpi, 0.0f, 0.0f);
                UIEndRow();

                const f32 listHeight = height - pad * 2.0f - headH - 52.0f * dpi;
                UIPushStyle(StyleVar_ItemSpacing, Vec2(0.0f, 6.0f * dpi));
                UIBeginScrollArea("##hublist", listHeight, &m_HubScroll);
                {
                    UI_Widget* area                            = ui->parents.Back();
                    area->style_vars[StyleVar_BackgroundColor] = Vec4(0.0f);
                    area->style_vars[StyleVar_BorderColor]     = Vec4(0.0f);
                    area->style_vars[StyleVar_Border]          = Vec4(0.0f);
                    area->style_vars[StyleVar_Padding]         = Vec4(0.0f, 2.0f * dpi, 0.0f, 0.0f);
                }

                u32 shown = 0;
                // most recently added first
                for(i32 i = (i32)recent.size() - 1; i >= 0; i--)
                {
                    const Editor::RecentProject& info = editor->GetRecentProject(recent[i]);
                    if(m_HubFilter[0])
                    {
                        bool found     = false;
                        const size_t n = strlen(m_HubFilter);
                        for(const char* c = info.Title.c_str(); *c && !found; c++)
                            found = strncasecmp(c, m_HubFilter, n) == 0;
                        for(const char* c = info.Folder.c_str(); *c && !found; c++)
                            found = strncasecmp(c, m_HubFilter, n) == 0;
                        if(!found)
                            continue;
                    }
                    shown++;

                    const bool current = editor->IsProjectLoaded() && recent[i].find(Application::Get().GetProjectSettings().m_ProjectRoot) == 0;

                    char id[32];
                    snprintf(id, sizeof(id), "###hubcard%d", i);
                    UI_Interaction card = UIBeginChild(id, { SizeKind_PercentOfParent, 1.0f }, { SizeKind_Pixels, floorf(62.0f * dpi) },
                                                       WidgetFlags_StackHorizontally | WidgetFlags_CentreChildrenY | WidgetFlags_DrawBackground | WidgetFlags_Clickable);
                    UI_Widget* widget = ui->parents.Back();
                    UIClearBorders(widget);
                    widget->style_vars[StyleVar_Padding]      = Vec4(14.0f * dpi, 0.0f, 0.0f, 0.0f);
                    widget->style_vars[StyleVar_ItemSpacing]  = Vec4(12.0f * dpi, 0.0f, 0.0f, 0.0f);
                    widget->style_vars[StyleVar_CornerRadius] = Vec4(10.0f * dpi, 0.0f, 0.0f, 0.0f);
                    UISetBackground(widget, current ? theme.Selection : theme.Field, theme.Selection, theme.Selection);

                    UIPushStyle(StyleVar_FontSize, theme.FontSize * 1.6f * dpi);
                    UIPushStyle(StyleVar_TextColor, info.Missing ? theme.TextDim : theme.Accent);
                    UILabelCStr("###icon", info.Missing ? ICON_MDI_ALERT : ICON_MDI_FOLDER);
                    UIPopStyle(StyleVar_TextColor);
                    UIPopStyle(StyleVar_FontSize);

                    UIBeginChild("###text", { SizeKind_ChildSum, 1.0f }, { SizeKind_ChildSum, 1.0f }, WidgetFlags_StackVertically);
                    {
                        UI_Widget* text                        = ui->parents.Back();
                        text->style_vars[StyleVar_Padding]     = Vec4(0.0f);
                        text->style_vars[StyleVar_ItemSpacing] = Vec4(0.0f, 2.0f * dpi, 0.0f, 0.0f);

                        char title[160];
                        snprintf(title, sizeof(title), "%s%s%s", info.Title.c_str(), info.Missing ? "   (missing)" : "", current ? "   (open)" : "");
                        UIPushStyle(StyleVar_FontSize, theme.FontSize * 1.08f * dpi);
                        UIPushStyle(StyleVar_TextColor, info.Missing ? theme.TextDim : theme.Text);
                        UILabelCStr("###title", title);
                        UIPopStyle(StyleVar_TextColor);
                        UIPopStyle(StyleVar_FontSize);

                        std::string line = info.Folder + (info.Export.empty() ? "" : "     " + info.Export);
                        UIPushStyle(StyleVar_FontSize, theme.FontSize * 0.92f * dpi);
                        UIPushStyle(StyleVar_TextColor, theme.TextDim);
                        UILabelEllipsis("###where", Str8C((char*)line.c_str()), listW / dpi - 130.0f);
                        UIPopStyle(StyleVar_TextColor);
                        UIPopStyle(StyleVar_FontSize);
                    }
                    UIEndChild();

                    UIFlexSpacer();
                    bool onButton = false;
                    // no hover on a touch screen: the remove button is always there
                    if(card.hovering || m_Touch)
                    {
                        UIPushStyle(StyleVar_TextColor, theme.TextDim);
                        if(!info.Missing && !m_Touch)
                        {
                            UI_Interaction reveal = UIIconButton(ICON_MDI_FOLDER_OPEN "###reveal", 26.0f);
                            UITooltip("Show the folder");
                            onButton |= reveal.hovering;
                            if(reveal.clicked)
                                OS::Get().OpenFileLocation(recent[i]);
                        }
                        UI_Interaction remove = UIIconButton(ICON_MDI_CLOSE "###remove", 26.0f);
                        UITooltip("Remove from this list");
                        onButton |= remove.hovering;
                        if(remove.clicked)
                            removeAt = i;
                        UIPopStyle(StyleVar_TextColor);
                    }
                    UIEndChild();

                    if(card.clicked && !onButton && !info.Missing && !dialogUp)
                        snprintf(openPath, sizeof(openPath), "%s", recent[i].c_str());
                }
                if(shown == 0)
                {
                    UIPushStyle(StyleVar_TextColor, theme.TextDim);
                    UILabelCStr("##hubnone", recent.empty() ? "Nothing here yet. Make a new project or open one." : "Nothing matches");
                    UIPopStyle(StyleVar_TextColor);
                }
                UIEndScrollArea();
                UIPopStyle(StyleVar_ItemSpacing);
            }
            else
            {
                UIPushStyle(StyleVar_FontSize, theme.FontSize * 1.25f * dpi);
                UILabelCStr("##hubnewtitle", "New project");
                UIPopStyle(StyleVar_FontSize);

                UIBeginPropertyGrid("##hubnewgrid", 0.24f);
                UIPropertyText("Name", m_NewName, sizeof(m_NewName), "ProjectName");
                UIPropertyText("Location", m_NewLocation, sizeof(m_NewLocation), "Folder to make it in");
                UIBeginPropertyRow("");
                if(UIButton(ICON_MDI_FOLDER_OPEN "  Choose Folder...###hubbrowse").clicked)
                {
                    UIFileDialogDesc dialog;
                    dialog.Title       = "Where to make the project";
                    dialog.StartFolder = m_NewLocation;
                    dialog.PickFolder  = true;
                    dialog.Accept      = "Choose";
                    dialog.OnPicked    = [](const char* picked, void* user)
                    {
                        EditorUI* self = (EditorUI*)user;
                        snprintf(self->m_NewLocation, sizeof(self->m_NewLocation), "%s", picked);
                    };
                    dialog.User = this;
                    UIOpenFileDialog(dialog);
                }
                UIEndPropertyRow();
                UIEndPropertyGrid();

                // what will be made, and anything in the way of it
                std::string location = m_NewLocation;
                while(location.size() > 1 && location.back() == '/')
                    location.pop_back();
                const std::string target = location + "/" + m_NewName + "/";
                std::error_code ec;
                const char* problem = nullptr;
                if(!m_NewName[0])
                    problem = "The project needs a name";
                else if(strpbrk(m_NewName, "/\\:"))
                    problem = "The name cannot contain / \\ or :";
                else if(location.empty() || !std::filesystem::is_directory(location, ec))
                    problem = "The location is not a folder";
                else if(std::filesystem::exists(target, ec))
                    problem = "There is already something with that name there";

                UIPushStyle(StyleVar_TextColor, problem ? theme.Warning : theme.TextDim);
                UILabelWrapped("##hubtarget", Str8C((char*)(problem ? problem : target.c_str())), listW / dpi - 10.0f);
                UIPopStyle(StyleVar_TextColor);

                UIBeginRowFullWidth();
                {
                    // the one thing this form is for: the accent, greyed while something is in the way
                    u64 hash;
                    String8 text      = HandleUIString(ICON_MDI_PLUS "   Create Project###hubcreate", &hash);
                    UI_Widget* button = PushWidget(WidgetFlags_DrawBackground | WidgetFlags_DrawText | WidgetFlags_Clickable, text, hash,
                                                   { SizeKind_TextContent, 1.0f }, { SizeKind_Pixels, floorf(36.0f * dpi) });
                    UIClearBorders(button);
                    button->TextAlignment                     = UI_Text_Alignment_Center_Y;
                    button->style_vars[StyleVar_Padding]      = Vec4(18.0f * dpi, 0.0f, 0.0f, 0.0f);
                    button->style_vars[StyleVar_CornerRadius] = Vec4(9.0f * dpi, 0.0f, 0.0f, 0.0f);
                    const Vec4 fill = problem ? theme.Field : theme.Accent;
                    UISetBackground(button, fill, fill, fill);
                    UISetTextColour(button, problem ? theme.TextDim : theme.AccentText);
                    if(HandleWidgetInteraction(button).clicked && !problem && !dialogUp)
                        create = true;
                }
                UIEndRow();

                UIFlexSpacer();
                UIPushStyle(StyleVar_TextColor, theme.TextDim);
                UILabelWrapped("##hubnewnote", Str8Lit("A project is a folder with a .lmproj file, an Assets folder and a first scene. Scripts are Lua: nothing to compile."), listW / dpi - 10.0f);
                UIPopStyle(StyleVar_TextColor);
            }
        }
        UIEndChild();

        UIEndPanel();
        UIPopStyle(StyleVar_FontSize);

        // acted on last: opening a project rebuilds a lot of what was just drawn
        if(removeAt >= 0)
            editor->RemoveRecentProject((size_t)removeAt);
        if(create)
        {
            std::string location = m_NewLocation;
            editor->CreateProject(location, m_NewName);
            if(editor->IsProjectLoaded() && editor->GetProjectLoadError().empty())
                close = true;
        }
        else if(openPath[0])
        {
            editor->ProjectOpenCallback(openPath);
            if(editor->GetProjectLoadError().empty())
                close = true;
        }
        if(close)
        {
            m_HubOpen = false;
            ui->FocusedTextInput    = 0;
            ui->TextInputBuffer     = nullptr;
            ui->TextInputBufferSize = 0;
        }
    }
}
