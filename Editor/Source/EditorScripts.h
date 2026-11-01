#pragma once

namespace Lumos
{
    class Editor;
    class EditorUI;
    struct UIDockSpace;

    // Lua run by the editor itself: every .lua in <project>/Editor/ is loaded when the project opens.
    // A script adds to the editor through the `Editor` table:
    //   Editor.RegisterCommand(name, fn [, hint])      a row in the command palette
    //   Editor.RegisterPanel(name, fn(width, height))  a dock panel; build it with the UI* functions
    //   Editor.GetSelected() / Select(entity) / OpenPanel(name) / Play() / Stop() / SaveScene() / Reload()
    //   Editor.RunCommand(name [, category])           any palette command, by name
    //   Editor.StartTour({ { panel = "Inspector", title = "...", text = "..." }, ... })
    // Scripts in <engine assets>/EditorScripts/ are loaded first, for every project (the Synth panel is one).
    // The palette's "=" mode runs one line of Lua with `sel` = the selected entity and `scene` = the open scene.
    namespace EditorScripts
    {
        void Init(Editor* editor, UIDockSpace* dock, EditorUI* ui);
        // Drops what the last project's scripts registered and runs the ones in this project. "" = only drop.
        void Load(const char* projectRoot);
        void Shutdown();
        // Files loaded / that failed, for a status line.
        // A script or the palette asked for a reload; true once.
        bool TakeReloadRequest();
        // one line of Lua as the palette's "=" mode runs it (sel / scene in reach)
        void RunLine(const char* line);
        int LoadedCount();
        int ErrorCount();
    }
}
