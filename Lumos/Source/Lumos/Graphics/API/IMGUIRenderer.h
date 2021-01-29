#pragma once


namespace Lumos
{
    namespace Graphics
    {
        class CommandBuffer;

        class LUMOS_EXPORT IMGUIRenderer
        {
        public:
            static IMGUIRenderer* Create(uint32_t width, uint32_t height, bool clearScreen);

            virtual ~IMGUIRenderer() = default;
            virtual void Init() = 0;
            virtual void NewFrame() = 0;
            virtual void Render(CommandBuffer* commandBuffer) = 0;
            virtual void OnResize(uint32_t width, uint32_t height) = 0;
			virtual void Clear() {}
			virtual bool Implemented() const = 0;
            virtual void RebuildFontTexture() = 0;
            
        protected:
            static IMGUIRenderer* (*CreateFunc)(uint32_t, uint32_t, bool);
        };
    }
}
