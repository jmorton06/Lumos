#include "LM.h"
#include "LuaScript.h"

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
}
