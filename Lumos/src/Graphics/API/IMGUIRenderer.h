#pragma once
#include "LM.h"

namespace Lumos
{
    namespace graphics
    {
        namespace api
        {
            class CommandBuffer;

            class LUMOS_EXPORT IMGUIRenderer
            {
            public:
                static IMGUIRenderer* Create(uint width, uint height);

                virtual ~IMGUIRenderer() = default;
                virtual void Init() = 0;
                virtual void NewFrame() = 0;
                virtual void Render(CommandBuffer* commandBuffer) = 0;

                bool Implemented() const { return m_Implemented; }

            protected:
                bool m_Implemented = false;
            };
        }
    }
}
