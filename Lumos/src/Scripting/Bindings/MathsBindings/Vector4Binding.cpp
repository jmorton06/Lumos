#include "LM.h"
#include "Vector4Binding.h"
#include "Scripting/LuaScript.h"

namespace lumos
{
	const char Vector4Binding::className[] = "Vector4";

	Luna<Vector4Binding>::FunctionType Vector4Binding::methods[] = {
		lunamethod(Vector4Binding, GetX),
		lunamethod(Vector4Binding, GetY),
		lunamethod(Vector4Binding, GetZ),
		lunamethod(Vector4Binding, GetW),
		lunamethod(Vector4Binding, SetX),
		lunamethod(Vector4Binding, SetY),
		lunamethod(Vector4Binding, SetZ),
		lunamethod(Vector4Binding, SetW),
		lunamethod(Vector4Binding, Transform),
		lunamethod(Vector4Binding, Length),
		lunamethod(Vector4Binding, Normalize),
		lunamethod(Vector4Binding, QuaternionNormalize),
		lunamethod(Vector4Binding, Add),
		lunamethod(Vector4Binding, Subtract),
		lunamethod(Vector4Binding, Multiply),
		lunamethod(Vector4Binding, Dot),
		lunamethod(Vector4Binding, Cross),
		lunamethod(Vector4Binding, Lerp),
		lunamethod(Vector4Binding, Slerp),
		lunamethod(Vector4Binding, Clamp),
		lunamethod(Vector4Binding, Normalize),
		lunamethod(Vector4Binding, QuaternionMultiply),
		lunamethod(Vector4Binding, QuaternionFromRollPitchYaw),
		lunamethod(Vector4Binding, Print),
	{ NULL, NULL }
	};
	Luna<Vector4Binding>::PropertyType Vector4Binding::properties[] = {
		{ NULL, NULL }
	};

	Vector4Binding::Vector4Binding(const maths::Vector4& vec4) : vec4(vec4)
	{
	}

	Vector4Binding::Vector4Binding(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		float x = 0.f, y = 0.f, z = 0.f, w = 0.f;

		if (argc > 0)
		{
			x = LuaScripting::SGetFloat(L, 1);
			if (argc > 1)
			{
				y = LuaScripting::SGetFloat(L, 2);
				if (argc > 2)
				{
					z = LuaScripting::SGetFloat(L, 3);
					if (argc > 3)
					{
						w = LuaScripting::SGetFloat(L, 4);
					}
				}
			}
		}
		vec4 = maths::Vector4(x, y, z, w);
	}

	Vector4Binding::~Vector4Binding()
	{
	}


	int Vector4Binding::GetX(lua_State* L)
	{
		LuaScripting::SSetFloat(L, vec4.GetX());
		return 1;
	}
	int Vector4Binding::GetY(lua_State* L)
	{
		LuaScripting::SSetFloat(L, vec4.GetY());
		return 1;
	}
	int Vector4Binding::GetZ(lua_State* L)
	{
		LuaScripting::SSetFloat(L, vec4.GetZ());
		return 1;
	}
	int Vector4Binding::GetW(lua_State* L)
	{
		LuaScripting::SSetFloat(L, vec4.GetW());
		return 1;
	}

