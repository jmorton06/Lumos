#include "lmpch.h"
#include "LuaManager.h"
#include "MathsLuaBind.h"
#include "ECSLuaBind.h"
#include "ImGuiLuaBind.h"
#include "Core/OS/Window.h"
#include <sol/sol.hpp>

namespace Lumos
{
	LuaManager::LuaManager() : m_State(nullptr)
	{
	}

	void LuaManager::OnInit()
	{
		m_State = lmnew sol::state();
		m_State->open_libraries(sol::lib::base, sol::lib::package, sol::lib::math);

		Scripting::BindMathsLua(m_State);
		Scripting::BindImGuiLua(m_State);
		Scripting::BindECSLua(m_State);

		m_State->script(R"(print('[LUA] - Initialised Lua!')
			local vec1 = Vector4.new(1.0, 0.0, 0.0, 1.0)
			local vec2 = Vector4.new(1.0, 0.0, 1.0, 0.0)
			local vector2 = Vector2.new(0.0, 1.0)
			local vector3 = Vector3.new(0.0, 2.0, 4.0)
			local testAdd = vec1 + vec2
			local quatTest = Quaternion.new(90.0,0.0,0.0)
			local eulerTest = quatTest:ToEuler()
			local dotTest = vector3:Dot(vector3)
			
			print(dotTest)

			print(quatTest)
			print(eulerTest)
			print(testAdd)
			function Update(dt)
				print(dt)
			end
			)"
		);

		sol::function fn = (*m_State)["Update"];
		fn.call(0.16f);
	}

	LuaManager::~LuaManager()
	{
		lmdel m_State;
	}

	WindowProperties LuaManager::LoadConfigFile(const String& file)
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
