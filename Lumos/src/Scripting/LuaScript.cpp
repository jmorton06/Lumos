#include "lmpch.h"
#include "LuaScript.h"
#include "Maths/Maths.h"
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
        
		m_State->new_usertype<Maths::Vector2>("Vector2",
			sol::constructors<sol::types<>, sol::types<float, float>>(),
			"x", &Maths::Vector2::x,
			"y", &Maths::Vector2::y,
			sol::meta_function::addition, sol::overload(
				static_cast<Maths::Vector2(Maths::Vector2::*)(float) const>(&Maths::Vector2::operator+),
				static_cast<Maths::Vector2(Maths::Vector2::*)(const Maths::Vector2&) const>(&Maths::Vector2::operator+)
			),
			sol::meta_function::multiplication, sol::overload(
				static_cast<Maths::Vector2(Maths::Vector2::*)(float) const>(&Maths::Vector2::operator*),
				static_cast<Maths::Vector2(Maths::Vector2::*)(const Maths::Vector2&) const>(&Maths::Vector2::operator*)
			),
			sol::meta_function::subtraction, sol::overload(
				static_cast<Maths::Vector2(Maths::Vector2::*)(float) const>(&Maths::Vector2::operator-),
				static_cast<Maths::Vector2(Maths::Vector2::*)(const Maths::Vector2&) const>(&Maths::Vector2::operator-)
			),
			sol::meta_function::division, sol::overload(
				static_cast<Maths::Vector2(Maths::Vector2::*)(float) const>(&Maths::Vector2::operator/),
				static_cast<Maths::Vector2(Maths::Vector2::*)(const Maths::Vector2&) const>(&Maths::Vector2::operator/)
			),
			sol::meta_function::equal_to, &Maths::Vector2::operator==
			);

		m_State->new_usertype<Maths::Vector3>("Vector3",
			sol::constructors<sol::types<>, sol::types<float, float, float>>(),
			"x", &Maths::Vector3::x,
			"y", &Maths::Vector3::y,
			"z", &Maths::Vector3::z,

			sol::meta_function::addition, sol::overload(
				static_cast<Maths::Vector3(Maths::Vector3::*)(float) const>(&Maths::Vector3::operator+),
				static_cast<Maths::Vector3(Maths::Vector3::*)(const Maths::Vector3&) const>(&Maths::Vector3::operator+)
			),
			sol::meta_function::multiplication, sol::overload(
				static_cast<Maths::Vector3(Maths::Vector3::*)(float) const>(&Maths::Vector3::operator*),
				static_cast<Maths::Vector3(Maths::Vector3::*)(const Maths::Vector3&) const>(&Maths::Vector3::operator*)
			),
			sol::meta_function::subtraction, sol::overload(
				static_cast<Maths::Vector3(Maths::Vector3::*)(float) const>(&Maths::Vector3::operator-),
				static_cast<Maths::Vector3(Maths::Vector3::*)(const Maths::Vector3&) const>(&Maths::Vector3::operator-)
			),
			sol::meta_function::division, sol::overload(
				static_cast<Maths::Vector3(Maths::Vector3::*)(float) const>(&Maths::Vector3::operator/),
				static_cast<Maths::Vector3(Maths::Vector3::*)(const Maths::Vector3&) const>(&Maths::Vector3::operator/)
			),
			sol::meta_function::equal_to, &Maths::Vector3::operator==
			);

		m_State->new_usertype<Maths::Vector4>("Vector4",
			sol::constructors<Maths::Vector4(), Maths::Vector4(float, float, float, float)>(),
			"x", &Maths::Vector4::x,
			"y", &Maths::Vector4::y,
			"z", &Maths::Vector4::z,
			"w", &Maths::Vector4::w,
		
			sol::meta_function::addition, sol::overload(
				static_cast<Maths::Vector4(Maths::Vector4::*)(float) const>(&Maths::Vector4::operator+),
				static_cast<Maths::Vector4(Maths::Vector4::*)(const Maths::Vector4&) const>(&Maths::Vector4::operator+)
			),
			sol::meta_function::multiplication, sol::overload(
				static_cast<Maths::Vector4(Maths::Vector4::*)(float) const>(&Maths::Vector4::operator*),
				static_cast<Maths::Vector4(Maths::Vector4::*)(const Maths::Vector4&) const>(&Maths::Vector4::operator*)
			),
			sol::meta_function::subtraction, sol::overload(
				static_cast<Maths::Vector4(Maths::Vector4::*)(float) const>(&Maths::Vector4::operator-),
				static_cast<Maths::Vector4(Maths::Vector4::*)(const Maths::Vector4&) const>(&Maths::Vector4::operator-)
			),
			sol::meta_function::division, sol::overload(
				static_cast<Maths::Vector4(Maths::Vector4::*)(float) const>(&Maths::Vector4::operator/),
				static_cast<Maths::Vector4(Maths::Vector4::*)(const Maths::Vector4&) const>(&Maths::Vector4::operator/)
			),
			sol::meta_function::equal_to, &Maths::Vector4::operator==
			);

        m_State->script(R"(print('[LUA] - Initialised Lua!')
			local vec1 = Vector4.new(1.0, 0.0, 0.0, 1.0)
			local vec2 = Vector4.new(0.0, 0.0, 1.0, 0.0)
			local vector2 = Vector2.new(0.0, 1.0)
			local vector3 = Vector3.new(0.0, 2.0, 4.0)

			function Update(dt)
				print(dt)
			end
			)"
		);

		sol::function fn = (*m_State)["Update"];
		fn.call(0.16f);
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
