#include "LM.h"
#include "LuaScript.h"
#include "App/Window.h"
#include <sol/sol.hpp>

namespace lumos
{
	LuaScript::LuaScript()
	{
	}
    
    void LuaScript::OnInit()
    {
        m_State = new sol::state();
        m_State->open_libraries(sol::lib::base, sol::lib::package);
        
        m_State->script("print('[LUA] - Initialised Lua!')");
    }

	LuaScript::~LuaScript()
	{
        delete m_State;
	}
    
    WindowProperties* LuaScript::LoadConfigFile(const String& file)
    {
        WindowProperties* windowProperties = new WindowProperties();
        
        m_State->script_file(ROOT_DIR"/Sandbox/Settings.lua");
        windowProperties->Title = m_State->get<std::string>("title");
        windowProperties->Width = m_State->get<int>("width");
        windowProperties->Height = m_State->get<int>("height");
        
        return windowProperties;
    }
}
