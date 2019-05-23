#include "LM.h"
#include "LuaScript.h"
#include "LuaGlobals.h"

#include "Bindings/LuaBindings.h"
#include <mutex>

namespace lumos
{
	LuaScripting* LuaScripting::g_Lua = nullptr;

#define WILUA_ERROR_PREFIX "[Lua Error] "

	LuaScripting::LuaScripting()
	{
		m_luaState = NULL;
		m_luaState = luaL_newstate();
		luaL_openlibs(m_luaState);
		RegisterFunc("debugout", DebugOut);
		RunText(LuaGlobals);
	}

	LuaScripting::~LuaScripting()
	{
		lua_close(m_luaState);
	}

	LuaScripting* LuaScripting::GetGlobal()
	{
		if (g_Lua == nullptr)
		{
			g_Lua = new LuaScripting();

			lumos::luabindings::BindAll();
		}
		return g_Lua;
	}

	bool LuaScripting::Success()
	{
		return m_status == 0;
	}
	bool LuaScripting::Failed()
	{
		return m_status != 0;
	}
	std::string LuaScripting::GetErrorMsg()
	{
		if (Failed()) 
		{
			std::string retVal = lua_tostring(m_luaState, -1);
			return retVal;
		}
		return std::string("");
	}

	std::string LuaScripting::PopErrorMsg() const
	{
		std::string retVal = lua_tostring(m_luaState, -1);
		lua_pop(m_luaState, 1); // remove error message
		return retVal;
	}
	void LuaScripting::PostErrorMsg(bool todebug, bool tobacklog)
	{
		if (Failed())
		{
			const char* str = lua_tostring(m_luaState, -1);

			if (str == nullptr)
				return;
			std::stringstream ss("");
			ss << WILUA_ERROR_PREFIX << str;

			if (tobacklog)
			{
				LUMOS_CORE_INFO(ss.str().c_str());
			}
			if (todebug)
			{
				ss << std::endl;
				LUMOS_CORE_INFO(ss.str().c_str());
			}

			lua_pop(m_luaState, 1); // remove error message
		}
	}
	bool LuaScripting::RunFile(const std::string& filename)
	{
		m_status = luaL_loadfile(m_luaState, filename.c_str());

		if (Success()) 
		{
			return RunScript();
		}

		PostErrorMsg();
		return false;
	}
	bool LuaScripting::RunText(const std::string& script)
	{
		m_status = luaL_loadstring(m_luaState, script.c_str());
		
		if (Success())
		{
			return RunScript();
		}

		PostErrorMsg();
		return false;
	}
	bool LuaScripting::RunScript()
	{
		m_status = lua_pcall(m_luaState, 0, LUA_MULTRET, 0);
	
		if (Failed())
		{
			PostErrorMsg();
			return false;
		}
		return true;
	}
	bool LuaScripting::RegisterFunc(const std::string& name, lua_CFunction function)
	{
		lua_register(m_luaState, name.c_str(), function);

		PostErrorMsg();

		return Success();
	}
	void LuaScripting::RegisterLibrary(const std::string& tableName, const luaL_Reg* functions)
	{
		if (luaL_newmetatable(m_luaState, tableName.c_str()) != 0)
		{
			//table is not yet present
			lua_pushvalue(m_luaState, -1);
			lua_setfield(m_luaState, -2, "__index"); // Object.__index = Object
		
			AddFuncArray(functions);
		}
	}
	bool LuaScripting::RegisterObject(const std::string& tableName, const std::string& name, void* object)
	{
		RegisterLibrary(tableName, nullptr);

		// does this call need to be checked? eg. userData == nullptr?
		const auto userData = static_cast<void**>(lua_newuserdata(m_luaState, sizeof(void*)));
		*(userData) = object;

		luaL_setmetatable(m_luaState, tableName.c_str());
		lua_setglobal(m_luaState, name.c_str());

		return true;
	}
	void LuaScripting::AddFunc(const std::string& name, lua_CFunction function) const
	{
		lua_pushcfunction(m_luaState, function);
		lua_setfield(m_luaState, -2, name.c_str());
	}
	void LuaScripting::AddFuncArray(const luaL_Reg* functions) const
	{
		if (functions != nullptr)
		{
			luaL_setfuncs(m_luaState, functions, 0);
		}
	}
	void LuaScripting::AddInt(const std::string& name, int data) const
	{
		lua_pushinteger(m_luaState, data);
		lua_setfield(m_luaState, -2, name.c_str());
	}

