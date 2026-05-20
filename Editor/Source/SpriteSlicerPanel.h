#pragma once

#include "EditorPanel.h"
#include <Lumos/Core/Reference.h>
#include <string>

namespace Lumos
{
    namespace Graphics
    {
        class Texture2D;
    }

    class SpriteSlicerPanel : public EditorPanel
    {
    public:
        SpriteSlicerPanel();
        ~SpriteSlicerPanel() = default;

        void OnImGui() override;

    private:
        void LoadTexture(const std::string& path);
        void GenerateFrames();

        SharedPtr<Graphics::Texture2D> m_Texture;
        std::string m_TexturePath;

        int m_Rows         = 4;
        int m_Cols         = 4;
        int m_FrameCount   = 16; // optional cap; 0 means rows*cols
        float m_FrameTime  = 0.1f;
        bool m_PingPong    = false;
        std::string m_StateName = "default";

        // Generated frames (UV cell coordinates).
        std::string m_GeneratedSummary;
    };
}
