#pragma once

#include "Core.h"
#include "Layer3D.h"

namespace Lumos
{
    namespace graphics
    {
        namespace api
        {
            class IMGUIRenderer;
        }
    }
    class IMGUIRenderer;

    class LUMOS_EXPORT Layer3DDeferred : public Layer3D
    {
    public:
        Layer3DDeferred(uint width, uint height, const std::string& name = "Layer3DDeferred");
        ~Layer3DDeferred();

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(TimeStep* dt) override;
        void OnEvent(Event& event) override;
        void OnRender(Scene* scene) override;
        void OnNewScene(Scene* scene) override { m_Scene = scene; };
    };
}
