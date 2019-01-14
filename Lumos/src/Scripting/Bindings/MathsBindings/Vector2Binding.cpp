#include "LM.h"
#include "Vector2Binding.h"
#include "Scripting/LuaScript.h"

namespace Lumos
{
	const char Vector2Binding::className[] = "Vector2";

	Luna<Vector2Binding>::FunctionType Vector2Binding::methods[] = {
		lunamethod(Vector2Binding, GetX),
		lunamethod(Vector2Binding, GetY),
		lunamethod(Vector2Binding, SetX),
		lunamethod(Vector2Binding, SetY),
		lunamethod(Vector2Binding, Transform),
		lunamethod(Vector2Binding, Length),
		lunamethod(Vector2Binding, Normalize),
		lunamethod(Vector2Binding, QuaternionNormalize),
		lunamethod(Vector2Binding, Add),
		lunamethod(Vector2Binding, Subtract),
		lunamethod(Vector2Binding, Multiply),
		lunamethod(Vector2Binding, Dot),
		lunamethod(Vector2Binding, Cross),
		lunamethod(Vector2Binding, Lerp),
		lunamethod(Vector2Binding, Slerp),
		lunamethod(Vector2Binding, Clamp),
		lunamethod(Vector2Binding, Normalize),
		lunamethod(Vector2Binding, QuaternionMultiply),
		lunamethod(Vector2Binding, QuaternionFromRollPitchYaw),
		lunamethod(Vector2Binding, Print),
	{ NULL, NULL }
	};
	Luna<Vector2Binding>::PropertyType Vector2Binding::properties[] = {
		{ NULL, NULL }
	};

	Vector2Binding::Vector2Binding(const maths::Vector2& vec2) : vec2(vec2)
	{
	}

