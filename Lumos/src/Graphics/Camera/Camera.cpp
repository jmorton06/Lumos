#include "lmpch.h"
#include "Camera.h"
#include <imgui/imgui.h>

namespace Lumos
{

	Camera::Camera(float FOV, float Near, float Far, float aspect)
		: m_Yaw(0.0f)
		, m_Pitch(0.0f)
		, m_AspectRatio(aspect)
		, m_Position(Maths::Vector3(0.0f, 0.0f, 0.0f))
		, m_Velocity(Maths::Vector3(0.0f))
		, m_RotateVelocity(Maths::Vector2(0.0f))
		, m_ZoomVelocity(0.0f)
		, m_Fov(FOV)
		, m_Near(Near)
		, m_Far(Far)
		, m_Roll(0.0f)
	{
		Camera::BuildViewMatrix();
	};

	Camera::Camera(float pitch, float yaw, const Maths::Vector3& position, float FOV, float Near, float Far, float aspect)
		: m_Yaw(yaw)
		, m_Pitch(pitch)
		, m_AspectRatio(aspect)
		, m_Position(position)
		, m_Velocity(Maths::Vector3(0.0f))
		, m_RotateVelocity(Maths::Vector2(0.0f))
		, m_ZoomVelocity(0.0f)
		, m_Fov(FOV)
		, m_Near(Near)
		, m_Far(Far)
		, m_Roll(0.0f)
	{
		Camera::BuildViewMatrix();
	}

	void Camera::UpdateScroll(float offset, float dt)
	{
		if (offset != 0.0f)
		{
			m_ZoomVelocity += dt * offset * 10.0f;
		}

		m_Position += GetForwardDirection() * m_ZoomVelocity;
		m_ZoomVelocity = m_ZoomVelocity * pow(m_ZoomDampeningFactor, dt);
	}

	void Camera::BuildViewMatrix()
	{
		m_ViewMatrix = Maths::Quaternion::EulerAnglesToQuaternion(-m_Pitch, -m_Yaw, -m_Roll).RotationMatrix4() * Maths::Matrix4::Translation(-m_Position);
	};

	void Camera::UpdateProjectionMatrix(float width, float height)
	{
		m_ProjMatrix = Maths::Matrix4::Perspective(m_Near, m_Far, width / height, m_Fov);
	}

	Maths::Vector3 Camera::GetUpDirection() const
	{
		Maths::Vector3 up = Maths::Vector3::UP;
		up = GetOrientation() * up;
		return up;
	}

	Maths::Vector3 Camera::GetRightDirection() const
	{
		Maths::Vector3 right = Maths::Vector3::RIGHT;
		right = GetOrientation() * right;
		return right;
	}

	Maths::Vector3 Camera::GetForwardDirection() const
	{
		Maths::Vector3 forward = Maths::Vector3::FORWARD;
		forward = GetOrientation() * forward;
		return forward;
	}

	Maths::Vector3 Camera::CalculatePosition() const
	{
		return m_FocalPoint - GetForwardDirection() * m_Distance;
	}

	Maths::Quaternion Camera::GetOrientation() const
	{
		return Maths::Quaternion::EulerAnglesToQuaternion(m_Pitch, m_Yaw, m_Roll);
	}

	void Camera::OnImGui()
	{
		if (ImGui::TreeNode("Camera"))
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
			ImGui::DragFloat("##Yaw", &m_Yaw);
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

			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::PopStyleVar();

			ImGui::TreePop();
		}
	}
}
