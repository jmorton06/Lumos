#include "LM.h"
#include "Vector3Binding.h"
#include "Scripting/LuaScript.h"

namespace lumos
{
	const char Vector3Binding::className[] = "Vector3";

	Luna<Vector3Binding>::FunctionType Vector3Binding::methods[] = {
		lunamethod(Vector3Binding, GetX),
		lunamethod(Vector3Binding, GetY),
		lunamethod(Vector3Binding, GetZ),
		lunamethod(Vector3Binding, SetX),
		lunamethod(Vector3Binding, SetY),
		lunamethod(Vector3Binding, SetZ),
		lunamethod(Vector3Binding, Transform),
		lunamethod(Vector3Binding, Length),
		lunamethod(Vector3Binding, Normalize),
		lunamethod(Vector3Binding, QuaternionNormalize),
		lunamethod(Vector3Binding, Add),
		lunamethod(Vector3Binding, Subtract),
		lunamethod(Vector3Binding, Multiply),
		lunamethod(Vector3Binding, Dot),
		lunamethod(Vector3Binding, Cross),
		lunamethod(Vector3Binding, Lerp),
		lunamethod(Vector3Binding, Slerp),
		lunamethod(Vector3Binding, Clamp),
		lunamethod(Vector3Binding, Normalize),
		lunamethod(Vector3Binding, QuaternionMultiply),
		lunamethod(Vector3Binding, QuaternionFromRollPitchYaw),
		lunamethod(Vector3Binding, Print),
	{ NULL, NULL }
	};
	Luna<Vector3Binding>::PropertyType Vector3Binding::properties[] = {
		{ NULL, NULL }
};

    Vector3Binding::Vector3Binding(const maths::Vector3& vec3) : vec3(vec3)
	{
	}

    Vector3Binding::Vector3Binding(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		float x = 0.f, y = 0.f, z = 0.f;

		if (argc > 0)
		{
			x = LuaScripting::SGetFloat(L, 1);
			if (argc > 1)
			{
				y = LuaScripting::SGetFloat(L, 2);
				if (argc > 2)
				{
					z = LuaScripting::SGetFloat(L, 3);
				}
			}
		}
		vec3 = maths::Vector3(x, y, z);
	}

    Vector3Binding::~Vector3Binding()
	{
	}

	int Vector3Binding::GetX(lua_State* L)
	{
		LuaScripting::SSetFloat(L, vec3.GetX());
		return 1;
	}

	int Vector3Binding::GetY(lua_State* L)
	{
		LuaScripting::SSetFloat(L, vec3.GetY());
		return 1;
	}

	int Vector3Binding::GetZ(lua_State* L)
	{
        LuaScripting::SSetFloat(L, vec3.GetZ());
        return 1;
    }

