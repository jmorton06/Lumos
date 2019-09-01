#pragma once
#include "LM.h"

namespace Lumos
{
    namespace Graphics
    {
        class CommandBuffer;

        class LUMOS_EXPORT IMGUIRenderer
        {
        public:
            static IMGUIRenderer* Create(u32 width, u32 height, bool clearScreen);

            virtual ~IMGUIRenderer() = default;
            virtual void Init() = 0;
            virtual void NewFrame() = 0;
            virtual void Render(CommandBuffer* commandBuffer) = 0;
            virtual void OnResize(u32 width, u32 height) = 0;
			virtual void Clear() {}
			virtual bool Implemented() const = 0;
            
        protected:
            static IMGUIRenderer* (*CreateFunc)(u32, u32, bool);
        };
    }
}
