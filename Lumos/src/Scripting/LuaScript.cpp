#include "lmpch.h"
#include "LuaScript.h"
#include "Core/OS/Window.h"
#include <sol/sol.hpp>

namespace Lumos
{
	LuaScript::LuaScript() : m_State(nullptr)
	{
	}
    
    void LuaScript::OnInit()
    {
        m_State = lmnew sol::state();
        m_State->open_libraries(sol::lib::base, sol::lib::package);
        
        m_State->script("print('[LUA] - Initialised Lua!')");
    }

	LuaScript::~LuaScript()
	{
        delete m_State;
	}
    
    WindowProperties LuaScript::LoadConfigFile(const String& file)
    {
        WindowProperties windowProperties;
        
        m_State->script_file(file);
        windowProperties.Title = m_State->get<std::string>("title");
        windowProperties.Width = m_State->get<int>("width");
        windowProperties.Height = m_State->get<int>("height");
        windowProperties.RenderAPI = m_State->get<int>("renderAPI");
        
        return windowProperties;
    }
}
