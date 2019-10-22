#include "lmpch.h"
#include "LuaScript.h"
#include "LuaManager.h"

#include "App/Scene.h"
#include "ECS/EntityManager.h"

#include "App/Engine.h"
namespace Lumos
{
	LuaScript::LuaScript(const String& fileName)
	{
		m_FileName = fileName;
	}

	void LuaScript::Init(Entity* entity)
	{
		m_Entity = entity;
		LoadScript(m_FileName);
	}

	void LuaScript::Init(Scene* scene)
    {
		m_Scene = scene;
		LoadScript(m_FileName);
    }

	LuaScript::~LuaScript()
	{
	}

	void LuaScript::LoadScript(const std::string& fileName)
	{
		sol::load_result loadFileResult = LuaManager::Instance()->GetState()->load_file(fileName);
		if (!loadFileResult.valid()) {
			sol::error err = loadFileResult;
			std::cerr << "failed to load file-based script::" << fileName << "::" << err.what() << std::endl;
		}
		sol::protected_function_result res = loadFileResult();
		if (!res.valid()) {
			sol::error err = res;
			std::cerr << "failed to Execture file-based script::" << fileName << "::" << err.what() << std::endl;
		}
		sol::table FunctionsTable = res;

		if (m_Scene)
			FunctionsTable["scene"] = m_Scene;

		if (m_Entity)
			FunctionsTable["entity"] = m_Entity;

		res = FunctionsTable["Init"]();
		if (!res.valid())
		{
			sol::error err = res;
			std::cerr << "failed to Execute Init in script::" << fileName << "::" << err.what() << std::endl;
		}
		m_LuaTables.push_back(FunctionsTable);
	}

	void LuaScript::Update()
	{
		float dt = Engine::Instance()->GetTimeStep()->GetElapsedMillis();
		for (size_t i = 0; i < m_LuaTables.size(); i++)
		{
			m_LuaTables[i]["Update"](dt);
		}
	}
    
}
