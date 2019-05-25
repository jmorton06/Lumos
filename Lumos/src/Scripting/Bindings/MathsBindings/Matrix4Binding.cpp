#include "LM.h"
#include "Matrix4Binding.h"
#include "Scripting/LuaScript.h"
#include "Vector4Binding.h"

namespace lumos
{
	const char Matrix4Binding::className[] = "Matrix4";

	Luna<Matrix4Binding>::FunctionType Matrix4Binding::methods[] = {
		lunamethod(Matrix4Binding, GetRow),
		lunamethod(Matrix4Binding, Translation),
		lunamethod(Matrix4Binding, Rotation),
		lunamethod(Matrix4Binding, RotationX),
		lunamethod(Matrix4Binding, RotationY),
		lunamethod(Matrix4Binding, RotationZ),
		lunamethod(Matrix4Binding, RotationQuaternion),
		lunamethod(Matrix4Binding, Scale),
		lunamethod(Matrix4Binding, LookTo),
		lunamethod(Matrix4Binding, LookAt),

		lunamethod(Matrix4Binding, Add),
		lunamethod(Matrix4Binding, Multiply),
		lunamethod(Matrix4Binding, Transpose),
		lunamethod(Matrix4Binding, Inverse),
		lunamethod(Matrix4Binding, Print),
	{ NULL, NULL }
	};
	Luna<Matrix4Binding>::PropertyType Matrix4Binding::properties[] = {
		{ NULL, NULL }
	};

	Matrix4Binding::Matrix4Binding(const maths::Matrix4& matrix) : matrix4(matrix)
	{
	}

	Matrix4Binding::Matrix4Binding(lua_State* L)
	{
		matrix4.ToIdentity();

		int argc = LuaScripting::SGetArgCount(L);

		// fill out all the four rows of the matrix
		for (int i = 0; i < 4; ++i)
		{
			float x = 0.f, y = 0.f, z = 0.f, w = 0.f;
			if (argc > i * 4)
			{
				x = LuaScripting::SGetFloat(L, i * 4 + 1);
				if (argc > i * 4 + 1)
				{
					y = LuaScripting::SGetFloat(L, i * 4 + 2);
					if (argc > i * 4 + 2)
					{
						z = LuaScripting::SGetFloat(L, i * 4 + 3);
						if (argc > i * 4 + 3)
						{
							w = LuaScripting::SGetFloat(L, i * 4 + 4);
						}
					}
				}
			}
			matrix4.SetRow(i,maths::Vector4(x, y, z, w));
		}
	}

	Matrix4Binding::~Matrix4Binding()
	{
	}


	int Matrix4Binding::GetRow(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		int row = 0;
		if (argc > 1)
		{
			row = LuaScripting::SGetInt(L, 2);
			if (row < 0 || row > 3)
				row = 0;
		}
		Luna<Vector4Binding>::push(L, new Vector4Binding(matrix4.GetRow(row)));
		return 1;
	}

	int Matrix4Binding::Translation(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		maths::Matrix4 mat;
		mat.ToIdentity();
		if (argc > 0)
		{
			Vector4Binding* vector = Luna<Vector4Binding>::lightcheck(L, 1);
			if (vector != nullptr)
			{
				mat = maths::Matrix4::Translation(maths::Vector3(vector->vec4.GetX(), vector->vec4.GetY(), vector->vec4.GetZ()));
			}
		}
		Luna<Matrix4Binding>::push(L, new Matrix4Binding(mat));
		return 1;
	}

	int Matrix4Binding::Rotation(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		maths::Matrix4 mat;
		mat.ToIdentity();
		if (argc > 0)
		{
			Vector4Binding* vector = Luna<Vector4Binding>::lightcheck(L, 1);
			if (vector != nullptr)
			{
				//mat = Matrix4::RotationRollPitchYawFromVector(vector->vec4);
			}
		}
		Luna<Matrix4Binding>::push(L, new Matrix4Binding(mat));
		return 1;
	}

	int Matrix4Binding::RotationX(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		maths::Matrix4 mat;
		mat.ToIdentity();
		if (argc > 0)
		{
			mat = maths::Matrix4::RotationX(LuaScripting::SGetFloat(L, 1));
		}
		Luna<Matrix4Binding>::push(L, new Matrix4Binding(mat));
		return 1;
	}

	int Matrix4Binding::RotationY(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		maths::Matrix4 mat;
		mat.ToIdentity();
		if (argc > 0)
		{
			mat = maths::Matrix4::RotationY(LuaScripting::SGetFloat(L, 1));
		}
		Luna<Matrix4Binding>::push(L, new Matrix4Binding(mat));
		return 1;
	}

	int Matrix4Binding::RotationZ(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		maths::Matrix4 mat;
		mat.ToIdentity();
		if (argc > 0)
		{
			mat = maths::Matrix4::RotationZ(LuaScripting::SGetFloat(L, 1));
		}
		Luna<Matrix4Binding>::push(L, new Matrix4Binding(mat));
		return 1;
	}

	int Matrix4Binding::RotationQuaternion(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		maths::Matrix4 mat;
		mat.ToIdentity();
		if (argc > 0)
		{
			Vector4Binding* vector = Luna<Vector4Binding>::lightcheck(L, 1);
			if (vector != nullptr)
			{
				//mat = Matrix4::RotationQuaternion(vector->vec4);
			}
		}
		Luna<Matrix4Binding>::push(L, new Matrix4Binding(mat));
		return 1;
	}

