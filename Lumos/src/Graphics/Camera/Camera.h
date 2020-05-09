#pragma once
#include "lmpch.h"

#include "CameraController.h"

#include "Maths/Maths.h"
#include "Maths/Ray.h"

namespace Lumos
{
	class LUMOS_EXPORT Camera
	{
	public:
		Camera(float FOV, float Near, float Far, float aspect);
		Camera(float pitch, float yaw, const Maths::Vector3& position, float FOV, float Near, float Far, float aspect);
		Camera(float aspectRatio, float scale);

        ~Camera() = default;
        void OnImGui();

		const Maths::Vector3& GetPosition() const { return m_Position; }
		void SetPosition(const Maths::Vector3& val)
		{
			m_Position = val; 
			m_ViewDirty = true;
			m_FrustumDirty = true;
		}

		void SetMouseSensitivity(float value) { m_MouseSensitivity = value; }
		
        void SetIsOrthographic(bool ortho) { m_Orthographic = ortho; }
		bool IsOrthographic() const { return m_Orthographic; }

		float GetRoll() const { return m_Roll; }
		void SetRoll(float y) 
		{ 
			m_ViewDirty = true;
			m_FrustumDirty = true;
			m_Roll = y; 
		}

		float GetYaw() const { return m_Yaw; }
		void SetYaw(float y) 
		{ 
			m_Yaw = y; 
			m_ViewDirty = true;
			m_FrustumDirty = true;
		}

		float GetPitch() const { return m_Pitch; }
		void SetPitch(float p) 
		{ 
			m_Pitch = p; 
			m_ViewDirty = true;
			m_FrustumDirty = true;
		}
		
		float GetAspectRatio() const { return m_AspectRatio; }
		void SetAspectRatio(float y) 
		{ 
			m_AspectRatio = y;
			m_ProjectionDirty = true;
			m_FrustumDirty = true;
		};

		const Maths::Matrix4& GetProjectionMatrix();
		const Maths::Matrix4& GetViewMatrix();
		const Maths::Vector3& GetVelocity() const { return m_Velocity; }

		Maths::Quaternion GetOrientation() const;

		float GetFar() const { return m_Far; }
		float GetNear() const { return m_Near; }
		float GetFOV() const { return m_Fov; }

		float GetScale() const { return m_Scale; };
		void  SetScale(float scale)
		{ 
			m_Scale = scale; 
			m_ProjectionDirty = true;
			m_FrustumDirty = true;
		}
		
		Maths::Vector3 GetUpDirection() const;
		Maths::Vector3 GetRightDirection() const;
		Maths::Vector3 GetForwardDirection() const;
		Maths::Vector3 CalculatePosition() const;

        Maths::Frustum& GetFrustum();
    
        Maths::Ray GetScreenRay(float x, float y) const;
    
        void SetCameraController(const Ref<CameraController>& controller) { m_CameraController = controller; }
    
        const Ref<CameraController>& GetController() const { return m_CameraController; }

	protected:

		void UpdateViewMatrix();
		void UpdateProjectionMatrix();

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
		float m_Scale = 1.0f;
		float m_Zoom = 1.0f;

		Maths::Vector2 m_ProjectionOffset = Maths::Vector2(0.0f, 0.0f);

		Maths::Vector2 m_PreviousCurserPos = Maths::Vector2(0.0f, 0.0f);

		Maths::Matrix4 m_ProjMatrix;
		Maths::Matrix4 m_ViewMatrix;

		Maths::Frustum m_Frustum;
		bool m_FrustumDirty = true;
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
    
        Ref<CameraController> m_CameraController;
	};

}
