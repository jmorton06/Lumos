#include "Precompiled.h"
#include "GLRenderPass.h"
#include "Graphics/RHI/Renderer.h"
#include "GLFramebuffer.h"
#include "GLDebug.h"

namespace Lumos
{
    namespace Graphics
    {
        GLRenderPass::GLRenderPass(const RenderPassDesc& renderPassDesc)
        {
            Init(renderPassDesc);
        }

        GLRenderPass::~GLRenderPass()
        {
        }

        bool GLRenderPass::Init(const RenderPassDesc& renderPassDesc)
        {
            m_Clear = renderPassDesc.clear;
            m_ClearCount = renderPassDesc.attachmentCount;
            return false;
        }

        void GLRenderPass::BeginRenderpass(CommandBuffer* commandBuffer, const glm::vec4& clearColour,
            Framebuffer* frame, SubPassContents contents, uint32_t width, uint32_t height) const
        {
            if(frame != nullptr)
            {
                frame->Bind(width, height);
                // frame->SetClearColour(clearColour);
                GLCall(glClearColor(clearColour.x, clearColour.y, clearColour.z, clearColour.w));
            }
            else
            {
                GLCall(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
                GLCall(glClearColor(clearColour.x, clearColour.y, clearColour.z, clearColour.w));
                GLCall(glViewport(0, 0, width, height));
            }

            if(m_Clear)
                GLRenderer::ClearInternal(RENDERER_BUFFER_COLOUR | RENDERER_BUFFER_DEPTH | RENDERER_BUFFER_STENCIL);
        }

        void GLRenderPass::EndRenderpass(CommandBuffer* commandBuffer)
        {
            GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        }

        void GLRenderPass::MakeDefault()
        {
            CreateFunc = CreateFuncGL;
        }

        RenderPass* GLRenderPass::CreateFuncGL(const RenderPassDesc& renderPassDesc)
        {
            return new GLRenderPass(renderPassDesc);
        }
    }
}
