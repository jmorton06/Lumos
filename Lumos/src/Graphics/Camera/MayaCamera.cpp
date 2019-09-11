#include "lmpch.h"
#include "MayaCamera.h"
#include "App/Application.h"
#include "Core/OS/Input.h"
#include "Core/OS/Window.h"

#include <imgui/imgui.h>

namespace Lumos
{

	MayaCamera::MayaCamera(float FOV, float Near, float Far, float aspect)
		: Camera(FOV, Near, Far, aspect), m_Free(false)
	{
		Application::Instance()->GetWindow()->HideMouse(false);

		m_PanSpeed = 0.002f;
		m_RotationSpeed = 1.2f;
		m_ZoomSpeed = 0.6f;

		m_Position = Maths::Vector3(-3.0f, 10.0f, 15.0f);

		m_FocalPoint = Maths::Vector3::Zero();
		Maths::Vector3 distance = (m_Position - m_FocalPoint);
		m_Distance = distance.Length();// m_Position.Distance(m_FocalPoint);

		m_ProjMatrix = Maths::Matrix4::Perspective(m_Near, m_Far, aspect, FOV);
		m_RotateDampeningFactor = 0.00001f;
	}

	MayaCamera::MayaCamera(float pitch, float yaw, const Maths::Vector3& position, float FOV, float Near, float Far, float aspect)
		: Camera(pitch, yaw, position, FOV, Near, Far, aspect), m_Free(false)
	{
		Application::Instance()->GetWindow()->HideMouse(false);

		m_PanSpeed = 0.002f;
		m_RotationSpeed = 1.2f;
		m_ZoomSpeed = 0.6f;

		m_FocalPoint = Maths::Vector3::Zero();
		Maths::Vector3 distance = (m_Position - m_FocalPoint);
		m_Distance = distance.Length();// m_Position.Distance(m_FocalPoint);

		m_ProjMatrix = Maths::Matrix4::Perspective(m_Near, m_Far, aspect, FOV);
		m_RotateDampeningFactor = 0.00001f;
	}

	MayaCamera::~MayaCamera()
	{
	}

	void MayaCamera::HandleMouse(float dt, float xpos, float ypos)
	{
		const Maths::Vector2 delta = (Maths::Vector2(xpos, ypos) - m_PreviousCurserPos);

		if (Input::GetInput()->GetMouseHeld(LUMOS_MOUSE_MIDDLE))
			MousePan(delta);
		else if (Input::GetInput()->GetMouseHeld(LUMOS_MOUSE_LEFT))
			MouseRotate(delta, dt);
		else if (Input::GetInput()->GetMouseHeld(LUMOS_MOUSE_RIGHT))
			MouseZoom(delta.GetY(), dt);

		float yawSign = GetUpDirection().GetY() < 0 ? -1.0f : 1.0f;
		m_Yaw -= yawSign * m_RotateVelocity.GetX()  * dt;
		m_Pitch -= m_RotateVelocity.GetY()  * dt;

		m_Distance -= m_ZoomVelocity * dt;
		m_FocalPoint += GetForwardDirection() * m_ZoomVelocity * dt;

		m_ZoomVelocity = m_ZoomVelocity * pow(m_ZoomDampeningFactor, dt);
		m_RotateVelocity = m_RotateVelocity * pow(m_RotateDampeningFactor, dt);

		if (m_Yaw < 0)
			m_Yaw += 360.0f;

		if (m_Yaw > 360.0f)
			m_Yaw -= 360.0f;

		m_PreviousCurserPos = Maths::Vector2(xpos, ypos);
		m_Position = CalculatePosition();
	}

	void MayaCamera::HandleKeyboard(float dt)
	{
		//	Camera::HandleKeyboard(dt);
	}

	void MayaCamera::MousePan(const Maths::Vector2& delta)
	{
		m_FocalPoint -= -GetRightDirection() * delta.GetX() * m_PanSpeed * m_Distance;
		m_FocalPoint -= GetUpDirection() * delta.GetY() * m_PanSpeed * m_Distance;
	}

	void MayaCamera::MouseRotate(const Maths::Vector2& delta, const float dt)
	{
		m_RotateVelocity = m_RotateVelocity + delta *  m_RotationSpeed;
	}

	void MayaCamera::MouseZoom(float delta, const float dt)
	{
		m_ZoomVelocity = m_ZoomVelocity + delta * m_ZoomSpeed;
	}

	void MayaCamera::OnImGui()
	{
		if (ImGui::TreeNode("Maya Camera"))
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
