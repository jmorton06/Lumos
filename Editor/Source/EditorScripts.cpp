#include "EditorScripts.h"
#include "Editor.h"
#include "EditorUI.h"
#include <Lumos/Scene/SceneUndo.h>

#include <Lumos/Core/Application.h>
#include <Lumos/Core/CommandPalette.h>
#include <Lumos/Graphics/UI.h>
#include <Lumos/Graphics/UIDock.h>
#include <Lumos/ImGui/IconsMaterialDesignIcons.h>
#include <Lumos/Scene/Entity.h>
#include <Lumos/Scripting/Lua/LuaManager.h>

#include <sol/sol.hpp>
#include <filesystem>

namespace Lumos
{
    namespace
    {
        constexpr int kMaxCommands = 48;
        constexpr int kMaxPanels   = 8;

        struct ScriptCommand
        {
            char Name[64];
            char Hint[32];
            sol::protected_function Run;
        };

        struct ScriptPanel
        {
            char Name[48];
            char Title[64];
            bool Registered; // in the dock (panels are never taken out again, only left empty)
            sol::protected_function Draw;
        };

        EditorUI* s_UI = nullptr;
        char s_LuaLine[2048] = { 0 };
        Editor* s_Editor    = nullptr;
        UIDockSpace* s_Dock = nullptr;
        ScriptCommand s_Commands[kMaxCommands];
        int s_CommandCount = 0;
        ScriptPanel s_Panels[kMaxPanels];
        int s_PanelCount = 0;
        int s_Loaded     = 0;
        int s_Errors     = 0;
        bool s_Reload    = false;
        char s_Root[512] = { 0 };

        const char* kCategory = "Script";

        void RunCommand(u64 index)
        {
            if(index >= (u64)s_CommandCount || !s_Commands[index].Run.valid())
                return;
            sol::protected_function_result result = s_Commands[index].Run();
            if(!result.valid())
            {
                sol::error error = result;
                LERROR("Editor script command '%s': %s", s_Commands[index].Name, error.what());
            }
        }

        void DrawPanel(const UIDockPanelContext& context)
        {
            ScriptPanel& panel = s_Panels[(size_t)context.UserData];
            if(!panel.Draw.valid())
            {
                UILabelCStr("##scriptpanelgone", "The script that drew this panel is not loaded");
                return;
            }
            // a script error part way through must not leave its containers open
            UI_State* ui       = GetUIState();
            const u32 parents  = (u32)ui->parents.Size();
            sol::protected_function_result result = panel.Draw(context.Size.x / context.DPIScale, context.Size.y / context.DPIScale);
            if(!result.valid())
            {
                sol::error error = result;
                LERROR("Editor script panel '%s': %s", panel.Name, error.what());
                panel.Draw = sol::protected_function();
                while((u32)ui->parents.Size() > parents)
                    ui->parents.PopBack();
            }
        }

        // The palette's "=" mode: the line typed, run with `sel` and `scene` in reach.
        void RunLuaLine(u64)
        {
            sol::state& lua = LuaManager::Get().GetState();
            sol::environment environment(lua, sol::create, lua.globals());
            const auto& selected = s_Editor->GetSelected();
            Entity entity        = selected.empty() ? Entity() : selected.front();
            if(entity.Valid())
                environment["sel"] = entity;
            environment["scene"] = Application::Get().GetCurrentScene();

            // an expression prints its value; anything else just runs. The whole entity is one undo step.
            if(entity.Valid())
                SceneUndo::BeginEntityEdit(entity);
            std::string code = std::string("return ") + s_LuaLine;
            sol::protected_function_result result = lua.safe_script(code, environment, sol::script_pass_on_error);
            if(!result.valid())
                result = lua.safe_script(s_LuaLine, environment, sol::script_pass_on_error);
            if(entity.Valid())
                SceneUndo::EndEntityEdit(entity, "Lua");

            if(!result.valid())
            {
                sol::error error = result;
                LERROR("= %s: %s", s_LuaLine, error.what());
                // the log is buffered: a screenshot run quits before it is written
                fprintf(stderr, "Lua line failed: %s\n", error.what());
            }
            else if(result.return_count() > 0 && result.get_type() != sol::type::lua_nil)
            {
                sol::protected_function tostring = lua["tostring"];
                sol::protected_function_result text = tostring(result.get<sol::object>());
                if(text.valid())
                    LINFO("= %s  ->  %s", s_LuaLine, text.get<std::string>().c_str());
            }
        }

