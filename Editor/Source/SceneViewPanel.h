#pragma once
#include "EditorPanel.h"
#include "Editor.h"
#include <Lumos/Maths/Transform.h>

namespace Lumos
{
    // The Scene view's render target: the editor camera's picture, at the size the editor UI shows it.
    class SceneViewPanel : public EditorPanel
    {
    public:
        SceneViewPanel();
        ~SceneViewPanel() = default;

        void OnNewScene(Scene* scene) override;
        void Resize(uint32_t width, uint32_t height);
        // Render resolution relative to the panel (times the quality scale).
        void SetViewScale(float scale) { m_ViewScale = scale; }
        uint32_t GetRenderWidth() const;
        uint32_t GetRenderHeight() const;
        Graphics::Texture2D* GetGameViewTexture() const { return m_GameViewTexture.get(); }

    private:
        SharedPtr<Graphics::Texture2D> m_GameViewTexture = nullptr;
        Scene* m_CurrentScene                            = nullptr;
        uint32_t m_Width = 0, m_Height = 0;
        float m_ViewScale = 1.0f, m_AppliedScale = 1.0f;
    };
}
