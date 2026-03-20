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

        // Resolve virtual //Assets paths to physical paths
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

        auto loadFileResult = LuaManager::Get().GetState().safe_script_file(actualPath, *m_Env, sol::script_pass_on_error);
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

        LuaManager::Get().GetState().collect_garbage();
    }

    void LuaScriptComponent::OnScriptError(const char* funcName, const char* errorMsg)
    {
        LERROR("Failed to Execute Script Lua %s", funcName);
        LERROR("Error : %s", errorMsg);

        m_Errored = true;

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
}
