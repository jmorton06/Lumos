#include "Precompiled.h"
#include "LuaScriptComponent.h"
#include "LuaManager.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/EntityManager.h"
#include "Utilities/StringUtilities.h"
#include "Core/Engine.h"
#include "Core/OS/FileSystem.h"
#include "Core/Application.h"
#include "Core/TestRunner.h"
#include "Core/ToolPanels.h"
#include "Graphics/UI.h"
#include "Graphics/UITheme.h"
#include "Graphics/UI.h"
#include "ImGui/IconsMaterialDesignIcons.h"

#include <sol/sol.hpp>

namespace Lumos
{
    LuaScriptComponent::LuaScriptComponent()
    {
        m_Scene    = nullptr;
        m_FileName = "";
        m_Env      = nullptr;
        // m_UUID = UUID();
    }
    LuaScriptComponent::LuaScriptComponent(const std::string& fileName, Scene* scene)
    {
        m_Scene    = scene;
        m_FileName = fileName;
        m_Env      = nullptr;
        // m_UUID = UUID();

        Init();
    }

    void LuaScriptComponent::Init()
    {
        LoadScript(m_FileName);
    }

    LuaScriptComponent::~LuaScriptComponent()
    {
        if(m_Env)
        {
            sol::protected_function releaseFunc = (*m_Env)["OnRelease"];
            if(releaseFunc.valid())
                releaseFunc.call();
        }
    }

    void LuaScriptComponent::LoadScript(const std::string& fileName)
    {
        m_FileName = fileName;
        m_Env      = CreateSharedPtr<sol::environment>(LuaManager::Get().GetState(), sol::create, LuaManager::Get().GetState().globals());

        LINFO("[Lua] LoadScript '%s'", m_FileName.c_str());

        auto loadFileResult = [&]() {
            ArenaTemp scratch = ScratchBegin(0, 0);
            String8 vfsPath = Str8((u8*)m_FileName.c_str(), m_FileName.size());
            String8 contents = FileSystem::Get().ReadTextFileVFS(scratch.arena, vfsPath);
            if(contents.size > 0 && contents.str != nullptr)
            {
                std::string code((const char*)contents.str, contents.size);
                ScratchEnd(scratch);
                LINFO("[Lua] loaded %zu bytes via VFS for '%s'", code.size(), m_FileName.c_str());
                return LuaManager::Get().GetState().safe_script(code, *m_Env, sol::script_pass_on_error);
            }
            ScratchEnd(scratch);

            std::string actualPath = m_FileName;
            if(m_FileName.find("//") == 0)
            {
                ArenaTemp scratch2 = ScratchBegin(0, 0);
                String8 virtualPath = Str8((u8*)m_FileName.c_str(), m_FileName.size());
                String8 physicalPath;
                if(FileSystem::Get().ResolvePhysicalPath(scratch2.arena, virtualPath, &physicalPath, false))
                    actualPath = std::string((const char*)physicalPath.str, physicalPath.size);
                ScratchEnd(scratch2);
            }
            LINFO("[Lua] VFS miss, falling back to disk path '%s'", actualPath.c_str());
            return LuaManager::Get().GetState().safe_script_file(actualPath, *m_Env, sol::script_pass_on_error);
        }();

        if(!loadFileResult.valid())
        {
            sol::error err = loadFileResult;
            LERROR("Failed to Execute Lua script %s", m_FileName.c_str());
            LERROR("Error : %s", err.what());
            std::string filename = StringUtilities::GetFileName(m_FileName);
            std::string error    = std::string(err.what());

            int line     = 1;
            auto linepos = error.find(".lua:");
            if(linepos != std::string::npos)
            {
                std::string errorLine = error.substr(linepos + 5);
                auto lineposEnd       = errorLine.find(":");
                if(lineposEnd != std::string::npos)
                {
                    errorLine = errorLine.substr(0, lineposEnd);
                    try { line = std::stoi(errorLine); } catch(...) {}
                    error = error.substr(linepos + errorLine.size() + lineposEnd + 4);
                }
            }

            m_Errors[line] = std::string(error);
            TestRunner::ReportScriptError(m_FileName.c_str(), err.what());
        }
        else
            m_Errors = {};

        if(!m_Scene)
            m_Scene = Application::Get().GetCurrentScene();

        (*m_Env)["CurrentScene"] = m_Scene;
        (*m_Env)["LuaComponent"] = this;

        m_OnInitFunc = CreateSharedPtr<sol::protected_function>((*m_Env)["OnInit"]);
        if(!m_OnInitFunc->valid())
            m_OnInitFunc.reset();

        m_UpdateFunc = CreateSharedPtr<sol::protected_function>((*m_Env)["OnUpdate"]);
        if(!m_UpdateFunc->valid())
            m_UpdateFunc.reset();

        m_Phys2DBeginFunc = CreateSharedPtr<sol::protected_function>((*m_Env)["OnCollision2DBegin"]);
        if(!m_Phys2DBeginFunc->valid())
            m_Phys2DBeginFunc.reset();

        m_Phys2DEndFunc = CreateSharedPtr<sol::protected_function>((*m_Env)["OnCollision2DEnd"]);
        if(!m_Phys2DEndFunc->valid())
            m_Phys2DEndFunc.reset();

        m_Phys3DBeginFunc = CreateSharedPtr<sol::protected_function>((*m_Env)["OnCollision3DBegin"]);
        if(!m_Phys3DBeginFunc->valid())
            m_Phys3DBeginFunc.reset();

        m_Phys3DEndFunc = CreateSharedPtr<sol::protected_function>((*m_Env)["OnCollision3DEnd"]);
        if(!m_Phys3DEndFunc->valid())
            m_Phys3DEndFunc.reset();

        LINFO("[Lua] bound script='%s' OnInit=%d OnUpdate=%d errored=%d",
              m_FileName.c_str(),
              m_OnInitFunc ? 1 : 0,
              m_UpdateFunc ? 1 : 0,
              m_Errored ? 1 : 0);

        LuaManager::Get().GetState().collect_garbage();
    }

