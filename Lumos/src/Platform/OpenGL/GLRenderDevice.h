#pragma once
#include "Graphics/API/RenderDevice.h"

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
        private:
            
        };
    }
}
