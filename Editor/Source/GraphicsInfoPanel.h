#pragma once

#include "EditorPanel.h"

namespace Lumos
{
    class GraphicsInfoPanel : public EditorPanel
    {
    public:
        GraphicsInfoPanel();
        ~GraphicsInfoPanel() = default;

        void OnImGui() override;
    };
}