	int Matrix4Binding::Scale(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		maths::Matrix4 mat;
		mat.ToIdentity();
		if (argc > 0)
		{
			Vector4Binding* vector = Luna<Vector4Binding>::lightcheck(L, 1);
			if (vector != nullptr)
			{
				mat = maths::Matrix4::Scale(maths::Vector3(vector->vec4.GetX(), vector->vec4.GetY(), vector->vec4.GetZ()));
			}
		}
		Luna<Matrix4Binding>::push(L, new Matrix4Binding(mat));
		return 1;
	}

	int Matrix4Binding::LookTo(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			Vector4Binding* pos = Luna<Vector4Binding>::lightcheck(L, 1);
			Vector4Binding* dir = Luna<Vector4Binding>::lightcheck(L, 2);
			if (pos != nullptr && dir != nullptr)
			{
				maths::Vector4 Up;
				if (argc > 3)
				{
					Vector4Binding* up = Luna<Vector4Binding>::lightcheck(L, 3);
					Up = up->vec4;
				}
				else
					Up = maths::Vector4(0, 1, 0, 0);
				//Luna<Matrix4Binding>::push(L, new Matrix4Binding(Matrix4::LookTo(pos->vec4, dir->vec4, Up)));
			}
			else
				LuaScripting::SError(L, "LookTo(Vector eye, Vector direction, opt Vector up) argument is not a Vector!");
		}
		else
			LuaScripting::SError(L, "LookTo(Vector eye, Vector direction, opt Vector up) not enough arguments!");
		return 1;
	}

	int Matrix4Binding::LookAt(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			//Vector4Binding* pos = Luna<Vector4Binding>::lightcheck(L, 1);
			Vector4Binding* dir = Luna<Vector4Binding>::lightcheck(L, 2);
			if (dir != nullptr)
			{
				maths::Vector4 Up;
				if (argc > 3)
				{
					Vector4Binding* up = Luna<Vector4Binding>::lightcheck(L, 3);
					Up = up->vec4;
				}
				else
					Up = maths::Vector4(0, 1, 0, 0);
				//Luna<Matrix4Binding>::push(L, new Matrix4Binding(Matrix4::lookat(pos->vec4, dir->vec4, Up)));
			}
			else
				LuaScripting::SError(L, "LookAt(Vector eye, Vector focusPos, opt Vector up) argument is not a Vector!");
		}
		else
			LuaScripting::SError(L, "LookAt(Vector eye, Vector focusPos, opt Vector up) not enough arguments!");
		return 1;
	}

	int Matrix4Binding::Multiply(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			Matrix4Binding* m1 = Luna<Matrix4Binding>::lightcheck(L, 1);
			Matrix4Binding* m2 = Luna<Matrix4Binding>::lightcheck(L, 2);
			if (m1 && m2)
			{
				Luna<Matrix4Binding>::push(L, new Matrix4Binding(m1->matrix4 * m2->matrix4));
				return 1;
			}
		}
		LuaScripting::SError(L, "Multiply(Matrix m1,m2) not enough arguments!");
		return 0;
	}
	int Matrix4Binding::Add(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 1)
		{
			Matrix4Binding* m1 = Luna<Matrix4Binding>::lightcheck(L, 1);
			Matrix4Binding* m2 = Luna<Matrix4Binding>::lightcheck(L, 2);
			if (m1 && m2)
			{
				//Luna<Matrix4Binding>::push(L, new Matrix4Binding(Matrix4(m1->matrix4 + m2->matrix4)));
				return 1;
			}
		}
		LuaScripting::SError(L, "Add(Matrix m1,m2) not enough arguments!");
		return 0;
	}
	int Matrix4Binding::Transpose(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 0)
		{
			Matrix4Binding* m1 = Luna<Matrix4Binding>::lightcheck(L, 1);
			if (m1)
			{
				m1->matrix4.Transpose();
				//Luna<Matrix4Binding>::push(L, new Matrix4Binding());
				return 1;
			}
		}
		LuaScripting::SError(L, "Transpose(Matrix m) not enough arguments!");
		return 0;
	}
	int Matrix4Binding::Inverse(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 0)
		{
			Matrix4Binding* m1 = Luna<Matrix4Binding>::lightcheck(L, 1);
			if (m1)
			{
				maths::Vector4 det;
				Luna<Matrix4Binding>::push(L, new Matrix4Binding(maths::Matrix4::Inverse(m1->matrix4)));
				//LuaScripting::SSetFloat(L, (det).GetX());
				return 2;
			}
		}
		LuaScripting::SError(L, "Inverse(Matrix m) not enough arguments!");
		return 0;
	}

	int Matrix4Binding::Print(lua_State* L)
	{
		std::cout << "JM :	[Lua] - " << matrix4 << std::endl;
		return 1;
	}

	void Matrix4Binding::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<Matrix4Binding>::Register(LuaScripting::GetGlobal()->GetLuaState());
			LuaScripting::GetGlobal()->RunText("matrix = Matrix4()");
		}
	}
}
