#pragma once
#include <glm/ext/vector_float4.hpp>
#include "Definitions.h"

namespace Lumos
{
    namespace Graphics
    {
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
