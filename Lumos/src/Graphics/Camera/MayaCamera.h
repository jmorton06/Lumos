#pragma once
#include "lmpch.h"
#include "CameraController.h"

namespace Lumos
{

	class LUMOS_EXPORT MayaCameraController : public CameraController
	{
	public:
		MayaCameraController();
		virtual ~MayaCameraController() override;

		virtual void HandleMouse(Camera* camera, float dt, float xpos, float ypos) override;
		virtual void HandleKeyboard(Camera* camera, float dt) override;

		void MousePan(Camera* camera, const Maths::Vector2& delta);
		void MouseRotate(const Maths::Vector2& delta, const float dt);
		void MouseZoom(float delta, const float dt);
        Maths::Vector3 CalculatePosition(Camera* camera) const;

	private:
		float m_PanSpeed, m_RotationSpeed, m_ZoomSpeed;
		bool m_Free;
	};
}

