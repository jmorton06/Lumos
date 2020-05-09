#pragma once
#include "lmpch.h"
#include "CameraController.h"

namespace Lumos
{

	class LUMOS_EXPORT MayaCameraController : public CameraController
	{
	public:
		MayaCameraController(Camera* camera);
		virtual ~MayaCameraController() override;

		virtual void HandleMouse(float dt, float xpos, float ypos) override;
		virtual void HandleKeyboard(float dt) override;

		void MousePan(const Maths::Vector2& delta);
		void MouseRotate(const Maths::Vector2& delta, const float dt);
		void MouseZoom(float delta, const float dt);

	private:
		float m_PanSpeed, m_RotationSpeed, m_ZoomSpeed;
		bool m_Free;
	};
}

