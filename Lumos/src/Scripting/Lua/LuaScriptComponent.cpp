#include "Precompiled.h"
#include "LuaScriptComponent.h"
#include "LuaManager.h"
#include "Scene/Scene.h"
#include "Core/Engine.h"

namespace Lumos
{
	LuaScriptComponent::LuaScriptComponent(const std::string& fileName, Scene* scene)
	{
		m_Scene = scene;
		m_FileName = fileName;

		Init();
	}

	void LuaScriptComponent::Init()
	{
		LoadScript(m_FileName);
	}

	LuaScriptComponent::~LuaScriptComponent()
	{
		if(m_Env && (*m_Env)["OnRelease"])
			(*m_Env)["OnRelease"]();
	}

	void LuaScriptComponent::LoadScript(const std::string& fileName)
	{
        m_FileName = fileName;
		std::string physicalPath;
		if(!VFS::Get()->ResolvePhysicalPath(fileName, physicalPath))
		{
			Debug::Log::Error("Failed to Load Lua script {0}", fileName);
			m_Env = nullptr;
			return;
		}

		m_Env = CreateRef<sol::environment>(LuaManager::Get().GetState(), sol::create, LuaManager::Get().GetState().globals());

		auto loadFileResult = LuaManager::Get().GetState().script_file(physicalPath, *m_Env, sol::script_pass_on_error);
		if(!loadFileResult.valid())
		{
			sol::error err = loadFileResult;
			Debug::Log::Error("Failed to Execute Lua script {0}", physicalPath);
			Debug::Log::Error("Error : {0}", err.what());
			m_Errors.push_back(std::string(err.what()));
		}

		if(m_Scene)
			(*m_Env)["CurrentScene"] = m_Scene;

		if((*m_Env)["OnInit"])
			m_OnInitFunc = CreateRef<sol::protected_function>((*m_Env)["OnInit"]);
		else
			m_OnInitFunc = nullptr;

		if((*m_Env)["OnUpdate"])
			m_UpdateFunc = CreateRef<sol::protected_function>((*m_Env)["OnUpdate"]);
		else
			m_UpdateFunc = nullptr;
        
        if((*m_Env)["OnCollision2DBegin"])
                m_Phys2DBeginFunc = CreateRef<sol::protected_function>((*m_Env)["OnCollision2DBegin"]);
            else
                m_Phys2DBeginFunc = nullptr;
        
        if((*m_Env)["OnCollision2DEnd"])
                m_Phys2DEndFunc = CreateRef<sol::protected_function>((*m_Env)["OnCollision2DEnd"]);
            else
                m_Phys2DEndFunc = nullptr;
        
        if((*m_Env)["OnCollision3DBegin"])
                m_Phys3DBeginFunc = CreateRef<sol::protected_function>((*m_Env)["OnCollision3DBegin"]);
            else
                m_Phys3DBeginFunc = nullptr;
        
        if((*m_Env)["OnCollision3DEnd"])
                m_Phys3DEndFunc = CreateRef<sol::protected_function>((*m_Env)["OnCollision3DEnd"]);
            else
                m_Phys3DEndFunc = nullptr;
    

		LuaManager::Get().GetState().collect_garbage();
    }

	void LuaScriptComponent::OnInit()
	{
		if(m_OnInitFunc)
		{
			sol::protected_function_result result = m_OnInitFunc->call();
			if(!result.valid())
			{
				sol::error err = result;
				Debug::Log::Error("Failed to Execute Script Lua Init function");
				Debug::Log::Error("Error : {0}", err.what());
			}
		}
	}

	void LuaScriptComponent::OnUpdate(float dt)
	{
		if(m_UpdateFunc)
		{
			sol::protected_function_result result = m_UpdateFunc->call(dt);
			if(!result.valid())
			{
				sol::error err = result;
				Debug::Log::Error("Failed to Execute Script Lua OnUpdate");
				Debug::Log::Error("Error : {0}", err.what());
			}
		}
	}

	void LuaScriptComponent::Reload()
	{
		if(m_Env && (*m_Env)["OnRelease"])
			(*m_Env)["OnRelease"]();

		Init();
	}

	void LuaScriptComponent::Load(const std::string& fileName)
	{
		if(m_Env && (*m_Env)["OnRelease"])
			(*m_Env)["OnRelease"]();

		m_FileName = fileName;
		Init();
	}

	void LuaScriptComponent::OnCollision2DBegin()
	{
        if(m_Phys2DBeginFunc)
        {
            sol::protected_function_result result = m_Phys2DBeginFunc->call();
            if(!result.valid())
            {
                sol::error err = result;
                Debug::Log::Error("Failed to Execute Script Lua OnCollision2DBegin");
                Debug::Log::Error("Error : {0}", err.what());
            }
        }
	}

	void LuaScriptComponent::OnCollision2DEnd()
	{
        if(m_Phys2DEndFunc)
        {
           sol::protected_function_result result = m_Phys2DEndFunc->call();
           if(!result.valid())
           {
               sol::error err = result;
               Debug::Log::Error("Failed to Execute Script Lua OnCollision2DEnd");
               Debug::Log::Error("Error : {0}", err.what());
           }
        }
	}

	void LuaScriptComponent::OnCollision3DBegin()
	{
        if(m_Phys3DBeginFunc)
        {
            sol::protected_function_result result = m_Phys3DBeginFunc->call();
            if(!result.valid())
            {
                sol::error err = result;
                Debug::Log::Error("Failed to Execute Script Lua OnCollision3DBegin");
                Debug::Log::Error("Error : {0}", err.what());
            }
        }
	}

	void LuaScriptComponent::OnCollision3DEnd()
	{
        if(m_Phys3DEndFunc)
        {
            sol::protected_function_result result = m_Phys3DEndFunc->call();
            if(!result.valid())
            {
                sol::error err = result;
                Debug::Log::Error("Failed to Execute Script Lua OnCollision3DEnd");
                Debug::Log::Error("Error : {0}", err.what());
            }
        }
	}
}