    void LuaScriptComponent::OnScriptError(const char* funcName, const char* errorMsg)
    {
        LERROR("Failed to Execute Script Lua %s", funcName);
        LERROR("Error : %s", errorMsg);

        m_Errored = true;

        TestRunner::ReportScriptError(funcName, errorMsg);

        // Pause editor on script error
        if(Application::Get().GetEditorState() == EditorState::Play)
        {
            LWARN("Exiting playmode due to script error in %s", m_FileName.c_str());
            Application::Get().ExitApp();
        }
    }

    void LuaScriptComponent::CheckForReload()
    {
        if(m_FileName.empty())
            return;

        // Resolve path
        std::string actualPath = m_FileName;
        if(m_FileName.find("//") == 0)
        {
            ArenaTemp scratch = ScratchBegin(0, 0);
            String8 virtualPath = Str8((u8*)m_FileName.c_str(), m_FileName.size());
            String8 physicalPath;
            if(FileSystem::Get().ResolvePhysicalPath(scratch.arena, virtualPath, &physicalPath, false))
            {
                actualPath = std::string((const char*)physicalPath.str, physicalPath.size);
            }
            ScratchEnd(scratch);
        }

        uint64_t modTime = FileSystem::GetFileModifiedTime(Str8StdS(actualPath));
        if(m_LastModifiedTime == 0)
        {
            m_LastModifiedTime = modTime;
            return;
        }

        if(modTime > m_LastModifiedTime)
        {
            LINFO("Hot-reloading script: %s", m_FileName.c_str());
            m_LastModifiedTime = modTime;
            m_Errored = false;
            Reload();
        }
    }

    void LuaScriptComponent::OnInit()
    {
        if(m_Errored || !m_OnInitFunc)
            return;

        sol::protected_function_result result = m_OnInitFunc->call();
        if(!result.valid())
        {
            sol::error err = result;
            OnScriptError("Init", err.what());
        }
    }

    void LuaScriptComponent::OnUpdate(float dt)
    {
        if(m_Errored || !m_UpdateFunc)
            return;

        sol::protected_function_result result = m_UpdateFunc->call(dt);
        if(!result.valid())
        {
            sol::error err = result;
            OnScriptError("OnUpdate", err.what());
        }
    }

