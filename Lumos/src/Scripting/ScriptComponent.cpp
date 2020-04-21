#include "lmpch.h"
#include "ScriptComponent.h"
#include "LuaManager.h"
#include "App/Scene.h"
#include "App/Engine.h"

namespace Lumos
{
    ScriptComponent::ScriptComponent(const String& fileName, Scene* scene)
    {
        m_Scene = scene;
        m_FileName = fileName;
        
        Init();
    }

    void ScriptComponent::Init()
    {
        LoadScript(m_FileName);
    }

    ScriptComponent::~ScriptComponent()
    {
        if(m_Env["OnRelease"])
            m_Env["OnRelease"]();
    }

    void ScriptComponent::LoadScript(const std::string& fileName)
    {
        m_Env = sol::environment(*LuaManager::Instance()->GetState(), sol::create, LuaManager::Instance()->GetState()->globals());
        LuaManager::Instance()->GetState()->script_file(fileName, m_Env);
        
        if(m_Scene)
            m_Env["CurrentScene"] = m_Scene;
        
        if(m_Env["OnInit"])
            m_Env["OnInit"]();

    }

    void ScriptComponent::Update(float dt)
    {
        if(m_Env["OnUpdate"])
            m_Env["OnUpdate"](dt);
    }

    void ScriptComponent::Reload()
    {
        if(m_Env["OnRelease"])
            m_Env["OnRelease"]();
        
        Init();
    }

    void ScriptComponent::Load(const String& fileName)
    {
        if(m_Env["OnRelease"])
            m_Env["OnRelease"]();
        
        m_FileName = fileName;
        Init();
    }
}
