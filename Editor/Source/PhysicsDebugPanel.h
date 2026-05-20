#pragma once

#include "EditorPanel.h"

namespace Lumos
{
    class PhysicsDebugPanel : public EditorPanel
    {
    public:
        PhysicsDebugPanel();
        ~PhysicsDebugPanel() = default;

        void OnImGui() override;
    };
}
