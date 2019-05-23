#pragma once
#include "LM.h"
#include "Scripting/LuaScript.h"
#include "Scripting/Luna.h"
#include "Graphics/Camera/FPSCamera.h"

namespace lumos
{
	class LUMOS_EXPORT CameraBinding
	{
	public:
		Camera* camera;

		static const char className[];
		static Luna<CameraBinding>::FunctionType methods[];
		static Luna<CameraBinding>::PropertyType properties[];

		explicit CameraBinding(Camera* camera);
		CameraBinding(lua_State* L);
		~CameraBinding();

		int BuildViewMatrix(lua_State* L);

		int GetPosition(lua_State* L);
		int SetPosition(lua_State* L);

		int GetPitch(lua_State* L);
		int SetPitch(lua_State* L);

		int GetYaw(lua_State* L);
		int SetYaw(lua_State* L);

		int InvertPitch(lua_State* L);
		int InvertYaw(lua_State* L);

		int UpdateScroll(lua_State* L);
		int SetMouseSensitivity(lua_State* L);

		int GetProjectionMatrix(lua_State* L);
		int SetProjectionMatrix(lua_State* L);
		int GetViewMatrix(lua_State* L);

		int GetNear(lua_State* L);
		int GetFar(lua_State* L);
		int GetFOV(lua_State* L);

		int Print(lua_State* L);

		static void Bind();
	};
}


