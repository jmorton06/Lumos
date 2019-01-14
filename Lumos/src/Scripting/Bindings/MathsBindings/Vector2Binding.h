#pragma once
#include "LM.h"
#include "Scripting/LuaScript.h"
#include "Scripting/Luna.h"
#include "Maths/Vector2.h"

namespace Lumos
{
	class LUMOS_EXPORT Vector2Binding
	{
	public:
		maths::Vector2 vec2;

		static const char className[];
		static Luna<Vector2Binding>::FunctionType methods[];
		static Luna<Vector2Binding>::PropertyType properties[];

		Vector2Binding(const maths::Vector2& vector);
		Vector2Binding(lua_State* L);
		~Vector2Binding();

		int GetX(lua_State* L);
		int GetY(lua_State* L);

		int SetX(lua_State* L);
		int SetY(lua_State* L);

		int Transform(lua_State* L);
		int Length(lua_State* L);
		int Normalize(lua_State* L);
		int QuaternionNormalize(lua_State* L);
		int Clamp(lua_State* L);
		int Saturate(lua_State* L);

		int Dot(lua_State* L);
		int Cross(lua_State* L);
		int Multiply(lua_State* L);
		int Add(lua_State* L);
		int Subtract(lua_State* L);
		int Lerp(lua_State* L);
		int Print(lua_State* L);

		int QuaternionMultiply(lua_State* L);
		int QuaternionFromRollPitchYaw(lua_State* L);
		int Slerp(lua_State* L);

		static void Bind();
	};
}


