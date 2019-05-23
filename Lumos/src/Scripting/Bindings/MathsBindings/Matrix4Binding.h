#pragma once
#include "LM.h"
#include "Scripting/LuaScript.h"
#include "Scripting/Luna.h"
#include "Maths/Matrix4.h"

namespace lumos
{
	class LUMOS_EXPORT Matrix4Binding
	{
	public:
		maths::Matrix4 matrix4;

		static const char className[];
		static Luna<Matrix4Binding>::FunctionType methods[];
		static Luna<Matrix4Binding>::PropertyType properties[];

		Matrix4Binding(const maths::Matrix4& matrix);
		Matrix4Binding(lua_State* L);
		~Matrix4Binding();

		int GetRow(lua_State* L);

		int Translation(lua_State* L);
		int Rotation(lua_State* L);
		int RotationX(lua_State* L);
		int RotationY(lua_State* L);
		int RotationZ(lua_State* L);
		int RotationQuaternion(lua_State* L);
		int Scale(lua_State* L);
		int LookTo(lua_State* L);
		int LookAt(lua_State* L);

		int Multiply(lua_State* L);
		int Add(lua_State* L);
		int Transpose(lua_State* L);
		int Inverse(lua_State* L);
		int Print(lua_State* L);

		static void Bind();
	};
}


