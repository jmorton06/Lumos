#pragma once
#include "LM.h"
#include "Camera.h"

namespace Lumos
{

	class LUMOS_EXPORT Camera2D : public Camera
	{
	public:
		Camera2D(uint width, uint height, float scale);
		virtual ~Camera2D() override;

		virtual void HandleMouse(float dt, float xpos, float ypos) override;
		virtual void HandleKeyboard(float dt) override;

		virtual void UpdateProjectionMatrix(float width, float height) override;
		void BuildViewMatrix() override;
        
        void  SetScale(float scale);
        float GetScale() const override;
        
        void UpdateScroll(float offset, float dt) override;
    private:
		float m_Scale;
	};
}

