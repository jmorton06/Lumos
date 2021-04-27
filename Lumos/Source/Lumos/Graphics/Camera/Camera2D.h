#pragma once

#include "CameraController.h"

namespace Lumos
{

    class LUMOS_EXPORT CameraController2D : public CameraController
    {
    public:
        CameraController2D();
        virtual ~CameraController2D() override;

        virtual void HandleMouse(Maths::Transform& transform, float dt, float xpos, float ypos) override;
        virtual void HandleKeyboard(Maths::Transform& transform, float dt) override;

        void UpdateScroll(Maths::Transform& transform, float offset, float dt) override;
    };
}
