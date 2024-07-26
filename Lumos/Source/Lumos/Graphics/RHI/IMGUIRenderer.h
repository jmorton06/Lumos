#pragma once
#include "RHIDefinitions.h"

namespace Lumos
{
    namespace Graphics
    {
        class CommandBuffer;

        struct ImGuiTextureID
        {
            Texture* texture;
            TextureType type;
            uint32_t level;
            uint32_t mip;
        };

        class LUMOS_EXPORT IMGUIRenderer
        {
        public:
            static IMGUIRenderer* Create(uint32_t width, uint32_t height, bool clearScreen);

            virtual ~IMGUIRenderer()                               = default;
            virtual void Init()                                    = 0;
            virtual void NewFrame()                                = 0;
            virtual void Render(CommandBuffer* commandBuffer)      = 0;
            virtual void OnResize(uint32_t width, uint32_t height) = 0;
            virtual void Clear() { }
            virtual bool Implemented() const  = 0;
            virtual void RebuildFontTexture() = 0;
            virtual ImGuiTextureID* AddTexture(Texture* texture, TextureType type = TextureType::COLOUR, uint32_t level = 0, uint32_t mip = 0);

        protected:
            static IMGUIRenderer* (*CreateFunc)(uint32_t, uint32_t, bool);

#define MAX_IMGUI_TEXTURES 1024
            ImGuiTextureID m_TextureIDs[MAX_IMGUI_TEXTURES];
            uint32_t m_CurrentTextureIDIndex = 0;
        };
    }
}
