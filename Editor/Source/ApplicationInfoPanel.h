#pragma once

#include "EditorPanel.h"

namespace Lumos
{
    class ApplicationInfoPanel : public EditorPanel
    {
    public:
        ApplicationInfoPanel();
        ~ApplicationInfoPanel() = default;

        void OnImGui() override;
    };
}