    // Optional OnBackgroundUpdate(dt): runs at a low rate while the app is backgrounded and not
    // drawing (iOS), e.g. to move a music player on to the next track.
    void LuaScriptComponent::OnBackgroundUpdate(float dt)
    {
        if(m_Errored || !m_Env)
            return;
        sol::protected_function fn = (*m_Env)["OnBackgroundUpdate"];
        if(!fn.valid())
            return;
        sol::protected_function_result result = fn.call(dt);
        if(!result.valid())
        {
            sol::error err = result;
            OnScriptError("OnBackgroundUpdate", err.what());
        }
    }

    void LuaScriptComponent::Reload()
    {
        if(m_Env)
        {
            sol::protected_function releaseFunc = (*m_Env)["OnRelease"];
            if (releaseFunc.valid())
            {
                sol::protected_function_result result = releaseFunc.call();
                if (!result.valid())
                {
                    sol::error err = result;
                    OnScriptError("OnRelease", err.what());
                }
            }
        }

        static const bool reloadModules = []
        {
            const char* v = std::getenv("LUMOS_HOT_RELOAD_MODULES");
            return v && v[0] == '1';
        }();

        if(reloadModules)
        {
            sol::table loaded = LuaManager::Get().GetState()["package"]["loaded"];
            std::vector<std::string> toRemove;
            for(auto& kv : loaded)
            {
                if(!kv.first.is<std::string>())
                    continue;
                std::string key = kv.first.as<std::string>();
                // Project modules only - a '/' or '.' in the name means it came
                // through the VFS searcher, not Lua's own standard library.
                if(key.find('/') != std::string::npos || key.find('.') != std::string::npos)
                    toRemove.push_back(key);
            }
            for(const auto& k : toRemove)
                loaded[k] = sol::lua_nil;

            if(!toRemove.empty())
                LINFO("[Lua] hot-reload: dropped %zu module(s) from package.loaded", toRemove.size());
        }

        Init();
    }

    Entity LuaScriptComponent::GetCurrentEntity()
    {
        // TODO: Faster alternative
        if(!m_Scene)
            m_Scene = Application::Get().GetCurrentScene();

        auto entities = m_Scene->GetEntityManager()->GetEntitiesWithType<LuaScriptComponent>();

        for(auto entity : entities)
        {
            LuaScriptComponent* comp = &entity.GetComponent<LuaScriptComponent>();
            if(comp->GetFilePath() == GetFilePath())
                return entity;
        }

        return Entity();
    }

    void LuaScriptComponent::SetThisComponent()
    {
        if(m_Env)
        {
            (*m_Env)["LuaComponent"] = this;
        }
    }

    void LuaScriptComponent::Load(const std::string& fileName)
    {
        if(m_Env)
        {
            sol::protected_function releaseFunc = (*m_Env)["OnRelease"];
            if(releaseFunc.valid())
                releaseFunc.call();
        }

        m_FileName = fileName;
        Init();
    }

    void LuaScriptComponent::OnCollision2DBegin()
    {
        if(m_Errored || !m_Phys2DBeginFunc)
            return;

        sol::protected_function_result result = m_Phys2DBeginFunc->call();
        if(!result.valid())
        {
            sol::error err = result;
            OnScriptError("OnCollision2DBegin", err.what());
        }
    }

    void LuaScriptComponent::OnCollision2DEnd()
    {
        if(m_Errored || !m_Phys2DEndFunc)
            return;

        sol::protected_function_result result = m_Phys2DEndFunc->call();
        if(!result.valid())
        {
            sol::error err = result;
            OnScriptError("OnCollision2DEnd", err.what());
        }
    }

    void LuaScriptComponent::OnCollision3DBegin(const CollisionInfo3D& info)
    {
        if(m_Errored || !m_Phys3DBeginFunc)
            return;

        sol::protected_function_result result = m_Phys3DBeginFunc->call(info);
        if(!result.valid())
        {
            sol::error err = result;
            OnScriptError("OnCollision3DBegin", err.what());
        }
    }

    void LuaScriptComponent::OnCollision3DEnd(const CollisionInfo3D& info)
    {
        if(m_Errored || !m_Phys3DEndFunc)
            return;

        sol::protected_function_result result = m_Phys3DEndFunc->call(info);
        if(!result.valid())
        {
            sol::error err = result;
            OnScriptError("OnCollision3DEnd", err.what());
        }
    }

