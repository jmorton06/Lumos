#pragma once
#include "LM.h"
#include "Camera.h"

namespace Lumos
{

	class LUMOS_EXPORT Camera2D : public Camera
	{
	public:
		Camera2D(float FOV, float Near, float Far, float aspect, int scale);
		Camera2D(uint width, uint height, float aspect, int scale);
		Camera2D(float pitch, float yaw, const maths::Vector3& position, float FOV, float Near, float Far, float aspect, int scale);
		virtual ~Camera2D() override;

		virtual void HandleMouse(float dt, float xpos, float ypos) override;
		virtual void HandleKeyboard(float dt) override;

		virtual void UpdateProjectionMatrix(float width, float height) override;
		void BuildViewMatrix() override;

		int m_Scale;

		void  SetScale(int scale);
		int GetScale() const override;
	};
}