        void ModeLua(const char* query)
        {
            while(*query == ' ')
                query++;
            CommandPalette::CommandDesc desc;
            desc.Category = "Lua";
            desc.Icon     = ICON_MDI_CODE_BRACES;
            desc.Always   = true;
            if(!query[0])
            {
                desc.Name     = "sel:GetTransform():SetLocalScale(Vec3.new(2, 2, 2))    sel = selection, scene = the scene";
                desc.KeepOpen = true;
                CommandPalette::AddResult(desc);
                return;
            }
            snprintf(s_LuaLine, sizeof(s_LuaLine), "%s", query);
            char name[80];
            snprintf(name, sizeof(name), "Run  %.70s", s_LuaLine);
            desc.Name = name;
            desc.Run  = &RunLuaLine;
            CommandPalette::AddResult(desc);
        }

        void Drop()
        {
            for(int i = 0; i < s_CommandCount; i++)
            {
                CommandPalette::Unregister(s_Commands[i].Name, kCategory);
                s_Commands[i].Run = sol::protected_function();
            }
            s_CommandCount = 0;
            for(int i = 0; i < s_PanelCount; i++)
                s_Panels[i].Draw = sol::protected_function();
        }

        void Bind(sol::state& lua)
        {
            sol::table editor = lua.create_named_table("Editor");

            editor.set_function("RegisterCommand", [](const std::string& name, sol::protected_function run, sol::optional<std::string> hint)
                                {
                                    if(s_CommandCount >= kMaxCommands || !run.valid())
                                        return false;
                                    ScriptCommand& command = s_Commands[s_CommandCount];
                                    snprintf(command.Name, sizeof(command.Name), "%s", name.c_str());
                                    snprintf(command.Hint, sizeof(command.Hint), "%s", hint ? hint->c_str() : "");
                                    command.Run = run;

                                    CommandPalette::CommandDesc desc;
                                    desc.Name     = command.Name;
                                    desc.Category = kCategory;
                                    desc.Icon     = ICON_MDI_PUZZLE;
                                    desc.Hint     = command.Hint[0] ? command.Hint : nullptr;
                                    desc.Run      = &RunCommand;
                                    desc.Arg      = (u64)s_CommandCount;
                                    CommandPalette::Register(desc);
                                    s_CommandCount++;
                                    return true; });

            editor.set_function("RegisterPanel", [](const std::string& name, sol::protected_function draw)
                                {
                                    if(!draw.valid() || !s_Dock)
                                        return false;
                                    int slot = -1;
                                    for(int i = 0; i < s_PanelCount; i++)
                                        if(strcmp(s_Panels[i].Name, name.c_str()) == 0)
                                            slot = i;
                                    if(slot < 0)
                                    {
                                        if(s_PanelCount >= kMaxPanels)
                                            return false;
                                        slot               = s_PanelCount++;
                                        ScriptPanel& panel = s_Panels[slot];
                                        snprintf(panel.Name, sizeof(panel.Name), "%s", name.c_str());
                                        snprintf(panel.Title, sizeof(panel.Title), ICON_MDI_PUZZLE " %s", name.c_str());
                                        panel.Registered = false;
                                    }
                                    ScriptPanel& panel = s_Panels[slot];
                                    panel.Draw         = draw;
                                    if(!panel.Registered)
                                    {
                                        UIDockPanelDesc desc;
                                        desc.Name     = panel.Name;
                                        desc.Title    = panel.Title;
                                        desc.Draw     = &DrawPanel;
                                        desc.UserData = (void*)(size_t)slot;
                                        desc.Slot     = UIDockSlot_Right;
                                        desc.Drawer   = true;
                                        UIDockRegisterPanel(s_Dock, desc);
                                        panel.Registered = true;
                                    }
                                    return true; });

            editor.set_function("GetSelected", [](sol::this_state state) -> sol::object
                                {
                                    const auto& selected = s_Editor->GetSelected();
                                    Entity front = selected.empty() ? Entity() : selected.front();
                                    if(!front.Valid())
                                        return sol::make_object(state, sol::lua_nil);
                                    return sol::make_object(state, front); });
            editor.set_function("Select", [](sol::optional<Entity> entity)
                                {
                                    s_Editor->ClearSelected();
                                    if(entity && entity->Valid())
                                        s_Editor->SetSelected(*entity); });
            editor.set_function("OpenPanel", [](const std::string& name)
                                {
                                    if(s_Dock)
                                        UIDockFocusPanel(s_Dock, name.c_str()); });
            editor.set_function("Play", []()
                                {
                                    if(Application::Get().GetEditorState() == EditorState::Preview)
                                        s_Editor->TogglePlay(); });
            editor.set_function("Stop", []()
                                {
                                    if(Application::Get().GetEditorState() != EditorState::Preview)
                                        s_Editor->TogglePlay(); });
            editor.set_function("IsPlaying", []() { return Application::Get().GetEditorState() != EditorState::Preview; });
            editor.set_function("SaveScene", []() { s_Editor->SaveCurrentScene(); });
            editor.set_function("ProjectRoot", []() { return std::string(s_Root); });
            editor.set_function("RunCommand", [](const std::string& name, sol::optional<std::string> category)
                                { return CommandPalette::Run(name.c_str(), category ? category->c_str() : nullptr); });
            editor.set_function("StartTour", [](sol::table steps)
                                {
                                    if(!s_UI)
                                        return;
                                    // strings must outlive the call: StartTour copies them before these go
                                    std::string text[16][3];
                                    EditorUI::TourStep tour[16];
                                    u32 count = 0;
                                    for(auto& pair : steps)
                                    {
                                        if(count >= 16 || !pair.second.is<sol::table>())
                                            continue;
                                        sol::table step = pair.second.as<sol::table>();
                                        text[count][0]  = step.get_or<std::string>("panel", "");
                                        text[count][1]  = step.get_or<std::string>("title", "");
                                        text[count][2]  = step.get_or<std::string>("text", "");
                                        tour[count]     = { text[count][0].c_str(), text[count][1].c_str(), text[count][2].c_str() };
                                        count++;
                                    }
                                    s_UI->StartTour(tour, count); });
            // not straight away: the caller is one of the functions about to be dropped
            editor.set_function("Reload", []() { s_Reload = true; });
        }
    }

