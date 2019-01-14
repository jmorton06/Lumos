#include "LM.h"
#include "CameraFPSBinding.h"
#include "Scripting/LuaScript.h"
#include "../MathsBindings/Vector3Binding.h"
#include "Scripting/Bindings/MathsBindings/Matrix4Binding.h"

namespace Lumos
{
	const char CameraFPSBinding::className[] = "FPSCamera";

	Luna<CameraFPSBinding>::FunctionType CameraFPSBinding::methods[] = {
			lunamethod(CameraFPSBinding, BuildViewMatrix),
			lunamethod(CameraFPSBinding, GetPosition),
			lunamethod(CameraFPSBinding, SetPosition),
			lunamethod(CameraFPSBinding, GetPitch),
			lunamethod(CameraFPSBinding, SetPitch),
			lunamethod(CameraFPSBinding, GetYaw),
			lunamethod(CameraFPSBinding, SetYaw),
			lunamethod(CameraFPSBinding, InvertPitch),
			lunamethod(CameraFPSBinding, InvertYaw),
			lunamethod(CameraFPSBinding, UpdateScroll),
			lunamethod(CameraFPSBinding, SetMouseSensitivity),
			lunamethod(CameraFPSBinding, GetProjectionMatrix),
			lunamethod(CameraFPSBinding, SetProjectionMatrix),
			lunamethod(CameraFPSBinding, GetViewMatrix),
			lunamethod(CameraFPSBinding, GetNear),
			lunamethod(CameraFPSBinding, GetFar),
			lunamethod(CameraFPSBinding, GetFOV),
            lunamethod(CameraFPSBinding, Print),
			{ NULL, NULL }
	};
	Luna<CameraFPSBinding>::PropertyType CameraFPSBinding::properties[] = {
			{ NULL, NULL }
	};

	CameraFPSBinding::CameraFPSBinding(FPSCamera* camera) : CameraBinding(camera)
	{
	}

	CameraFPSBinding::CameraFPSBinding(lua_State* L) : CameraBinding(L)
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
		camera = new FPSCamera(fov, n, f, sr);
	}

	CameraFPSBinding::~CameraFPSBinding()
	{
	}

	void CameraFPSBinding::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<CameraFPSBinding>::Register(LuaScripting::GetGlobal()->GetLuaState());
			//LuaScripting::GetGlobal()->RunText("camera = Camera()");
		}
	}
}
