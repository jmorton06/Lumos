#pragma once

#include "LM.h"  
#include "lua.hpp"

namespace Lumos
{

typedef int(*lua_CFunction) (lua_State *L);

class LUMOS_EXPORT LuaScripting
{
private:
	lua_State * m_luaState;
	int m_status{}; 
	static LuaScripting* g_Lua;
	static int DebugOut(lua_State *L);
	bool RunScript();
public:
	LuaScripting();
	~LuaScripting();

	inline lua_State* GetLuaState() const { return m_luaState; }

	static LuaScripting* GetGlobal();
	bool Success();
	bool Failed();
	std::string GetErrorMsg();
	std::string PopErrorMsg() const;
	void PostErrorMsg(bool todebug = true, bool tobacklog = true);
	bool RunFile(const std::string& filename);
	bool RunText(const std::string& script);
	bool RegisterFunc(const std::string& name, lua_CFunction function);
	void RegisterLibrary(const std::string& tableName, const luaL_Reg* functions);
	bool RegisterObject(const std::string& tableName, const std::string& name, void* object);
	void AddFunc(const std::string& name, lua_CFunction function) const;
	void AddFuncArray(const luaL_Reg* functions) const;
	void AddInt(const std::string& name, int data) const;

	void SetDeltaTime(double dt);
	void FixedUpdate();
	void Update();
	void Render();

	//send a signal to lua
	void Signal(const std::string& name) const;
	bool TrySignal(const std::string& name) const;
	void KillProcesses();

	//Static function wrappers
	static std::string SGetString(lua_State* L, int stackpos);
	static bool SIsString(lua_State* L, int stackpos);
	static bool SIsNumber(lua_State* L, int stackpos);
	static int SGetInt(lua_State* L, int stackpos);
	static long SGetLong(lua_State* L, int stackpos);
	static long long SGetLongLong(lua_State* L, int stackpos);
	static float SGetFloat(lua_State* L, int stackpos);
	static double SGetDouble(lua_State* L, int stackpos);
	static bool SGetBool(lua_State* L, int stackpos);
	static int SGetArgCount(lua_State* L);
	static void* SGetUserData(lua_State* L);

	static void SSetInt(lua_State* L, int data);
	static void SSetFloat(lua_State* L, float data);
	static void SSetDouble(lua_State* L, double data);
	static void SSetString(lua_State* L, const std::string& data);
	static void SSetBool(lua_State* L, bool data);
	static void SSetPointer(lua_State* L, void* data);
	static void SSetNull(lua_State* L);

	//throw error
	static void SError(lua_State* L, const std::string& error = "", bool todebug = true, bool tobacklog = true);
	static void SAddMetatable(lua_State* L, const std::string& name);
};
}
