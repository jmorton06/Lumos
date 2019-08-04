#pragma once
#include "Graphics/API/RenderDevice.h"

namespace Lumos
{
    namespace Graphics
    {
        class VKRenderDevice : public RenderDevice
        {
        public:
            VKRenderDevice() = default;
            ~VKRenderDevice() = default;
            
            void Init() override;
        private:
            
        };
    }
}
