#pragma once
#include "LM.h"
#include "Camera.h"

namespace Lumos
{

	class LUMOS_EXPORT MayaCamera : public Camera
	{
	public:
		MayaCamera(float FOV, float Near, float Far, float aspect);
		MayaCamera(float pitch, float yaw, const Maths::Vector3& position, float FOV, float Near, float Far, float aspect);
		virtual ~MayaCamera() override;

		virtual void HandleMouse(float dt, float xpos, float ypos) override;
		virtual void HandleKeyboard(float dt) override;

		void MousePan(const Maths::Vector2& delta);
		void MouseRotate(const Maths::Vector2& delta, const float dt);
		void MouseZoom(float delta, const float dt);

		float m_PanSpeed, m_RotationSpeed, m_ZoomSpeed;
		bool m_Free;
	};
}

