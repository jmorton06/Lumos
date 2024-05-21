#pragma once
#include <Lumos/Maths/Ray.h>
#include <Lumos/Maths/Transform.h>
#include <Lumos/Utilities/IniFile.h>
#include <Lumos/Graphics/Camera/EditorCamera.h>
#include <Lumos/Graphics/Camera/Camera.h>
#include <Lumos/ImGui/ImGuiUtilities.h>
#include <Lumos/Core/Application.h>
#include <Lumos/Scene/Entity.h>

namespace Lumos
{
    namespace Graphics
    {
        class Environment;
        class Texture2D;
        class RenderPasses;
    }

    class Scene;

    class PreviewDraw
    {
        friend class Editor;

    public:
        PreviewDraw();
        ~PreviewDraw();

        void Draw();
        void LoadMesh(String8 path);
        void LoadMaterial(String8 path);
        void SetDimensions(u32 width, u32 height);
        void CreateDefaultScene();
        void SaveTexture(String8 savePath);
        void DeletePreviewModel();

        void ReleaseResources();

    private:
        Entity m_PreviewObjectEntity;
        Entity m_CameraEntity;

        Scene* m_PreviewScene = nullptr;
        SharedPtr<Graphics::RenderPasses> m_PreviewRenderer;
        SharedPtr<Graphics::Texture2D> m_PreviewTexture;

        u32 m_Width  = 256;
        u32 m_Height = 256;
    };

}