	int Vector3Binding::SetX(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 0)
		{
			vec3.SetX(LuaScripting::SGetFloat(L, 1));
		}
		else
			LuaScripting::SError(L, "SetX(float value) not enough arguments!");
		return 0;
	}
	int Vector3Binding::SetY(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 0)
		{
			vec3.SetY(LuaScripting::SGetFloat(L, 1));
		}
		else
			LuaScripting::SError(L, "SetY(float value) not enough arguments!");
		return 0;
	}
	int Vector3Binding::SetZ(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 0)
		{
			vec3.SetZ(LuaScripting::SGetFloat(L, 1));
		}
		else
			LuaScripting::SError(L, "SetZ(float value) not enough arguments!");
		return 0;
	}

	int Vector3Binding::Transform(lua_State* L)
	{
		/*int argc = LuaScripting::SGetArgCount(L);
		if (argc > 0)
		{
			Matrix_BindLua* mat = Luna<Matrix_BindLua>::lightcheck(L, 1);
			if (mat)
			{
				Luna<Vector3Binding>::push(L, new Vector3Binding(XMVector4Transform(vec3, mat->matrix)));
				return 1;
			}
			else
				LuaScripting::SError(L, "Transform(Matrix matrix) argument is not a Matrix!");
		}
		else
			LuaScripting::SError(L, "Transform(Matrix matrix) not enough arguments!");*/
		return 0;
	}
	int Vector3Binding::Length(lua_State* L)
	{
		LuaScripting::SSetFloat(L, vec3.Length());
		return 1;
	}
	int Vector3Binding::Normalize(lua_State* L)
	{
		Luna<Vector3Binding>::push(L, new Vector3Binding(vec3.Normal()));
		return 1;
	}
	int Vector3Binding::QuaternionNormalize(lua_State* L)
	{
		//Luna<Vector3Binding>::push(L, new Vector3Binding((vec3)));
		return 1;
	}
	int Vector3Binding::Clamp(lua_State* L)
	{
		/*int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			float a = LuaScripting::SGetFloat(L, 1);
			float b = LuaScripting::SGetFloat(L, 2);
			Luna<Vector3Binding>::push(L, new Vector3Binding((vec3, (a, a, a, a), (b, b, b, b))));
			return 1;
		}
		else
			LuaScripting::SError(L, "Clamp(float min,max) not enough arguments!");*/
		return 0;
	}
	int Vector3Binding::Saturate(lua_State* L)
	{
		//Luna<Vector3Binding>::push(L, new Vector3Binding((vec3)));
		return 1;
	}



	int Vector3Binding::Dot(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			Vector3Binding* v1 = Luna<Vector3Binding>::lightcheck(L, 1);
			Vector3Binding* v2 = Luna<Vector3Binding>::lightcheck(L, 2);
			if (v1 && v2)
			{
				LuaScripting::SSetFloat(L, v1->vec3.Dot( v2->vec3));
				return 1;
			}
		}
		LuaScripting::SError(L, "Dot(vec3 v1,v2) not enough arguments!");
		return 0;
	}
	int Vector3Binding::Cross(lua_State* L)
	{
		/*int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			Vector3Binding* v1 = Luna<Vector3Binding>::lightcheck(L, 1);
			Vector3Binding* v2 = Luna<Vector3Binding>::lightcheck(L, 2);
			if (v1 && v2)
			{
				Luna<Vector3Binding>::push(L, new Vector3Binding(v1->vec3.(v2->vec3));
				return 1;
			}
		}*/
		LuaScripting::SError(L, "Cross(vec3 v1,v2) not enough arguments!");
		return 0;
	}
	int Vector3Binding::Multiply(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			Vector3Binding* v1 = Luna<Vector3Binding>::lightcheck(L, 1);
			Vector3Binding* v2 = Luna<Vector3Binding>::lightcheck(L, 2);
			if (v1 && v2)
			{
				Luna<Vector3Binding>::push(L, new Vector3Binding(v1->vec3 * v2->vec3));
				return 1;
			}
			else if (v1)
			{
				Luna<Vector3Binding>::push(L, new Vector3Binding(v1->vec3 * LuaScripting::SGetFloat(L, 2)));
				return 1;
			}
			else if (v2)
			{
				Luna<Vector3Binding>::push(L, new Vector3Binding(v2->vec3 * LuaScripting::SGetFloat(L, 1)));
				return 1;
			}
		}
		LuaScripting::SError(L, "Multiply(vec3 v1,v2) not enough arguments!");
		return 0;
	}
	int Vector3Binding::Add(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			Vector3Binding* v1 = Luna<Vector3Binding>::lightcheck(L, 1);
			Vector3Binding* v2 = Luna<Vector3Binding>::lightcheck(L, 2);
			if (v1 && v2)
			{
				Luna<Vector3Binding>::push(L, new Vector3Binding(v1->vec3 + v2->vec3));
				return 1;
			}
		}
		LuaScripting::SError(L, "Add(vec3 v1,v2) not enough arguments!");
		return 0;
	}
	int Vector3Binding::Subtract(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			Vector3Binding* v1 = Luna<Vector3Binding>::lightcheck(L, 1);
			Vector3Binding* v2 = Luna<Vector3Binding>::lightcheck(L, 2);
			if (v1 && v2)
			{
				Luna<Vector3Binding>::push(L, new Vector3Binding(v1->vec3 - v2->vec3));
				return 1;
			}
		}
		LuaScripting::SError(L, "Subtract(vec3 v1,v2) not enough arguments!");
		return 0;
	}
	int Vector3Binding::Lerp(lua_State* L)
	{
		/*int argc = LuaScripting::SGetArgCount(L);
		if (argc > 2)
		{
			Vector3Binding* v1 = Luna<Vector3Binding>::lightcheck(L, 1);
			Vector3Binding* v2 = Luna<Vector3Binding>::lightcheck(L, 2);
			float t = LuaScripting::SGetFloat(L, 3);
			if (v1 && v2)
			{
				Luna<Vector3Binding>::push(L, new Vector3Binding(XMVectorLerp(v1->vec3, v2->vec3, t)));
				return 1;
			}
		}*/
		LuaScripting::SError(L, "Lerp(vec3 v1,v2, float t) not enough arguments!");
		return 0;
	}


	int Vector3Binding::QuaternionMultiply(lua_State* L)
	{/*
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			Vector3Binding* v1 = Luna<Vector3Binding>::lightcheck(L, 1);
			Vector3Binding* v2 = Luna<Vector3Binding>::lightcheck(L, 2);
			if (v1 && v2)
			{
				Luna<Vector3Binding>::push(L, new Vector3Binding(XMQuaternionMultiply(v1->vec3, v2->vec3)));
				return 1;
			}
		}
		LuaScripting::SError(L, "QuaternionMultiply(vec3 v1,v2) not enough arguments!");*/
		return 0;
	}
	int Vector3Binding::QuaternionFromRollPitchYaw(lua_State* L)
	{
		/*int argc = LuaScripting::SGetArgCount(L);
		if (argc > 0)
		{
			Vector3Binding* v1 = Luna<Vector3Binding>::lightcheck(L, 1);
			if (v1)
			{
				Luna<Vector3Binding>::push(L, new Vector3Binding(XMQuaternionRotationRollPitchYawFromVector(v1->vec3)));
				return 1;
			}
		}
		LuaScripting::SError(L, "QuaternionFromRollPitchYaw(vec3 rotXYZ) not enough arguments!");*/
		return 0;
	}
	int Vector3Binding::Slerp(lua_State* L)
	{
		/*int argc = LuaScripting::SGetArgCount(L);
		if (argc > 2)
		{
			Vector3Binding* v1 = Luna<Vector3Binding>::lightcheck(L, 1);
			Vector3Binding* v2 = Luna<Vector3Binding>::lightcheck(L, 2);
			float t = LuaScripting::SGetFloat(L, 3);
			if (v1 && v2)
			{
				Luna<Vector3Binding>::push(L, new Vector3Binding(XMQuaternionSlerp(v1->vec3, v2->vec3, t)));
				return 1;
			}
		}
		LuaScripting::SError(L, "QuaternionSlerp(vec3 v1,v2, float t) not enough arguments!");*/
		return 0;
	}


	void Vector3Binding::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<Vector3Binding>::Register(LuaScripting::GetGlobal()->GetLuaState());
			LuaScripting::GetGlobal()->RunText("vec3 = Vector3()");
		}
	}

	int Vector3Binding::Print(lua_State* L)
	{
		std::cout << "JM :	[Lua] - " << vec3 << std::endl;
		return 1;
	}

}
