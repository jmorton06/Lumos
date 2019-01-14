#include "JM.h"
#include "CameraThirdPersonBinding.h"
#include "Scripting/LuaScript.h"
#include "../MathsBindings/Vector3Binding.h"
#include "Scripting/Bindings/MathsBindings/Matrix4Binding.h"

namespace jm
{
	const char CameraThirdPersonBinding::className[] = "ThirdPersonCamera";

	Luna<CameraThirdPersonBinding>::FunctionType CameraThirdPersonBinding::methods[] = {
			lunamethod(CameraThirdPersonBinding, BuildViewMatrix),
			lunamethod(CameraThirdPersonBinding, GetPosition),
			lunamethod(CameraThirdPersonBinding, SetPosition),
			lunamethod(CameraThirdPersonBinding, GetPitch),
			lunamethod(CameraThirdPersonBinding, SetPitch),
            lunamethod(CameraThirdPersonBinding, GetYaw),
            lunamethod(CameraThirdPersonBinding, SetYaw),
			lunamethod(CameraThirdPersonBinding, InvertPitch),
			lunamethod(CameraThirdPersonBinding, InvertYaw),
			lunamethod(CameraThirdPersonBinding, UpdateScroll),
			lunamethod(CameraThirdPersonBinding, SetMouseSensitivity),
			lunamethod(CameraThirdPersonBinding, GetProjectionMatrix),
			lunamethod(CameraThirdPersonBinding, SetProjectionMatrix),
			lunamethod(CameraThirdPersonBinding, GetViewMatrix),
			lunamethod(CameraThirdPersonBinding, GetNear),
			lunamethod(CameraThirdPersonBinding, GetFar),
			lunamethod(CameraThirdPersonBinding, GetFOV),
            lunamethod(CameraThirdPersonBinding, Print),
			{ NULL, NULL }
	};
	Luna<CameraThirdPersonBinding>::PropertyType CameraThirdPersonBinding::properties[] = {
			{ NULL, NULL }
	};

	CameraThirdPersonBinding::CameraThirdPersonBinding(ThirdPersonCamera* camera) : CameraBinding(camera)
	{
	}

	CameraThirdPersonBinding::CameraThirdPersonBinding(lua_State* L) : CameraBinding(L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		float fov = 0.f, n = 0.f, f = 0.f, sr = 1.f;

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
		camera = new ThirdPersonCamera(fov, n, f, sr);
	}

	CameraThirdPersonBinding::~CameraThirdPersonBinding()
	{
	}

	void CameraThirdPersonBinding::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<CameraThirdPersonBinding>::Register(LuaScripting::GetGlobal()->GetLuaState());
			//LuaScripting::GetGlobal()->RunText("camera = Camera()");
		}
	}
}
