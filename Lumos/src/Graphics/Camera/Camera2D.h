#pragma once
#include "lmpch.h"
#include "Camera.h"

namespace Lumos
{

	class LUMOS_EXPORT Camera2D : public Camera
	{
	public:
		Camera2D(float aspectRatio, float scale);
		virtual ~Camera2D() override;

		virtual void HandleMouse(float dt, float xpos, float ypos) override;
		virtual void HandleKeyboard(float dt) override;

		virtual void UpdateProjectionMatrix() override;
		void BuildViewMatrix() override;
        
        void  SetScale(float scale);
        float GetScale() const;
        
        void UpdateScroll(float offset, float dt) override;
		void OnImGui() override;
		bool Is2D() const override { return true; }

    private:
		float m_Scale;
		float m_AspectRatio;
	};
}

