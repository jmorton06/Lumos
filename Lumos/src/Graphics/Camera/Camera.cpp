#include "lmpch.h"
#include "Camera.h"
#include "ImGui/ImGuiHelpers.h"

#include <imgui/imgui.h>

namespace Lumos
{

	Camera::Camera(float FOV, float Near, float Far, float aspect)
		: m_AspectRatio(aspect)
		, m_FrustumDirty(true)
		, m_ProjectionDirty(true)
		, m_Fov(FOV)
		, m_Near(Near)
		, m_Far(Far)
		, m_Orthographic(false)
		, m_Scale(1.0f)
	{
	};

	Camera::Camera(float pitch, float yaw, const Maths::Vector3& position, float FOV, float Near, float Far, float aspect)
		: m_AspectRatio(aspect)
		, m_FrustumDirty(true)
		, m_ProjectionDirty(true)
		, m_Fov(FOV)
		, m_Near(Near)
		, m_Far(Far)
		, m_Orthographic(false)
		, m_Scale(1.0f)
	{
	}

	Camera::Camera(float aspectRatio, float scale)
		: m_AspectRatio(aspectRatio)
		, m_Scale(scale)
		, m_FrustumDirty(true)
		, m_ProjectionDirty(true)
		, m_Fov(0)
		, m_Near(-10.0)
		, m_Far(10.0f)
		, m_Orthographic(true)
	{
	}

	void Camera::UpdateProjectionMatrix()
	{
		if(m_Orthographic)
			m_ProjMatrix = Maths::Matrix4::Orthographic(-m_AspectRatio * m_Scale, m_AspectRatio * m_Scale, -m_Scale, m_Scale, m_Near, m_Far, m_ProjectionOffset.x, m_ProjectionOffset.y, m_Zoom);
		else
			m_ProjMatrix = Maths::Matrix4::Perspective(m_Near, m_Far, m_AspectRatio, m_Fov, m_ProjectionOffset.x, m_ProjectionOffset.y, m_Zoom);
	}

	Maths::Frustum& Camera::GetFrustum(const Maths::Matrix4& viewMatrix)
	{
		if(m_ProjectionDirty)
			UpdateProjectionMatrix();

		{
			customProjection_ = true; //temp
			if(customProjection_)
				m_Frustum.Define(m_ProjMatrix * viewMatrix);
			else
			{
				if(m_Orthographic)
					m_Frustum.DefineOrtho(m_Scale, m_AspectRatio, m_Zoom, GetNear(), GetFar(), Maths::Matrix3x4(viewMatrix));
				else
					m_Frustum.Define(m_Fov, m_AspectRatio, m_Zoom, GetNear(), GetFar(), Maths::Matrix3x4(viewMatrix));
			}

			m_FrustumDirty = false;
		}

		return m_Frustum;
	}

	const Maths::Matrix4& Camera::GetProjectionMatrix()
	{
		if(m_ProjectionDirty)
		{
			UpdateProjectionMatrix();
			m_ProjectionDirty = false;
		}
		return m_ProjMatrix;
	}

	Maths::Ray Camera::GetScreenRay(float x, float y, const Maths::Matrix4& viewMatrix, bool invertY) const
	{
		Maths::Ray ret;

		Maths::Matrix4 viewProjInverse = (m_ProjMatrix * viewMatrix).Inverse();

		// The parameters range from 0.0 to 1.0. Expand to normalized device coordinates (-1.0 to 1.0)
		x = 2.0f * x - 1.0f;
		y = 2.0f * y - 1.0f;

		if(invertY)
			y *= -1.0f;
		Maths::Vector3 nearPlane(x, y, 0.0f);
		Maths::Vector3 farPlane(x, y, 1.0f);

		ret.origin_ = viewProjInverse * nearPlane;
		ret.direction_ = ((viewProjInverse * farPlane) - ret.origin_).Normalized();

		return ret;
	}

	void Camera::OnImGui()
	{
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			//ImGuiHelpers::Property("Position", m_Position, -1000.0f, 1000.0f);
			ImGuiHelpers::Property("Aspect", m_AspectRatio, 0.0f, 10.0f);
			//ImGuiHelpers::Property("Pitch", m_Pitch, -360.0f, 360.0f);
			//ImGuiHelpers::Property("Yaw", m_Yaw, -360.0f, 360.0f);
			//ImGuiHelpers::Property("Roll", m_Roll, -360.0f, 360.0f);
			ImGuiHelpers::Property("Fov", m_Fov, 1.0f, 120.0f);
			ImGuiHelpers::Property("Near", m_Near, 0.0f, 10.0f);
			ImGuiHelpers::Property("Far", m_Far, 10.0f, 10000.0f);
			ImGuiHelpers::Property("Zoom", m_Zoom, 0.0f, 100.0f);
			ImGuiHelpers::Property("Offset", m_ProjectionOffset, 0.0f, 10.0f);
			ImGuiHelpers::Property("Scale", m_Scale, 0.0f, 1000.0f);
            ImGuiHelpers::Property("Orthograhic", m_Orthographic);

			m_ProjectionDirty = true;
			m_FrustumDirty = true;

			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::PopStyleVar();
		}
	}
}