	int Vector4Binding::SetX(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 0)
		{
			vec4.SetX(LuaScripting::SGetFloat(L, 1));
		}
		else
			LuaScripting::SError(L, "SetX(float value) not enough arguments!");
		return 0;
	}
	int Vector4Binding::SetY(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 0)
		{
			vec4.SetY(LuaScripting::SGetFloat(L, 1));
		}
		else
			LuaScripting::SError(L, "SetY(float value) not enough arguments!");
		return 0;
	}
	int Vector4Binding::SetZ(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 0)
		{
			vec4.SetZ(LuaScripting::SGetFloat(L, 1));
		}
		else
			LuaScripting::SError(L, "SetZ(float value) not enough arguments!");
		return 0;
	}
	int Vector4Binding::SetW(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 0)
		{
			vec4.SetW(LuaScripting::SGetFloat(L, 1));
		}
		else
			LuaScripting::SError(L, "SetW(float value) not enough arguments!");
		return 0;
	}

	int Vector4Binding::Transform(lua_State* L)
	{
		/*int argc = LuaScripting::SGetArgCount(L);
		if (argc > 0)
		{
			Matrix_BindLua* mat = Luna<Matrix_BindLua>::lightcheck(L, 1);
			if (mat)
			{
				Luna<Vector4Binding>::push(L, new Vector4Binding(XMVector4Transform(vec4, mat->matrix)));
				return 1;
			}
			else
				LuaScripting::SError(L, "Transform(Matrix matrix) argument is not a Matrix!");
		}
		else
			LuaScripting::SError(L, "Transform(Matrix matrix) not enough arguments!");*/
		return 0;
	}
	int Vector4Binding::Length(lua_State* L)
	{
		LuaScripting::SSetFloat(L, vec4.Length());
		return 1;
	}
	int Vector4Binding::Normalize(lua_State* L)
	{
		Luna<Vector4Binding>::push(L, new Vector4Binding(vec4.Normal()));
		return 1;
	}
	int Vector4Binding::QuaternionNormalize(lua_State* L)
	{
		//Luna<Vector4Binding>::push(L, new Vector4Binding((vec4)));
		return 1;
	}
	int Vector4Binding::Clamp(lua_State* L)
	{
		/*int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			float a = LuaScripting::SGetFloat(L, 1);
			float b = LuaScripting::SGetFloat(L, 2);
			Luna<Vector4Binding>::push(L, new Vector4Binding((vec4, (a, a, a, a), (b, b, b, b))));
			return 1;
		}
		else
			LuaScripting::SError(L, "Clamp(float min,max) not enough arguments!");*/
		return 0;
	}
	int Vector4Binding::Saturate(lua_State* L)
	{
		//Luna<Vector4Binding>::push(L, new Vector4Binding((vec4)));
		return 1;
	}



	int Vector4Binding::Dot(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			Vector4Binding* v1 = Luna<Vector4Binding>::lightcheck(L, 1);
			Vector4Binding* v2 = Luna<Vector4Binding>::lightcheck(L, 2);
			if (v1 && v2)
			{
				LuaScripting::SSetFloat(L, v1->vec4.Dot( v2->vec4));
				return 1;
			}
		}
		LuaScripting::SError(L, "Dot(vec4 v1,v2) not enough arguments!");
		return 0;
	}
	int Vector4Binding::Cross(lua_State* L)
	{
		/*int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			Vector4Binding* v1 = Luna<Vector4Binding>::lightcheck(L, 1);
			Vector4Binding* v2 = Luna<Vector4Binding>::lightcheck(L, 2);
			if (v1 && v2)
			{
				Luna<Vector4Binding>::push(L, new Vector4Binding(v1->vec4.(v2->vec4));
				return 1;
			}
		}*/
		LuaScripting::SError(L, "Cross(vec4 v1,v2) not enough arguments!");
		return 0;
	}
	int Vector4Binding::Multiply(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			Vector4Binding* v1 = Luna<Vector4Binding>::lightcheck(L, 1);
			Vector4Binding* v2 = Luna<Vector4Binding>::lightcheck(L, 2);
			if (v1 && v2)
			{
				Luna<Vector4Binding>::push(L, new Vector4Binding(v1->vec4 * v2->vec4));
				return 1;
			}
			else if (v1)
			{
				Luna<Vector4Binding>::push(L, new Vector4Binding(v1->vec4 * LuaScripting::SGetFloat(L, 2)));
				return 1;
			}
			else if (v2)
			{
				Luna<Vector4Binding>::push(L, new Vector4Binding(v2->vec4 * LuaScripting::SGetFloat(L, 1)));
				return 1;
			}
		}
		LuaScripting::SError(L, "Multiply(vec4 v1,v2) not enough arguments!");
		return 0;
	}
	int Vector4Binding::Add(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			Vector4Binding* v1 = Luna<Vector4Binding>::lightcheck(L, 1);
			Vector4Binding* v2 = Luna<Vector4Binding>::lightcheck(L, 2);
			if (v1 && v2)
			{
				Luna<Vector4Binding>::push(L, new Vector4Binding(v1->vec4 + v2->vec4));
				return 1;
			}
		}
		LuaScripting::SError(L, "Add(vec4 v1,v2) not enough arguments!");
		return 0;
	}
	int Vector4Binding::Subtract(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			Vector4Binding* v1 = Luna<Vector4Binding>::lightcheck(L, 1);
			Vector4Binding* v2 = Luna<Vector4Binding>::lightcheck(L, 2);
			if (v1 && v2)
			{
				Luna<Vector4Binding>::push(L, new Vector4Binding(v1->vec4 - v2->vec4));
				return 1;
			}
		}
		LuaScripting::SError(L, "Subtract(vec4 v1,v2) not enough arguments!");
		return 0;
	}
	int Vector4Binding::Lerp(lua_State* L)
	{
		/*int argc = LuaScripting::SGetArgCount(L);
		if (argc > 2)
		{
			Vector4Binding* v1 = Luna<Vector4Binding>::lightcheck(L, 1);
			Vector4Binding* v2 = Luna<Vector4Binding>::lightcheck(L, 2);
			float t = LuaScripting::SGetFloat(L, 3);
			if (v1 && v2)
			{
				Luna<Vector4Binding>::push(L, new Vector4Binding(XMVectorLerp(v1->vec4, v2->vec4, t)));
				return 1;
			}
		}*/
		LuaScripting::SError(L, "Lerp(vec4 v1,v2, float t) not enough arguments!");
		return 0;
	}


	int Vector4Binding::QuaternionMultiply(lua_State* L)
	{/*
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			Vector4Binding* v1 = Luna<Vector4Binding>::lightcheck(L, 1);
			Vector4Binding* v2 = Luna<Vector4Binding>::lightcheck(L, 2);
			if (v1 && v2)
			{
				Luna<Vector4Binding>::push(L, new Vector4Binding(XMQuaternionMultiply(v1->vec4, v2->vec4)));
				return 1;
			}
		}
		LuaScripting::SError(L, "QuaternionMultiply(vec4 v1,v2) not enough arguments!");*/
		return 0;
	}
	int Vector4Binding::QuaternionFromRollPitchYaw(lua_State* L)
	{
		/*int argc = LuaScripting::SGetArgCount(L);
		if (argc > 0)
		{
			Vector4Binding* v1 = Luna<Vector4Binding>::lightcheck(L, 1);
			if (v1)
			{
				Luna<Vector4Binding>::push(L, new Vector4Binding(XMQuaternionRotationRollPitchYawFromVector(v1->vec4)));
				return 1;
			}
		}
		LuaScripting::SError(L, "QuaternionFromRollPitchYaw(vec4 rotXYZ) not enough arguments!");*/
		return 0;
	}
	int Vector4Binding::Slerp(lua_State* L)
	{
		/*int argc = LuaScripting::SGetArgCount(L);
		if (argc > 2)
		{
			Vector4Binding* v1 = Luna<Vector4Binding>::lightcheck(L, 1);
			Vector4Binding* v2 = Luna<Vector4Binding>::lightcheck(L, 2);
			float t = LuaScripting::SGetFloat(L, 3);
			if (v1 && v2)
			{
				Luna<Vector4Binding>::push(L, new Vector4Binding(XMQuaternionSlerp(v1->vec4, v2->vec4, t)));
				return 1;
			}
		}
		LuaScripting::SError(L, "QuaternionSlerp(vec4 v1,v2, float t) not enough arguments!");*/
		return 0;
	}


	void Vector4Binding::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<Vector4Binding>::Register(LuaScripting::GetGlobal()->GetLuaState());
			LuaScripting::GetGlobal()->RunText("vec4 = Vector4()");
		}
	}

	int Vector4Binding::Print(lua_State* L)
	{
		std::cout << "JM :	[Lua] - " << vec4 << std::endl;
		return 1;
	}

}
