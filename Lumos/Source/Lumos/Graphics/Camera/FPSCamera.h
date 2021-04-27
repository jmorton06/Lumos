#pragma once

#include "CameraController.h"

namespace Lumos
{

    class LUMOS_EXPORT FPSCameraController : public CameraController
    {
    public:
        FPSCameraController();
        virtual ~FPSCameraController() override;

        virtual void HandleMouse(Maths::Transform& transform, float dt, float xpos, float ypos) override;
        virtual void HandleKeyboard(Maths::Transform& transform, float dt) override;
        void UpdateScroll(Maths::Transform& transform, float offset, float dt) override {};
    };

}