    namespace EditorScripts
    {
        void RunLine(const char* line)
        {
            snprintf(s_LuaLine, sizeof(s_LuaLine), "%s", line);
            RunLuaLine(0);
        }

        void Init(Editor* editor, UIDockSpace* dock, EditorUI* ui)
        {
            s_Editor = editor;
            s_Dock   = dock;
            s_UI     = ui;
            CommandPalette::AddMode('=', "Lua", "  A line of Lua. sel = the selected entity", &ModeLua);
            CommandPalette::CommandDesc desc;
            desc.Name     = "Reload Editor Scripts";
            desc.Category = "Editor";
            desc.Icon     = ICON_MDI_REFRESH;
            desc.Run      = [](u64) { s_Reload = true; };
            CommandPalette::Register(desc);
        }

        void Load(const char* projectRoot)
        {
            s_Reload = false;
            Drop();
            s_Loaded = 0;
            s_Errors = 0;
            snprintf(s_Root, sizeof(s_Root), "%s", projectRoot ? projectRoot : "");
            if(!s_Root[0])
                return;

            sol::state& lua = LuaManager::Get().GetState();
            Bind(lua);

            // the engine's own first (for every project), then the project's; each folder by name,
            // so one script can count on another
            std::error_code error;
            const std::filesystem::path folders[2] = {
                std::filesystem::path(Application::Get().GetProjectSettings().m_EngineAssetPath) / "EditorScripts",
                std::filesystem::path(s_Root) / "Editor",
            };
            const std::filesystem::path& folder = folders[1];
            TDArray<std::string> files;
            for(const std::filesystem::path& from : folders)
            {
                if(!std::filesystem::is_directory(from, error))
                    continue;
                const u32 first = (u32)files.Size();
                for(const auto& entry : std::filesystem::directory_iterator(from, error))
                    if(entry.path().extension() == ".lua")
                        files.PushBack(entry.path().string());
                for(u32 i = first; i < (u32)files.Size(); i++)
                    for(u32 j = i + 1; j < (u32)files.Size(); j++)
                        if(files[j] < files[i])
                            std::swap(files[i], files[j]);
            }

            for(const std::string& file : files)
            {
                // its own environment: an editor script cannot trample the game's globals by accident
                sol::environment environment(lua, sol::create, lua.globals());
                sol::protected_function_result result = lua.safe_script_file(file, environment, sol::script_pass_on_error);
                if(result.valid())
                    s_Loaded++;
                else
                {
                    sol::error scriptError = result;
                    LERROR("Editor script %s: %s", file.c_str(), scriptError.what());
                    s_Errors++;
                }
            }
            if(s_Loaded || s_Errors)
                LINFO("Editor scripts: %d loaded, %d failed (%s)", s_Loaded, s_Errors, folder.string().c_str());
        }

        void Shutdown()
        {
            Drop();
            s_Editor = nullptr;
            s_Dock   = nullptr;
            s_UI     = nullptr;
        }

        int LoadedCount() { return s_Loaded; }
        int ErrorCount() { return s_Errors; }

        // called by the editor UI each frame
        bool TakeReloadRequest()
        {
            const bool reload = s_Reload;
            s_Reload          = false;
            return reload;
        }
    }
}
