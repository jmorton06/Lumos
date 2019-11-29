#include "lmpch.h"
#include "Camera.h"
#include "ImGui/ImGuiHelpers.h"
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
		, m_Orthographic(false)
		, m_ViewDirty(true)
		, m_ProjectionDirty(true)
		, m_FrustumDirty(true)
	{
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
		, m_Orthographic(false)
		, m_ViewDirty(true)
		, m_ProjectionDirty(true)
		, m_FrustumDirty(true)
	{
	}

	Camera::Camera(float aspectRatio, float scale)
		: m_Scale(scale), m_AspectRatio(aspectRatio), m_Orthographic(true), m_ViewDirty(true) , m_ProjectionDirty(true), m_FrustumDirty(true)
	{
	}

	void Camera::UpdateScroll(float offset, float dt)
	{
		if (offset != 0.0f)
		{
			m_ZoomVelocity -= dt * offset * 10.0f;
		}

		if (!Maths::Equals(m_ZoomVelocity, 0.0f))
		{
			m_Position += GetForwardDirection() * m_ZoomVelocity;
			m_ZoomVelocity = m_ZoomVelocity * pow(m_ZoomDampeningFactor, dt);
			m_ViewDirty = true;
			m_FrustumDirty = true;
		}

	}

	void Camera::UpdateViewMatrix()
	{
		m_ViewMatrix = Maths::Matrix3x4(m_Position, Maths::Quaternion::EulerAnglesToQuaternion(m_Pitch, m_Yaw, m_Roll), Maths::Vector3(1.0f)).Inverse().ToMatrix4();
	};

	void Camera::UpdateProjectionMatrix()
	{
		if(m_Orthographic)
			m_ProjMatrix = Maths::Matrix4::Orthographic(-m_AspectRatio * m_Scale, m_AspectRatio * m_Scale, -m_Scale, m_Scale, m_Near, m_Far, m_ProjectionOffset.x, m_ProjectionOffset.y, m_Zoom);
		else
			m_ProjMatrix = Maths::Matrix4::Perspective(m_Near, m_Far, m_AspectRatio, m_Fov, m_ProjectionOffset.x, m_ProjectionOffset.y, m_Zoom);
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

	const Maths::Frustum& Camera::GetFrustum()
	{
		if (m_ProjectionDirty)
			UpdateProjectionMatrix();

		if (m_FrustumDirty)
		{
			customProjection_ = true; //temp
			if (customProjection_)
				m_Frustum.Define(m_ProjMatrix * m_ViewMatrix);
			else
			{
				if (m_Orthographic)
					m_Frustum.DefineOrtho(m_Scale, m_AspectRatio, m_Zoom, GetNear(), GetFar(), Maths::Matrix3x4(m_ViewMatrix));
				else
					m_Frustum.Define(m_Fov, m_AspectRatio, m_Zoom, GetNear(), GetFar(), Maths::Matrix3x4(m_ViewMatrix));
			}

			m_FrustumDirty = false;
		}

		return m_Frustum;

	}

	const Maths::Matrix4& Camera::GetProjectionMatrix() 
	{ 
		if (m_ProjectionDirty)
		{
			UpdateProjectionMatrix();
			m_ProjectionDirty = false;
		}
		return m_ProjMatrix; 
	}

	const Maths::Matrix4 & Camera::GetViewMatrix()
	{
		if (m_ViewDirty)
		{
			UpdateViewMatrix();
			m_ViewDirty = false;
		}
		return m_ViewMatrix;
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

            ImGuiHelpers::Property("Position", m_Position, -1000.0f, 1000.0f);
			ImGuiHelpers::Property("Aspect", m_AspectRatio, 0.0f, 10.0f);
			ImGuiHelpers::Property("Pitch", m_Pitch, -360.0f, 360.0f);
			ImGuiHelpers::Property("Yaw", m_Yaw, -360.0f, 360.0f);
			ImGuiHelpers::Property("Roll", m_Roll, -360.0f, 360.0f);
			ImGuiHelpers::Property("Fov", m_Fov, 1.0f, 120.0f);
			ImGuiHelpers::Property("Near", m_Near, 0.0f, 10.0f);
			ImGuiHelpers::Property("Far", m_Far, 10.0f, 10000.0f);
			ImGuiHelpers::Property("Zoom", m_Zoom, 0.0f, 100.0f);
			ImGuiHelpers::Property("Offset", m_ProjectionOffset, 0.0f, 10.0f);
			ImGuiHelpers::Property("Scale", m_Scale, 0.0f, 1000.0f);
			ImGui::Checkbox("Orthograhic", &m_Orthographic);
            
            m_ProjectionDirty = true;
			m_ViewDirty = true;
			m_FrustumDirty = true;

			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::PopStyleVar();

			ImGui::TreePop();
		}
	}
}
