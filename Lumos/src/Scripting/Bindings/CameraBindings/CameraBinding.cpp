#include "LM.h"
#include "CameraBinding.h"
#include "Scripting/LuaScript.h"
#include "../MathsBindings/Vector3Binding.h"
#include "Scripting/Bindings/MathsBindings/Matrix4Binding.h"

namespace lumos
{
	const char CameraBinding::className[] = "Camera";

	Luna<CameraBinding>::FunctionType CameraBinding::methods[] = {
			lunamethod(CameraBinding, BuildViewMatrix),
			lunamethod(CameraBinding, GetPosition),
			lunamethod(CameraBinding, SetPosition),
			lunamethod(CameraBinding, GetPitch),
			lunamethod(CameraBinding, SetPitch),
            lunamethod(CameraBinding, GetYaw),
            lunamethod(CameraBinding, SetYaw),
			lunamethod(CameraBinding, InvertPitch),
			lunamethod(CameraBinding, InvertYaw),
			lunamethod(CameraBinding, UpdateScroll),
			lunamethod(CameraBinding, SetMouseSensitivity),
			lunamethod(CameraBinding, GetProjectionMatrix),
			lunamethod(CameraBinding, SetProjectionMatrix),
			lunamethod(CameraBinding, GetViewMatrix),
			lunamethod(CameraBinding, GetNear),
			lunamethod(CameraBinding, GetFar),
			lunamethod(CameraBinding, GetFOV),
			lunamethod(CameraBinding, Print),
			{ NULL, NULL }
	};
	Luna<CameraBinding>::PropertyType CameraBinding::properties[] = {
			{ NULL, NULL }
	};

	CameraBinding::CameraBinding(Camera* camera) : camera(camera)
	{
	}

	CameraBinding::CameraBinding(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		float fov = 0.f, n = 0.f, f = 0.f,sr=1.f;
	
		if (argc > 0)
		{
			fov = LuaScripting::SGetFloat(L, 1);
			if (argc > 1)
			{
				n = LuaScripting::SGetFloat(L, 2);
				if (argc > 2)
				{
					f = LuaScripting::SGetFloat(L, 3);
					if (argc > 3)
					{
						sr = LuaScripting::SGetFloat(L, 4);
					}
				}
			}
		}
		camera = new FPSCamera(fov,n,f,sr);
	}

	CameraBinding::~CameraBinding()
	{
        delete camera;
	}

	void CameraBinding::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<CameraBinding>::Register(LuaScripting::GetGlobal()->GetLuaState());
			//LuaScripting::GetGlobal()->RunText("camera = Camera()");
		}
	}

	int CameraBinding::BuildViewMatrix(lua_State *L)
	{
		camera->BuildViewMatrix();
		return 1;
	}

	int CameraBinding::GetPosition(lua_State *L)
	{
		Luna<Vector3Binding>::push(L, new Vector3Binding(camera->GetPosition()));
		return 1;
	}

	int CameraBinding::SetPosition(lua_State *L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 0)
		{
			Vector3Binding* v1 = Luna<Vector3Binding>::lightcheck(L, 1);
			if (v1)
			{
				camera->SetPosition(v1->vec3);
				return 1;
			}
		}
		LuaScripting::SError(L, "SetPosition not enough arguments!");
		return 0;
	}

	int CameraBinding::GetPitch(lua_State *L)
	{
		LuaScripting::SSetFloat(L, camera->GetPitch());
		return 1;
	}

	int CameraBinding::SetPitch(lua_State *L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 0)
		{
			camera->SetPitch(LuaScripting::SGetFloat(L, 1));
		}
		else
			LuaScripting::SError(L, "SetPitch(float value) not enough arguments!");
		return 0;
	}

    int CameraBinding::SetYaw(lua_State *L)
    {
        int argc = LuaScripting::SGetArgCount(L);
        if (argc > 0)
        {
            camera->SetYaw(LuaScripting::SGetFloat(L, 1));
        }
        else
            LuaScripting::SError(L, "SetYaw(float value) not enough arguments!");
        return 0;
    }

    int CameraBinding::GetYaw(lua_State *L)
    {
        LuaScripting::SSetFloat(L, camera->GetYaw());
        return 1;
    }

	int CameraBinding::InvertPitch(lua_State *L)
	{
		camera->InvertPitch();
		return 1;
	}

	int CameraBinding::InvertYaw(lua_State *L)
	{
		camera->InvertYaw();
		return 1;
	}

	int CameraBinding::UpdateScroll(lua_State *L)
	{
		return 0;
	}

	int CameraBinding::SetMouseSensitivity(lua_State *L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 0)
		{
			camera->SetMouseSensitivity(LuaScripting::SGetFloat(L, 1));
		}
		else
			LuaScripting::SError(L, "SetMouseSensitivity(float value) not enough arguments!");
		return 0;
	}

	int CameraBinding::GetProjectionMatrix(lua_State *L)
	{
		Luna<Matrix4Binding>::push(L, new Matrix4Binding(camera->GetProjectionMatrix()));
		return 1;
	}

	int CameraBinding::SetProjectionMatrix(lua_State *L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 0)
		{
			Matrix4Binding* v1 = Luna<Matrix4Binding>::lightcheck(L, 1);
			if (v1)
			{
				camera->SetProjectionMatrix(v1->matrix4);
				return 1;
			}
		}
		LuaScripting::SError(L, "SetProjectionMatrix not enough arguments!");
		return 0;
	}

	int CameraBinding::GetViewMatrix(lua_State *L)
	{
		Luna<Matrix4Binding>::push(L, new Matrix4Binding(camera->GetViewMatrix()));
		return 1;
	}

	int CameraBinding::GetNear(lua_State *L)
	{
		LuaScripting::SSetFloat(L, camera->GetNear());
		return 1;
	}

	int CameraBinding::GetFar(lua_State *L)
	{
		LuaScripting::SSetFloat(L, camera->GetFar());
		return 1;
	}

	int CameraBinding::GetFOV(lua_State *L)
	{
		LuaScripting::SSetFloat(L, camera->GetFOV());
		return 1;
	}

	int CameraBinding::Print(lua_State *L)
	{
	    std::cout << "Position - " << camera->GetPosition() << "Pitch/Yaw - " << camera->GetPitch() << "," << camera->GetYaw() << std::endl;
        std::cout << "FOV - " << camera->GetFOV() << "Near/Far - " << camera->GetNear() << "," << camera->GetFar() << std::endl;
		return 1;
    }
}