	void LuaScripting::SetDeltaTime(double dt)
	{
		lua_getglobal(m_luaState, "setDeltaTime");
		SSetDouble(m_luaState, dt);
		lua_call(m_luaState, 1, 0);
	}
	void LuaScripting::FixedUpdate()
	{
		TrySignal("LumosEngine_fixed_update_tick");
	}
	void LuaScripting::Update()
	{
		TrySignal("LumosEngine_update_tick");
	}
	void LuaScripting::Render()
	{
		TrySignal("LumosEngine_render_tick");
	}

	inline void SignalHelper(lua_State* L, const std::string& name)
	{
		lua_getglobal(L, "signal");
		LuaScripting::SSetString(L, name);
		lua_call(L, 1, 0);
	}
	void LuaScripting::Signal(const std::string& name) const
	{
		SignalHelper(m_luaState, name);
	}
	bool LuaScripting::TrySignal(const std::string& name) const
	{
		SignalHelper(m_luaState, name);
		return true;
	}

	void LuaScripting::KillProcesses()
	{
		RunText("killProcesses();");
	}

	int LuaScripting::DebugOut(lua_State* L)
	{
		int argc = lua_gettop(L);

		std::stringstream ss("");

		for (int i = 1; i <= argc; i++)
		{
			static std::mutex sm;
			sm.lock();
			const char* str = lua_tostring(L, i);
			sm.unlock();
			if (str != nullptr)
			{
				ss << str;
			}
		}
		ss << std::endl;

		LUMOS_CORE_INFO(ss.str().c_str());

		//number of results
		return 0;
	}

	std::string LuaScripting::SGetString(lua_State* L, int stackpos)
	{
		const auto str = lua_tostring(L, stackpos);
		if (str != nullptr)
			return str;
		return std::string("");
	}
	bool LuaScripting::SIsString(lua_State* L, int stackpos)
	{
		return lua_isstring(L, stackpos) != 0;
	}
	bool LuaScripting::SIsNumber(lua_State* L, int stackpos)
	{
		return lua_isnumber(L, stackpos) != 0;
	}
	int LuaScripting::SGetInt(lua_State* L, int stackpos)
	{
		return static_cast<int>(SGetLongLong(L, stackpos));
	}
	long LuaScripting::SGetLong(lua_State* L, int stackpos)
	{
		return static_cast<long>(SGetLongLong(L, stackpos));
	}
	long long LuaScripting::SGetLongLong(lua_State* L, int stackpos)
	{
		return lua_tointeger(L, stackpos);
	}
	float LuaScripting::SGetFloat(lua_State* L, int stackpos)
	{
		return static_cast<float>(SGetDouble(L, stackpos));
	}

	double LuaScripting::SGetDouble(lua_State* L, int stackpos)
	{
		return lua_tonumber(L, stackpos);
	}
	bool LuaScripting::SGetBool(lua_State* L, int stackpos)
	{
		return lua_toboolean(L, stackpos) != 0;
	}
	int LuaScripting::SGetArgCount(lua_State* L)
	{
		return lua_gettop(L);
	}
	void* LuaScripting::SGetUserData(lua_State* L)
	{
		return lua_touserdata(L, 1);
	}

	void LuaScripting::SSetInt(lua_State* L, int data)
	{
		lua_pushinteger(L, (lua_Integer)data);
	}
	void LuaScripting::SSetFloat(lua_State* L, float data)
	{
		lua_pushnumber(L, (lua_Number)data);
	}

	void LuaScripting::SSetDouble(lua_State* L, double data)
	{
		lua_pushnumber(L, (lua_Number)data);
	}
	void LuaScripting::SSetString(lua_State* L, const std::string& data)
	{
		lua_pushstring(L, data.c_str());
	}
	void LuaScripting::SSetBool(lua_State* L, bool data)
	{
		lua_pushboolean(L, static_cast<int>(data));
	}
	void LuaScripting::SSetPointer(lua_State* L, void* data)
	{
		lua_pushlightuserdata(L, data);
	}
	void LuaScripting::SSetNull(lua_State* L)
	{
		lua_pushnil(L);
	}

	void LuaScripting::SError(lua_State* L, const std::string& error, bool todebug, bool tobacklog)
	{
		//retrieve line number for error info
		lua_Debug ar;
		lua_getstack(L, 1, &ar);
		lua_getinfo(L, "nSl", &ar);
		int line = ar.currentline;

		std::stringstream ss("");
		ss << WILUA_ERROR_PREFIX << "Line " << line << ": ";
		if (!error.empty())
		{
			ss << error;
		}
		if (tobacklog)
		{
			LUMOS_CORE_INFO(ss.str().c_str());
		}
	}

	void LuaScripting::SAddMetatable(lua_State* L, const std::string& name)
	{
		luaL_newmetatable(L, name.c_str());
	}
}
