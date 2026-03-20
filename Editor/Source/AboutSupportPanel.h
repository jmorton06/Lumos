#pragma once

#include "EditorPanel.h"

namespace Lumos
{
    class AboutSupportPanel : public EditorPanel
    {
    public:
        AboutSupportPanel();
        ~AboutSupportPanel() = default;

        void OnImGui() override;
    };
}
