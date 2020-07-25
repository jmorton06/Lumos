#include "lmpch.h"
#include "LuaCameraController.h"
#include "Core/Application.h"
#include "Core/OS/Input.h"
#include "Core/OS/Window.h"
#include "Camera.h"

#include <imgui/imgui.h>

namespace Lumos
{

	LuaCameraController::LuaCameraController()
	{
	}

	LuaCameraController::~LuaCameraController() = default;

	void LuaCameraController::HandleMouse(Camera* camera, float dt, float xpos, float ypos)
	{
		if(m_HandleMouseFunc)
			m_HandleMouseFunc.call(camera, dt, xpos, ypos);

		if(m_HandleScrollFunc)
			m_HandleScrollFunc.call(camera, Input::GetInput()->GetScrollOffset(), dt);
	}

	void LuaCameraController::HandleKeyboard(Camera* camera, float dt)
	{
		if(m_HandleKeyboardFunc)
			m_HandleKeyboardFunc.call(camera, dt);
	}
}
