#include "LM.h"
#include "CameraMayaBinding.h"
#include "Scripting/LuaScript.h"
#include "../MathsBindings/Vector3Binding.h"
#include "Scripting/Bindings/MathsBindings/Matrix4Binding.h"

namespace Lumos
{
	const char CameraMayaBinding::className[] = "MayaCamera";

	Luna<CameraMayaBinding>::FunctionType CameraMayaBinding::methods[] = {
			lunamethod(CameraMayaBinding, BuildViewMatrix),
			lunamethod(CameraMayaBinding, GetPosition),
			lunamethod(CameraMayaBinding, SetPosition),
			lunamethod(CameraMayaBinding, GetPitch),
			lunamethod(CameraMayaBinding, SetPitch),
            lunamethod(CameraMayaBinding, GetYaw),
            lunamethod(CameraMayaBinding, SetYaw),
			lunamethod(CameraMayaBinding, InvertPitch),
			lunamethod(CameraMayaBinding, InvertYaw),
			lunamethod(CameraMayaBinding, UpdateScroll),
			lunamethod(CameraMayaBinding, SetMouseSensitivity),
			lunamethod(CameraMayaBinding, GetProjectionMatrix),
			lunamethod(CameraMayaBinding, SetProjectionMatrix),
			lunamethod(CameraMayaBinding, GetViewMatrix),
			lunamethod(CameraMayaBinding, GetNear),
			lunamethod(CameraMayaBinding, GetFar),
			lunamethod(CameraMayaBinding, GetFOV),
            lunamethod(CameraMayaBinding, Print),
			{ NULL, NULL }
	};
	Luna<CameraMayaBinding>::PropertyType CameraMayaBinding::properties[] = {
			{ NULL, NULL }
	};

	CameraMayaBinding::CameraMayaBinding(MayaCamera* camera) : CameraBinding(camera)
	{
	}

	CameraMayaBinding::CameraMayaBinding(lua_State* L) : CameraBinding(L)
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
		camera = new MayaCamera(fov, n, f, sr);
	}

	CameraMayaBinding::~CameraMayaBinding()
	{
	}

	void CameraMayaBinding::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<CameraMayaBinding>::Register(LuaScripting::GetGlobal()->GetLuaState());
			//LuaScripting::GetGlobal()->RunText("camera = Camera()");
		}
	}
}
