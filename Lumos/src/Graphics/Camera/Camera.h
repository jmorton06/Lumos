#pragma once
#include "lmpch.h"

#include "Maths/Maths.h"

namespace Lumos
{
	class LUMOS_EXPORT Camera
	{
	public:
		Camera(float FOV, float Near, float Far, float aspect);
		Camera(float pitch, float yaw, const Maths::Vector3& position, float FOV, float Near, float Far, float aspect);
		Camera(float aspectRatio, float scale);

		virtual ~Camera() = default;

		virtual void HandleMouse(float dt, float xpos, float ypos) = 0;
		virtual void HandleKeyboard(float dt) = 0;

		virtual void BuildViewMatrix();
		virtual void UpdateScroll(float offset, float dt);
		virtual void UpdateProjectionMatrix();

		virtual void OnImGui();
		virtual bool Is2D() const { return false; }

		const Maths::Vector3& GetPosition() const { return m_Position; }
		void SetPosition(const Maths::Vector3& val) { m_Position = val; }

		void SetMouseSensitivity(float value) { m_MouseSensitivity = value; }

		float GetRoll() const { return m_Roll; }
		void SetRoll(float y) { m_Roll = y; }

		float GetYaw() const { return m_Yaw; }
		void SetYaw(float y) 
		{ 
			m_Yaw = y; 
			m_ProjectionDirty = true;
		}

		float GetPitch() const { return m_Pitch; }
		void SetPitch(float p) 
		{ 
			m_Pitch = p; 
			m_ProjectionDirty = true;
		}
		
		float GetAspectRatio() const { return m_AspectRatio; }
		void SetAspectRatio(float y) 
		{ 
			m_AspectRatio = y;
			m_ProjectionDirty = true;
		};

		const Maths::Matrix4& GetProjectionMatrix();
		const Maths::Matrix4& GetViewMatrix() const { return m_ViewMatrix; }
		const Maths::Vector3& GetVelocity() const { return m_Velocity; }

		Maths::Quaternion GetOrientation() const;

		float GetFar() const { return m_Far; }
		float GetNear() const { return m_Near; }
		float GetFOV() const { return m_Fov; }

		void  SetScale(float scale) { m_Scale = scale; }
		float GetScale() const { return m_Scale; };
		
		Maths::Vector3 GetUpDirection() const;
		Maths::Vector3 GetRightDirection() const;
		Maths::Vector3 GetForwardDirection() const;
		Maths::Vector3 CalculatePosition() const;

		const Maths::Frustum& GetFrustum();

	protected:
		float m_Pitch;
		float m_Yaw;
		float m_Roll;

		Maths::Vector3 m_Position;
		Maths::Vector3 m_Velocity;
		Maths::Vector2 m_RotateVelocity;
		Maths::Vector3 m_FocalPoint;

		float m_AspectRatio;
		float m_ZoomVelocity;
		float m_CameraSpeed;
		float m_Distance;
		float m_Scale;
		float m_Zoom = 1.0f;

		Maths::Vector2 m_ProjectionOffset = Maths::Vector2(0.0f, 0.0f);

		Maths::Vector2 m_PreviousCurserPos = Maths::Vector2(0.0f, 0.0f);

		Maths::Matrix4 m_ProjMatrix;
		Maths::Matrix4 m_ViewMatrix;

		Maths::Frustum m_Frustum;
		bool m_FrustumDirty = false;
		bool m_ProjectionDirty = false;
		bool m_ViewDirty = false;
		bool customProjection_ = false;

		float m_Fov, m_Near, m_Far;
		float m_MouseSensitivity = 0.1f;

		float m_ZoomDampeningFactor = 0.00001f;
		float m_DampeningFactor = 0.00001f;
		float m_RotateDampeningFactor = 0.00001f;

		u32 m_ScreenWidth;
		u32 m_ScreenHeight;

		bool m_Orthographic = false;
	};

}
