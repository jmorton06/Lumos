#pragma once
#include "lmpch.h"
#include "CameraController.h"

#include <sol/sol.hpp>
 
namespace Lumos
{

	class LUMOS_EXPORT LuaCameraController : CameraController
	{
	public:
		LuaCameraController(Camera* camera);
		virtual ~LuaCameraController() override;

		virtual void HandleMouse(float dt, float xpos, float ypos) override;
		virtual void HandleKeyboard(float dt) override;

        void SetFunctions(sol::protected_function mouseFunc, sol::protected_function keyboardFunc)
        {
            m_HandleKeyboardFunc = keyboardFunc;
            m_HandleMouseFunc = mouseFunc;
        }

        void SetScrollFunc(sol::protected_function scrollFunc)
        {
            m_HandleScrollFunc = scrollFunc;
        }

    private: 
        sol::protected_function m_HandleMouseFunc;
        sol::protected_function m_HandleKeyboardFunc;
        sol::protected_function m_HandleScrollFunc;

	};

}

