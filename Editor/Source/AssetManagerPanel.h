#pragma once

#include "EditorPanel.h"

namespace Lumos
{
    class AssetManagerPanel : public EditorPanel
    {
    public:
        AssetManagerPanel();
        ~AssetManagerPanel() = default;

        void OnImGui() override;
    };
}
