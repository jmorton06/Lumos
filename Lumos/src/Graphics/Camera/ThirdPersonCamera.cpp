#include "LM.h"
#include "ThirdPersonCamera.h"
#include "App/Application.h"
#include "App/Input.h"
#include "App/Window.h"

#include <imgui/imgui.h>

namespace Lumos
{

	ThirdPersonCamera::ThirdPersonCamera(float FOV, float Near, float Far, float aspect)
		: Camera(FOV, Near, Far, aspect), m_Free(false)
	{
		Application::Instance()->GetWindow()->HideMouse(false);
		m_ProjMatrix = Maths::Matrix4::Perspective(m_Near, m_Far, aspect, FOV);
		m_RotateDampeningFactor = 0.0f;
	}

	ThirdPersonCamera::ThirdPersonCamera(float pitch, float yaw, const Maths::Vector3& position, float FOV, float Near, float Far, float aspect)
		: Camera(pitch, yaw, position, FOV, Near, Far, aspect), m_Free(false)
	{
		Application::Instance()->GetWindow()->HideMouse(false);
		m_ProjMatrix = Maths::Matrix4::Perspective(m_Near, m_Far, aspect, FOV);
		m_RotateDampeningFactor = 0.0f;
	}

	ThirdPersonCamera::~ThirdPersonCamera()
	{
	}

	void ThirdPersonCamera::HandleMouse(float dt, float xpos, float ypos)
	{
		//if (Input::GetInput().m_UpdateCamera)
		{
			//if (!Input::GetInput().firstUpdate)
            if (Input::GetInput().GetMouseHeld(InputCode::MouseKey::ButtonRight))
			{
				m_RotateVelocity = m_RotateVelocity + Maths::Vector2((xpos - m_PreviousCurserPos.GetX()), (ypos - m_PreviousCurserPos.GetY())) *  m_MouseSensitivity;
				m_Pitch -= m_RotateVelocity.GetY();
				m_Yaw -= m_RotateVelocity.GetX();

				//m_Pitch = Maths::Min(m_Pitch, 90.0f);
				//m_Pitch = Maths::Max(m_Pitch, -90.0f);

				if (m_Yaw < 0)
				{
					m_Yaw += 360.0f;
				}
				if (m_Yaw > 360.0f)
				{
					m_Yaw -= 360.0f;
				}
			}
			/*else
			{
			Input::GetInput().firstUpdate = false;
			}*/

			m_PreviousCurserPos = Maths::Vector2(xpos, ypos);
		}

		m_RotateVelocity = m_RotateVelocity * pow(m_RotateDampeningFactor, dt);
		UpdateScroll(Input::GetInput().GetScrollOffset(), dt);
		Input::GetInput().SetScrollOffset(0.0f);
	}

	void ThirdPersonCamera::HandleKeyboard(float dt)
	{
		m_CameraSpeed = 1000.0f * dt;

        if (Input::GetInput().GetKeyHeld(InputCode::Key::W))
		{
			m_Velocity += GetForwardDirection() * m_CameraSpeed;
		}

        if (Input::GetInput().GetKeyHeld(InputCode::Key::S))
		{
			m_Velocity -= GetForwardDirection() * m_CameraSpeed;
		}

        if (Input::GetInput().GetKeyHeld(InputCode::Key::A))
		{
			m_Velocity -= GetRightDirection() * m_CameraSpeed;
		}

        if (Input::GetInput().GetKeyHeld(InputCode::Key::D))
		{
			m_Velocity += GetRightDirection() * m_CameraSpeed;
		}

        if (Input::GetInput().GetKeyHeld(InputCode::Key::Space))
		{
			m_Velocity -= GetUpDirection() * m_CameraSpeed;
		}

        if (Input::GetInput().GetKeyHeld(InputCode::Key::LeftShift))
		{
			m_Velocity += GetUpDirection() * m_CameraSpeed;
		}

		m_Position += m_Velocity * dt;
		m_Velocity = m_Velocity * pow(m_DampeningFactor, dt);
	}

	void ThirdPersonCamera::OnImGui()
	{
		if (ImGui::TreeNode("Third Person Camera"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Position");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::DragFloat3("##Position", &m_Position.x);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Aspect");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::DragFloat("##Aspect", &m_AspectRatio);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Pitch");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::DragFloat("##Pitch", &m_Pitch);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Yaw");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::DragFloat3("##Yaw", &m_Yaw);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Fov");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::DragFloat("##Fov", &m_Fov);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Near");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::DragFloat("##Near", &m_Near);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Far");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::DragFloat("##Far", &m_Far);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("MouseSensitivity");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::InputFloat("##MouseSensitivity", &m_MouseSensitivity);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("ZoomDampeningFactor");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::InputFloat("##ZoomDampeningFactor", &m_ZoomDampeningFactor);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("DampeningFactor");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::InputFloat("##DampeningFactor", &m_DampeningFactor);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("RotateDampeningFactor");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::InputFloat("##RotateDampeningFactor", &m_RotateDampeningFactor);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::PopStyleVar();

			ImGui::TreePop();
		}
	}
}
