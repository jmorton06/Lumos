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
            int mipIndex = -1;
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

            virtual void Bind(uint32_t width, uint32_t height) const = 0;
            virtual void Bind() const = 0;
            virtual void UnBind() const = 0;
            virtual void Clear() = 0;
            virtual void Validate() {};
            virtual void AddTextureAttachment(RHIFormat format, Texture* texture) = 0;
            virtual void AddCubeTextureAttachment(RHIFormat format, CubeFace face, TextureCube* texture) = 0;
            virtual void AddShadowAttachment(Texture* texture) = 0;
            virtual void AddTextureLayer(int index, Texture* texture) = 0;
            virtual void GenerateFramebuffer() = 0;

            virtual uint32_t GetWidth() const = 0;
            virtual uint32_t GetHeight() const = 0;
            virtual void SetClearColour(const glm::vec4& colour) = 0;

        protected:
            static Framebuffer* (*CreateFunc)(const FramebufferDesc&);
        };
    }
}
