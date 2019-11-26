#include "lmpch.h"
#include "ThirdPersonCamera.h"
#include "App/Application.h"
#include "Core/OS/Input.h"
#include "Core/OS/Window.h"

#include <imgui/imgui.h>

namespace Lumos
{

	ThirdPersonCamera::ThirdPersonCamera(float FOV, float Near, float Far, float aspect)
		: Camera(FOV, Near, Far, aspect), m_Free(false)
	{
		m_RotateDampeningFactor = 0.0f;
	}

	ThirdPersonCamera::ThirdPersonCamera(float pitch, float yaw, const Maths::Vector3& position, float FOV, float Near, float Far, float aspect)
		: Camera(pitch, yaw, position, FOV, Near, Far, aspect), m_Free(false)
	{
		m_RotateDampeningFactor = 0.0f;
	}

	ThirdPersonCamera::~ThirdPersonCamera()
	{
	}

	void ThirdPersonCamera::HandleMouse(float dt, float xpos, float ypos)
	{
		{
            if (Input::GetInput()->GetMouseHeld(InputCode::MouseKey::ButtonRight))
			{
				m_RotateVelocity = m_RotateVelocity + Maths::Vector2((xpos - m_PreviousCurserPos.x), (ypos - m_PreviousCurserPos.y)) *  m_MouseSensitivity;
				m_Pitch -= m_RotateVelocity.y;
				m_Yaw -= m_RotateVelocity.x;

				if (m_Yaw < 0)
				{
					m_Yaw += 360.0f;
				}
				if (m_Yaw > 360.0f)
				{
					m_Yaw -= 360.0f;
				}
			}

			m_PreviousCurserPos = Maths::Vector2(xpos, ypos);
		}

		m_RotateVelocity = m_RotateVelocity * pow(m_RotateDampeningFactor, dt);
		UpdateScroll(Input::GetInput()->GetScrollOffset(), dt);
		
		m_ViewDirty = true;
	}

	void ThirdPersonCamera::HandleKeyboard(float dt)
	{
		m_CameraSpeed = 1000.0f * dt;

        if (Input::GetInput()->GetKeyHeld(InputCode::Key::W))
		{
			m_Velocity += GetForwardDirection() * m_CameraSpeed;
		}

        if (Input::GetInput()->GetKeyHeld(InputCode::Key::S))
		{
			m_Velocity -= GetForwardDirection() * m_CameraSpeed;
		}

        if (Input::GetInput()->GetKeyHeld(InputCode::Key::A))
		{
			m_Velocity -= GetRightDirection() * m_CameraSpeed;
		}

        if (Input::GetInput()->GetKeyHeld(InputCode::Key::D))
		{
			m_Velocity += GetRightDirection() * m_CameraSpeed;
		}

        if (Input::GetInput()->GetKeyHeld(InputCode::Key::Space))
		{
			m_Velocity -= GetUpDirection() * m_CameraSpeed;
		}

        if (Input::GetInput()->GetKeyHeld(InputCode::Key::LeftShift))
		{
			m_Velocity += GetUpDirection() * m_CameraSpeed;
		}

		m_Position += m_Velocity * dt;
		m_Velocity = m_Velocity * pow(m_DampeningFactor, dt);

		m_ViewDirty = true;
	}

	void ThirdPersonCamera::OnImGui()
	{
		if (ImGui::TreeNode("Third Person Camera"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Position");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::DragFloat3("##Position", Maths::ValuePointer(m_Position));
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Aspect");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::DragFloat("##Aspect", &m_AspectRatio);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Pitch");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::DragFloat("##Pitch", &m_Pitch);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Yaw");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::DragFloat3("##Yaw", &m_Yaw);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Fov");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::DragFloat("##Fov", &m_Fov);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Near");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::DragFloat("##Near", &m_Near);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Far");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::DragFloat("##Far", &m_Far);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("MouseSensitivity");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::InputFloat("##MouseSensitivity", &m_MouseSensitivity);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("ZoomDampeningFactor");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::InputFloat("##ZoomDampeningFactor", &m_ZoomDampeningFactor);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("DampeningFactor");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::InputFloat("##DampeningFactor", &m_DampeningFactor);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("RotateDampeningFactor");
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