	Vector2Binding::Vector2Binding(lua_State* L)
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
		vec2 = maths::Vector2(x, y);
	}

	Vector2Binding::~Vector2Binding()
	{
	}


	int Vector2Binding::GetX(lua_State* L)
	{
		LuaScripting::SSetFloat(L, vec2.GetX());
		return 1;
	}
	int Vector2Binding::GetY(lua_State* L)
	{
		LuaScripting::SSetFloat(L, vec2.GetY());
		return 1;
	}

	int Vector2Binding::SetX(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 0)
		{
			vec2.SetX(LuaScripting::SGetFloat(L, 1));
		}
		else
			LuaScripting::SError(L, "SetX(float value) not enough arguments!");
		return 0;
	}
	int Vector2Binding::SetY(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 0)
		{
			vec2.SetY(LuaScripting::SGetFloat(L, 1));
		}
		else
			LuaScripting::SError(L, "SetY(float value) not enough arguments!");
		return 0;
	}

	int Vector2Binding::Transform(lua_State* L)
	{
		/*int argc = LuaScripting::SGetArgCount(L);
		if (argc > 0)
		{
			Matrix_BindLua* mat = Luna<Matrix_BindLua>::lightcheck(L, 1);
			if (mat)
			{
				Luna<Vector2Binding>::push(L, new Vector2Binding(XMVector2Transform(vec2, mat->matrix)));
				return 1;
			}
			else
				LuaScripting::SError(L, "Transform(Matrix matrix) argument is not a Matrix!");
		}
		else
			LuaScripting::SError(L, "Transform(Matrix matrix) not enough arguments!");*/
		return 0;
	}
	int Vector2Binding::Length(lua_State* L)
	{
		LuaScripting::SSetFloat(L, vec2.Length());
		return 1;
	}
	int Vector2Binding::Normalize(lua_State* L)
	{
		vec2.Normalise();
		Luna<Vector2Binding>::push(L, new Vector2Binding(vec2));
		return 1;
	}
	int Vector2Binding::QuaternionNormalize(lua_State* L)
	{
		//Luna<Vector2Binding>::push(L, new Vector2Binding((vec2)));
		return 1;
	}
	int Vector2Binding::Clamp(lua_State* L)
	{
		/*int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			float a = LuaScripting::SGetFloat(L, 1);
			float b = LuaScripting::SGetFloat(L, 2);
			Luna<Vector2Binding>::push(L, new Vector2Binding((vec2, (a, a, a, a), (b, b, b, b))));
			return 1;
		}
		else
			LuaScripting::SError(L, "Clamp(float min,max) not enough arguments!");*/
		return 0;
	}
	int Vector2Binding::Saturate(lua_State* L)
	{
		//Luna<Vector2Binding>::push(L, new Vector2Binding((vec2)));
		return 1;
	}



	int Vector2Binding::Dot(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			Vector2Binding* v1 = Luna<Vector2Binding>::lightcheck(L, 1);
			Vector2Binding* v2 = Luna<Vector2Binding>::lightcheck(L, 2);
			if (v1 && v2)
			{
				//LuaScripting::SSetFloat(L, v1->vec2.( v2->vec2));
				return 1;
			}
		}
		LuaScripting::SError(L, "Dot(vec2 v1,v2) not enough arguments!");
		return 0;
	}
	int Vector2Binding::Cross(lua_State* L)
	{
		/*int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			Vector2Binding* v1 = Luna<Vector2Binding>::lightcheck(L, 1);
			Vector2Binding* v2 = Luna<Vector2Binding>::lightcheck(L, 2);
			if (v1 && v2)
			{
				Luna<Vector2Binding>::push(L, new Vector2Binding(v1->vec2.(v2->vec2));
				return 1;
			}
		}*/
		LuaScripting::SError(L, "Cross(vec2 v1,v2) not enough arguments!");
		return 0;
	}
	int Vector2Binding::Multiply(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			Vector2Binding* v1 = Luna<Vector2Binding>::lightcheck(L, 1);
			Vector2Binding* v2 = Luna<Vector2Binding>::lightcheck(L, 2);
			if (v1 && v2)
			{
				//Luna<Vector2Binding>::push(L, new Vector2Binding(v1->vec2 * v2->vec2));
				return 1;
			}
			else if (v1)
			{
				Luna<Vector2Binding>::push(L, new Vector2Binding(v1->vec2 * LuaScripting::SGetFloat(L, 2)));
				return 1;
			}
			else if (v2)
			{
				//Luna<Vector2Binding>::push(L, new Vector2Binding(LuaScripting::SGetFloat(L, 1) * v2->vec2));
				return 1;
			}
		}
		LuaScripting::SError(L, "Multiply(vec2 v1,v2) not enough arguments!");
		return 0;
	}
	int Vector2Binding::Add(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			Vector2Binding* v1 = Luna<Vector2Binding>::lightcheck(L, 1);
			Vector2Binding* v2 = Luna<Vector2Binding>::lightcheck(L, 2);
			if (v1 && v2)
			{
				Luna<Vector2Binding>::push(L, new Vector2Binding(v1->vec2 + v2->vec2));
				return 1;
			}
		}
		LuaScripting::SError(L, "Add(vec2 v1,v2) not enough arguments!");
		return 0;
	}
	int Vector2Binding::Subtract(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			Vector2Binding* v1 = Luna<Vector2Binding>::lightcheck(L, 1);
			Vector2Binding* v2 = Luna<Vector2Binding>::lightcheck(L, 2);
			if (v1 && v2)
			{
				Luna<Vector2Binding>::push(L, new Vector2Binding(v1->vec2 - v2->vec2));
				return 1;
			}
		}
		LuaScripting::SError(L, "Subtract(vec2 v1,v2) not enough arguments!");
		return 0;
	}
	int Vector2Binding::Lerp(lua_State* L)
	{
		/*int argc = LuaScripting::SGetArgCount(L);
		if (argc > 2)
		{
			Vector2Binding* v1 = Luna<Vector2Binding>::lightcheck(L, 1);
			Vector2Binding* v2 = Luna<Vector2Binding>::lightcheck(L, 2);
			float t = LuaScripting::SGetFloat(L, 3);
			if (v1 && v2)
			{
				Luna<Vector2Binding>::push(L, new Vector2Binding(XMVectorLerp(v1->vec2, v2->vec2, t)));
				return 1;
			}
		}*/
		LuaScripting::SError(L, "Lerp(vec2 v1,v2, float t) not enough arguments!");
		return 0;
	}


	int Vector2Binding::QuaternionMultiply(lua_State* L)
	{/*
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			Vector2Binding* v1 = Luna<Vector2Binding>::lightcheck(L, 1);
			Vector2Binding* v2 = Luna<Vector2Binding>::lightcheck(L, 2);
			if (v1 && v2)
			{
				Luna<Vector2Binding>::push(L, new Vector2Binding(XMQuaternionMultiply(v1->vec2, v2->vec2)));
				return 1;
			}
		}
		LuaScripting::SError(L, "QuaternionMultiply(vec2 v1,v2) not enough arguments!");*/
		return 0;
	}
	int Vector2Binding::QuaternionFromRollPitchYaw(lua_State* L)
	{
		/*int argc = LuaScripting::SGetArgCount(L);
		if (argc > 0)
		{
			Vector2Binding* v1 = Luna<Vector2Binding>::lightcheck(L, 1);
			if (v1)
			{
				Luna<Vector2Binding>::push(L, new Vector2Binding(XMQuaternionRotationRollPitchYawFromVector(v1->vec2)));
				return 1;
			}
		}
		LuaScripting::SError(L, "QuaternionFromRollPitchYaw(vec2 rotXYZ) not enough arguments!");*/
		return 0;
	}
	int Vector2Binding::Slerp(lua_State* L)
	{
		/*int argc = LuaScripting::SGetArgCount(L);
		if (argc > 2)
		{
			Vector2Binding* v1 = Luna<Vector2Binding>::lightcheck(L, 1);
			Vector2Binding* v2 = Luna<Vector2Binding>::lightcheck(L, 2);
			float t = LuaScripting::SGetFloat(L, 3);
			if (v1 && v2)
			{
				Luna<Vector2Binding>::push(L, new Vector2Binding(XMQuaternionSlerp(v1->vec2, v2->vec2, t)));
				return 1;
			}
		}
		LuaScripting::SError(L, "QuaternionSlerp(vec2 v1,v2, float t) not enough arguments!");*/
		return 0;
	}


	void Vector2Binding::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<Vector2Binding>::Register(LuaScripting::GetGlobal()->GetLuaState());
			LuaScripting::GetGlobal()->RunText("vec2 = Vector2()");
		}
	}

	int Vector2Binding::Print(lua_State* L)
	{
		std::cout << "JM :	[Lua] - " << vec2 << std::endl;
		return 1;
	}

}
