#pragma once
#include "LM.h"
#include "Scripting/LuaScript.h"
#include "Scripting/Luna.h"
#include "Graphics/Camera/FPSCamera.h"
#include "CameraBinding.h"

namespace Lumos
{
	class LUMOS_EXPORT CameraFPSBinding : public CameraBinding
	{
	public:
		static const char className[];
		static Luna<CameraFPSBinding>::FunctionType methods[];
		static Luna<CameraFPSBinding>::PropertyType properties[];

		explicit CameraFPSBinding(FPSCamera* camera);
		CameraFPSBinding(lua_State* L);
		~CameraFPSBinding();

		static void Bind();
	};
}