    static void DrawLuaScriptInspector(Entity, void* component)
    {
        LuaScriptComponent* script   = (LuaScriptComponent*)component;
        const UIToolTheme& theme     = UIGetToolTheme();
        const f32 dpi                = GetUIState()->DPIScale;
        const std::string& path      = script->GetFilePath();
        const char* name             = strrchr(path.c_str(), '/');
        const bool hasFile           = !path.empty();
        char dropped[256]            = { 0 };
        bool gotDrop                 = false;

        // ---- the file: its name, and the things to do with it ----
        UIBeginPropertyRow("Script");
        UIPushStyle(StyleVar_TextColor, hasFile ? theme.Text : theme.TextDim);
        UILabelEllipsis("##luafile", Str8C((char*)(hasFile ? (name ? name + 1 : path.c_str()) : "none - drop a .lua file here")), 150.0f);
        UIPopStyle(StyleVar_TextColor);
        gotDrop |= ToolPanels::AssetDropTarget(".lua", dropped, sizeof(dropped));
        UIFlexSpacer();
        gotDrop |= ToolPanels::AssetPickButton("###luapick", ".lua", dropped, sizeof(dropped));
        UIEndPropertyRow();
        if(hasFile)
            UIPropertyLabel("Path", Str8C((char*)path.c_str()));

        UIBeginPropertyRow("");
        if(UIButton(ICON_MDI_PENCIL "  Edit###luaedit").clicked && hasFile)
            ToolPanels::OpenAsset(path.c_str());
        gotDrop |= ToolPanels::AssetDropTarget(".lua", dropped, sizeof(dropped));
        if(UIButton(ICON_MDI_REFRESH "  Reload###luareload").clicked && hasFile)
        {
            script->ClearError();
            script->Reload();
        }
        UIEndPropertyRow();

        // ---- how it is doing ----
        if(!hasFile)
            UIPropertyLabel("Status", Str8Lit("no script"));
        else if(script->IsErrored() || !script->GetErrors().empty())
        {
            UIBeginPropertyRow("Status");
            UIPushStyle(StyleVar_TextColor, theme.Error);
            UILabelCStr("##luastatus", ICON_MDI_ALERT_CIRCLE "  error");
            UIPopStyle(StyleVar_TextColor);
            UIEndPropertyRow();
            int shown = 0;
            for(const auto& error : script->GetErrors())
            {
                if(shown++ >= 4)
                    break;
                char id[32];
                snprintf(id, sizeof(id), "##luaerror%d", error.first);
                UIPushStyle(StyleVar_TextColor, theme.Error);
                UILabelWrapped(id, Str8C((char*)error.second.c_str()), 300.0f);
                UIPopStyle(StyleVar_TextColor);
            }
        }
        else
            UIPropertyLabel("Status", script->Loaded() ? Str8Lit("loaded") : Str8Lit("not loaded"));

        // ---- which of the engine's calls it answers ----
        if(script->Loaded())
        {
            const sol::environment& env = script->GetSolEnvironment();
            static const char* kHooks[] = { "OnInit", "OnUpdate", "OnRelease", "OnCollision2DBegin", "OnCollision2DEnd", "OnCollision3DBegin", "OnCollision3DEnd" };
            char hooks[200];
            int at = 0;
            hooks[0] = 0;
            for(const char* hook : kHooks)
            {
                sol::object object = env[hook];
                if(object.valid() && object.get_type() == sol::type::function && at < (int)sizeof(hooks) - 32)
                    at += snprintf(hooks + at, sizeof(hooks) - (size_t)at, "%s%s", at ? ", " : "", hook);
            }
            UIBeginPropertyRow("Handles");
            UIPushStyle(StyleVar_TextColor, theme.TextDim);
            UILabelWrapped("##luahooks", Str8C((char*)(hooks[0] ? hooks : "none of the engine's callbacks")), 190.0f);
            UIPopStyle(StyleVar_TextColor);
            UIEndPropertyRow();
        }

        if(gotDrop && dropped[0])
        {
            script->ClearError();
            script->Load(dropped);
        }
        (void)dpi;
    }

    void RegisterLuaScriptInspector()
    {
        ToolPanels::RegisterComponent<LuaScriptComponent>("Lua Script", &DrawLuaScriptInspector, false, true);
    }
}
