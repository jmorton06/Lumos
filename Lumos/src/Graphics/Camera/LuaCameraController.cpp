#include "lmpch.h"
#include "LuaCameraController.h"
#include "App/Application.h"
#include "Core/OS/Input.h"
#include "Core/OS/Window.h"
#include "Camera.h"

#include <imgui/imgui.h>

namespace Lumos
{

	LuaCameraController::LuaCameraController(Camera* camera)
		: CameraController(camera)
	{
	}

	LuaCameraController::~LuaCameraController() = default;

	void LuaCameraController::HandleMouse(float dt, float xpos, float ypos)
	{
        if(m_HandleMouseFunc)
            m_HandleMouseFunc.call(dt, xpos, ypos);

        if(m_HandleScrollFunc)
            m_HandleScrollFunc.call(Input::GetInput()->GetScrollOffset(), dt);  
	}

	void LuaCameraController::HandleKeyboard(float dt)
	{
        if(m_HandleKeyboardFunc)
            m_HandleKeyboardFunc.call(dt);
	}
}
