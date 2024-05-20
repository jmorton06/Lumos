#pragma once
#include "Definitions.h"
#include <glm/ext/vector_float4.hpp>

namespace Lumos
{
    namespace Graphics
    {
        class LUMOS_EXPORT RenderPass
        {
        public:
            virtual ~RenderPass();
            static RenderPass* Create(const RenderPassDesc& renderPassDesc);
            static SharedPtr<RenderPass> Get(const RenderPassDesc& renderPassDesc);
            static void ClearCache();
            static void DeleteUnusedCache();

            virtual void BeginRenderPass(CommandBuffer* commandBuffer, float* clearColour, Framebuffer* frame, SubPassContents contents, uint32_t width, uint32_t height) const = 0;
            virtual void EndRenderPass(CommandBuffer* commandBuffer)                                                                                                            = 0;
            virtual int GetAttachmentCount() const                                                                                                                              = 0;

        protected:
            static RenderPass* (*CreateFunc)(const RenderPassDesc&);
        };
    }
}
