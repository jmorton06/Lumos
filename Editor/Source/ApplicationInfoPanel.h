#pragma once

#include "EditorPanel.h"
#include <vector>

namespace Lumos
{
    class ApplicationInfoPanel : public EditorPanel
    {
    public:
        ApplicationInfoPanel();
        ~ApplicationInfoPanel() = default;

        void OnImGui() override;

        std::vector<float> m_FPSData;
    };
}
