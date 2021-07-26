#pragma once
#include "Graphics/RHI/RenderDevice.h"

namespace Lumos
{
    namespace Graphics
    {
        class GLRenderDevice : public RenderDevice
        {
        public:
            GLRenderDevice() = default;
            ~GLRenderDevice() = default;

            void Init() override;

            static void MakeDefault();

        protected:
            static RenderDevice* CreateFuncGL();
        };
    }
}
