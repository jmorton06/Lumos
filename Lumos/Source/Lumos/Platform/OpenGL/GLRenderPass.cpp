#include "Precompiled.h"
#include "GLRenderPass.h"
#include "Graphics/API/Renderer.h"
#include "GLFramebuffer.h"
#include "GLDebug.h"

namespace Lumos
{
    namespace Graphics
    {
        GLRenderPass::GLRenderPass(const RenderPassInfo& renderpassCI)
        {
            Init(renderpassCI);
        }

        GLRenderPass::~GLRenderPass()
        {
        }

        bool GLRenderPass::Init(const RenderPassInfo& renderpassCI)
        {
            m_Clear = renderpassCI.clear;
            m_ClearCount = renderpassCI.attachmentCount;
            return false;
        }

        void GLRenderPass::BeginRenderpass(CommandBuffer* commandBuffer, const Maths::Vector4& clearColour,
            Framebuffer* frame, SubPassContents contents, uint32_t width, uint32_t height) const
        {
            if(frame != nullptr)
            {
                frame->Bind(width, height);
                //frame->SetClearColour(clearColour);
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

        RenderPass* GLRenderPass::CreateFuncGL(const RenderPassInfo& renderpassCI)
        {
            return new GLRenderPass(renderpassCI);
        }
    }
}
