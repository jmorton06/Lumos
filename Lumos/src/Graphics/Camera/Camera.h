#pragma once
#include "lmpch.h"

#include "CameraController.h"

#include "Maths/Maths.h"
#include "Maths/Ray.h"

#include <cereal/cereal.hpp>

namespace Lumos
{
	enum RenderPath
	{
		Forward,
		Deferred,
		Batch2D
	};

	enum RenderTarget
	{
		Texture,
		Display0
	};

	enum class ControllerType
	{
		FPS,
		ThirdPerson,
		Maya,
		Simple,
		Camera2D,
		EditorCamera,
		Custom
	};

	class LUMOS_EXPORT Camera
	{
	public:
		Camera() = default;
		Camera(float FOV, float Near, float Far, float aspect);
		Camera(float pitch, float yaw, const Maths::Vector3& position, float FOV, float Near, float Far, float aspect);
		Camera(float aspectRatio, float scale);

		~Camera() = default;
		void OnImGui();

		const Maths::Vector3& GetPosition() const
		{
			return m_Position;
		}
		void SetPosition(const Maths::Vector3& val)
		{
			m_Position = val;
			m_ViewDirty = true;
			m_FrustumDirty = true;
		}

		void SetMouseSensitivity(float value)
		{
			m_MouseSensitivity = value;
		}

		void SetIsOrthographic(bool ortho)
		{
			m_FrustumDirty = true;
			m_ProjectionDirty = true;
			m_Orthographic = ortho;
		}
		bool IsOrthographic() const
		{
			return m_Orthographic;
		}

		float GetRoll() const
		{
			return m_Roll;
		}
		void SetRoll(float y)
		{
			m_ViewDirty = true;
			m_FrustumDirty = true;
			m_Roll = y;
		}

		float GetYaw() const
		{
			return m_Yaw;
		}
		void SetYaw(float y)
		{
			m_Yaw = y;
			m_ViewDirty = true;
			m_FrustumDirty = true;
		}

		float GetPitch() const
		{
			return m_Pitch;
		}
		void SetPitch(float p)
		{
			m_Pitch = p;
			m_ViewDirty = true;
			m_FrustumDirty = true;
		}

		float GetAspectRatio() const
		{
			return m_AspectRatio;
		}
		void SetAspectRatio(float y)
		{
			m_AspectRatio = y;
			m_ProjectionDirty = true;
			m_FrustumDirty = true;
		};

		const Maths::Matrix4& GetProjectionMatrix();
		const Maths::Matrix4& GetViewMatrix();

		Maths::Quaternion GetOrientation() const;

		float GetFar() const
		{
			return m_Far;
		}
		float GetNear() const
		{
			return m_Near;
		}
		float GetFOV() const
		{
			return m_Fov;
		}

		float GetScale() const
		{
			return m_Scale;
		};
		void SetScale(float scale)
		{
			m_Scale = scale;
			m_ProjectionDirty = true;
			m_FrustumDirty = true;
		}

		Maths::Vector3 GetUpDirection() const;
		Maths::Vector3 GetRightDirection() const;
		Maths::Vector3 GetForwardDirection() const;

		Maths::Frustum& GetFrustum();

		Maths::Ray GetScreenRay(float x, float y, bool invertY = false) const;

		void SetCameraController(const Ref<CameraController>& controller)
		{
			m_CameraController = controller;
		}

		const Ref<CameraController>& GetController() const
		{
			return m_CameraController;
		}

		void SetCameraControllerType(ControllerType type);

		template<typename Archive>
		void save(Archive& archive) const
		{
			archive(cereal::make_nvp("Position", m_Position), cereal::make_nvp("Pitch", m_Pitch), cereal::make_nvp("Yaw", m_Yaw), cereal::make_nvp("Roll", m_Roll), cereal::make_nvp("Scale", m_Scale), cereal::make_nvp("Aspect", m_AspectRatio), cereal::make_nvp("FOV", m_Fov), cereal::make_nvp("Near", m_Near), cereal::make_nvp("Far", m_Far), cereal::make_nvp("ControllerType", m_ControllerType));
		}

		template<typename Archive>
		void load(Archive& archive)
		{
			archive(cereal::make_nvp("Position", m_Position), cereal::make_nvp("Pitch", m_Pitch), cereal::make_nvp("Yaw", m_Yaw), cereal::make_nvp("Roll", m_Roll), cereal::make_nvp("Scale", m_Scale), cereal::make_nvp("Aspect", m_AspectRatio), cereal::make_nvp("FOV", m_Fov), cereal::make_nvp("Near", m_Near), cereal::make_nvp("Far", m_Far), cereal::make_nvp("ControllerType", m_ControllerType));

			SetCameraControllerType(m_ControllerType);
			m_FrustumDirty = true;
			m_ProjectionDirty = true;
			m_ViewDirty = true;
		}

	protected:
		void UpdateViewMatrix();
		void UpdateProjectionMatrix();

		float m_Pitch;
		float m_Yaw;
		float m_Roll;

		Maths::Vector3 m_Position = Maths::Vector3(0.0f);

		float m_AspectRatio;
		float m_Scale = 1.0f;
		float m_Zoom = 1.0f;

		Maths::Vector2 m_ProjectionOffset = Maths::Vector2(0.0f, 0.0f);

		Maths::Matrix4 m_ProjMatrix;
		Maths::Matrix4 m_ViewMatrix;

		Maths::Frustum m_Frustum;
		bool m_FrustumDirty = true;
		bool m_ProjectionDirty = false;
		bool m_ViewDirty = false;
		bool customProjection_ = false;

		float m_Fov, m_Near, m_Far;
		float m_MouseSensitivity = 0.1f;

		bool m_Orthographic = false;
		RenderPath m_RenderPath = RenderPath::Deferred;
		bool m_CastShadow = true;
		ControllerType m_ControllerType = ControllerType::ThirdPerson;
		RenderTarget m_Target = RenderTarget::Display0;

		Ref<CameraController> m_CameraController;
	};

}
