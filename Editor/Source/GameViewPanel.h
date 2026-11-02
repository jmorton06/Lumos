#pragma once
#include "EditorPanel.h"
#include "Editor.h"
#include <Lumos/Maths/Frustum.h>
#include <Lumos/Maths/Transform.h>
#include <Lumos/Graphics/Camera/Camera.h>
#include <Lumos/Utilities/StringUtilities.h>
#include <Lumos/Graphics/Renderers/SceneRenderer.h>

namespace Lumos
{
    class GameViewPanel : public EditorPanel
    {
        friend class Editor;

    public:
        GameViewPanel();
        ~GameViewPanel() = default;

        void OnNewScene(Scene* scene) override;
        void OnRender() override;

        void Resize(uint32_t width, uint32_t height);

        // Keeps the
        // game camera's render going at this size and hands back the target to show.
        // A scene with no camera of its own (one whose script makes it on start) is drawn through the editor's
        // camera. fitEditorCamera = this view may set that camera's shape (not when the Scene view is up too).
        Graphics::Texture2D* PrepareExternal(uint32_t width, uint32_t height, bool fitEditorCamera = true);
        // The last PrepareExternal found no camera in the scene and used the editor's.
        bool HasNoCamera() const { return m_NoCamera; }
        void SetExternalVisible(bool visible) { m_ExternalVisible = visible; }
        // Render resolution relative to the panel (times the quality scale).
        void SetViewScale(float scale) { m_ViewScale = scale; }
        uint32_t GetRenderWidth() const { return m_GameViewTexture ? m_GameViewTexture->GetWidth() : 0; }
        uint32_t GetRenderHeight() const { return m_GameViewTexture ? m_GameViewTexture->GetHeight() : 0; }

    private:
        SharedPtr<Graphics::Texture2D> m_GameViewTexture = nullptr;
        Scene* m_CurrentScene                            = nullptr;
        uint32_t m_Width, m_Height;
        float m_ViewScale = 1.0f, m_AppliedScale = 1.0f;
        UniquePtr<Graphics::SceneRenderer> m_SceneRenderer;
        bool m_GameViewVisible = false;
        bool m_NoCamera = false;
        bool m_ExternalVisible = false;
        bool m_ShowStats       = false;
    };
}
