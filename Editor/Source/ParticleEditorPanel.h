#pragma once

#include "EditorPanel.h"

namespace Lumos
{
    class ParticleEditorPanel : public EditorPanel
    {
    public:
        ParticleEditorPanel();
        ~ParticleEditorPanel() = default;

        void OnImGui() override;
    };
}
