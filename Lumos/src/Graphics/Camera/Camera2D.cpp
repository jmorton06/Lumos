#include "lmpch.h"
#include "Camera2D.h"
#include "App/Application.h"
#include "Core/OS/Input.h"
#include "Maths/Maths.h"

#include "Core/OS/Window.h"

#include <imgui/imgui.h>

namespace Lumos
{
    Camera2D::Camera2D(float aspectRatio, float scale) : Camera(45.0f, 0.0f, 1.0f, 1.0f)
		, m_Scale(scale), m_AspectRatio(aspectRatio)
	{
		Application::Instance()->GetWindow()->HideMouse(false);
		m_ProjMatrix = Maths::Matrix4::Orthographic(-m_AspectRatio * m_Scale, m_AspectRatio * m_Scale, -m_Scale, m_Scale, -1.0f, 1.0f);
		m_Position = Maths::Vector3(0.0f);
		m_Velocity = Maths::Vector3(0.0f);
		m_MouseSensitivity = 0.005f;
        BuildViewMatrix();
	}

	Camera2D::~Camera2D()
	{
	}

	void Camera2D::HandleMouse(float dt, float xpos, float ypos)
	{
		if (Input::GetInput()->GetMouseHeld(InputCode::MouseKey::ButtonRight))
		{
			m_Position.x -= (xpos - m_PreviousCurserPos.x) * m_Scale * m_MouseSensitivity * 0.5f;
			m_Position.y += (ypos - m_PreviousCurserPos.y) * m_Scale * m_MouseSensitivity * 0.5f;
		}

		m_PreviousCurserPos = Maths::Vector2(xpos, ypos);
	}

	void Camera2D::HandleKeyboard(float dt)
	{
		Maths::Vector3 up = Maths::Vector3(0, 1, 0), right = Maths::Vector3(1, 0, 0);

		m_CameraSpeed = m_Scale * dt * 20.0f;

		if (Input::GetInput()->GetKeyHeld(LUMOS_KEY_A))
		{
			m_Velocity -= right * m_CameraSpeed;
		}

		if (Input::GetInput()->GetKeyHeld(LUMOS_KEY_D))
		{
			m_Velocity += right * m_CameraSpeed;
		}

		if (Input::GetInput()->GetKeyHeld(LUMOS_KEY_W))
		{
			m_Velocity += up * m_CameraSpeed;
		}

		if (Input::GetInput()->GetKeyHeld(LUMOS_KEY_S))
		{
			m_Velocity -= up * m_CameraSpeed;
		}

		m_Position += m_Velocity * dt;
		m_Velocity = m_Velocity * pow(m_DampeningFactor, dt);
        
        UpdateScroll(Input::GetInput()->GetScrollOffset(), dt);
		Input::GetInput()->SetScrollOffset(0.0f);
	}

	void Camera2D::UpdateProjectionMatrix()
	{
		m_ProjMatrix = Maths::Matrix4::Orthographic(-m_AspectRatio * m_Scale, m_AspectRatio * m_Scale, -m_Scale, m_Scale, -1.0f, 1.0f);
	}

	void Camera2D::BuildViewMatrix()
	{
		m_ViewMatrix = Maths::Matrix4::Translation(-m_Position);
	}

	float Camera2D::GetScale() const
	{
		return m_Scale;
	}

	void Camera2D::SetScale(float scale)
	{
		m_Scale = scale;
	}
    
    void Camera2D::UpdateScroll(float offset, float dt)
    {
		float multiplier = 2.0f;
		if (Input::GetInput()->GetKeyHeld(InputCode::Key::LeftShift))
		{
			multiplier = 10.0f;
		}

        if (offset != 0.0f)
        {
            m_ZoomVelocity += dt * offset * multiplier;
        }
        
        m_Scale -= m_ZoomVelocity;
        m_Scale = Lumos::Maths::Max(m_Scale, 0.15f);
        m_ZoomVelocity = m_ZoomVelocity * pow(m_ZoomDampeningFactor, dt);
    }

	void Camera2D::OnImGui()
	{
		if (ImGui::TreeNode("Camera2D"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted("Position");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::DragFloat3("##Position", &m_Position.x);
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
			ImGui::TextUnformatted("Scale");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::DragFloat("##Scale", &m_Scale);
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::PopStyleVar();

			ImGui::TreePop();
		}
	}
}
