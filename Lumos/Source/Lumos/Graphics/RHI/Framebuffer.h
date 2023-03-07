#pragma once
#include <glm/vec4.hpp>
#include "Definitions.h"

namespace Lumos
{
    namespace Graphics
    {
        enum class CubeFace
        {
            PositiveX,
            NegativeX,
            PositiveY,
            NegativeY,
            PositiveZ,
            NegativeZ
        };

        struct FramebufferDesc
        {
            uint32_t width;
            uint32_t height;
            uint32_t layer = 0;
            uint32_t attachmentCount;
            uint32_t msaaLevel;
            int mipIndex   = 0;
            bool screenFBO = false;
            Texture** attachments;
            TextureType* attachmentTypes;
            Graphics::RenderPass* renderPass;
        };

        class LUMOS_EXPORT Framebuffer
        {
        public:
            static SharedPtr<Framebuffer> Get(const FramebufferDesc& framebufferDesc);
            static Framebuffer* Create(const FramebufferDesc& framebufferDesc);
            static void ClearCache();
            static void DeleteUnusedCache();

            virtual ~Framebuffer();

            virtual void Validate() {};

            virtual uint32_t GetWidth() const                    = 0;
            virtual uint32_t GetHeight() const                   = 0;
            virtual void SetClearColour(const glm::vec4& colour) = 0;

        protected:
            static Framebuffer* (*CreateFunc)(const FramebufferDesc&);
        };
    }
}
