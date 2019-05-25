#pragma once
#include "LM.h"
#include "Scripting/LuaScript.h"
#include "Scripting/Luna.h"
#include "Graphics/Camera/Camera2D.h"
#include "CameraBinding.h"

namespace lumos
{
	class LUMOS_EXPORT Camera2DBinding : public CameraBinding
	{
	public:
		static const char className[];
		static Luna<Camera2DBinding>::FunctionType methods[];
		static Luna<Camera2DBinding>::PropertyType properties[];

		explicit Camera2DBinding(Camera2D* camera);
		Camera2DBinding(lua_State* L);
		~Camera2DBinding();

		static void Bind();
	};
}


