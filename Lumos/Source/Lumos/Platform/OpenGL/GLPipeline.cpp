#include "Precompiled.h"
#include "GLPipeline.h"
#include "GLDescriptorSet.h"
#include "GLShader.h"
#include "GLFramebuffer.h"
#include "GLRenderPass.h"
#include "GLSwapChain.h"

#include <glad/glad.h>

namespace Lumos
{
    namespace Graphics
    {
        GLPipeline::GLPipeline(const PipelineDesc& pipelineDesc)
            : m_RenderPass(nullptr)
        {
            Init(pipelineDesc);
        }

        GLPipeline::~GLPipeline()
        {
            glDeleteVertexArrays(1, &m_VertexArray);
        }

        void VertexAtrribPointer(Format format, uint32_t index, size_t offset, uint32_t stride)
        {
            switch(format)
            {
            case Format::R32_FLOAT:
                GLCall(glVertexAttribPointer(index, 1, GL_FLOAT, false, stride, (const void*)(intptr_t)(offset)));
                break;
            case Format::R32G32_FLOAT:
                GLCall(glVertexAttribPointer(index, 2, GL_FLOAT, false, stride, (const void*)(intptr_t)(offset)));
                break;
            case Format::R32G32B32_FLOAT:
                GLCall(glVertexAttribPointer(index, 3, GL_FLOAT, false, stride, (const void*)(intptr_t)(offset)));
                break;
            case Format::R32G32B32A32_FLOAT:
                GLCall(glVertexAttribPointer(index, 4, GL_FLOAT, false, stride, (const void*)(intptr_t)(offset)));
                break;
            case Format::R8_UINT:
                GLCall(glVertexAttribPointer(index, 1, GL_UNSIGNED_BYTE, false, stride, (const void*)(intptr_t)(offset)));
                break;
            case Format::R32_UINT:
                GLCall(glVertexAttribPointer(index, 1, GL_UNSIGNED_INT, false, stride, (const void*)(intptr_t)(offset)));
                break;
            case Format::R32G32_UINT:
                GLCall(glVertexAttribPointer(index, 2, GL_UNSIGNED_INT, false, stride, (const void*)(intptr_t)(offset)));
                break;
            case Format::R32G32B32_UINT:
                GLCall(glVertexAttribPointer(index, 3, GL_UNSIGNED_INT, false, stride, (const void*)(intptr_t)(offset)));
                break;
            case Format::R32G32B32A32_UINT:
                GLCall(glVertexAttribPointer(index, 4, GL_UNSIGNED_INT, false, stride, (const void*)(intptr_t)(offset)));
                break;
            case Format::R32G32_INT:
                GLCall(glVertexAttribPointer(index, 2, GL_INT, false, stride, (const void*)(intptr_t)(offset)));
                break;
            case Format::R32G32B32_INT:
                GLCall(glVertexAttribPointer(index, 3, GL_INT, false, stride, (const void*)(intptr_t)(offset)));
                break;
            case Format::R32G32B32A32_INT:
                GLCall(glVertexAttribPointer(index, 4, GL_INT, false, stride, (const void*)(intptr_t)(offset)));
                break;
            }
        }

        bool GLPipeline::Init(const PipelineDesc& pipelineDesc)
        {
            m_TransparencyEnabled = pipelineDesc.transparencyEnabled;
            m_CullMode = pipelineDesc.cullMode;
            m_Description = pipelineDesc;

            GLCall(glGenVertexArrays(1, &m_VertexArray));

            m_Shader = pipelineDesc.shader.get();
            m_BlendMode = pipelineDesc.blendMode;

            CreateFramebuffers();
            return true;
        }

        void GLPipeline::BindVertexArray()
        {
            GLCall(glBindVertexArray(m_VertexArray));

            auto& vertexLayout = ((GLShader*)m_Shader)->GetBufferLayout().GetLayout();
            uint32_t count = 0;

            for(auto& layout : vertexLayout)
            {
                GLCall(glEnableVertexAttribArray(count));
                size_t offset = static_cast<size_t>(layout.offset);
                VertexAtrribPointer(layout.format, count, offset, ((GLShader*)m_Shader)->GetBufferLayout().GetStride());
                count++;
            }
        }

        void GLPipeline::CreateFramebuffers()
        {
            std::vector<TextureType> attachmentTypes;
            std::vector<AttachmentInfo> textureTypes;
            std::vector<Texture*> attachments;

            if(m_Description.swapchainTarget)
            {
                textureTypes.push_back({ TextureType::COLOUR, TextureFormat::RGBA8 });
                attachmentTypes.push_back(TextureType::COLOUR);
                attachments.push_back(nullptr);
            }
            else
            {
                for(auto texture : m_Description.colourTargets)
                {
                    if(texture)
                    {
                        textureTypes.push_back({ TextureType::COLOUR, texture->GetFormat() });
                        attachmentTypes.push_back(texture->GetType());
                        attachments.push_back(texture);
                    }
                }
            }

            if(m_Description.depthTarget)
            {
                textureTypes.push_back({ TextureType::DEPTH, TextureFormat::DEPTH }); //TODO: Custom depth format
                attachmentTypes.push_back(m_Description.depthTarget->GetType());
                attachments.push_back(m_Description.depthTarget);
            }

            if(m_Description.depthArrayTarget)
            {
                textureTypes.push_back({ TextureType::DEPTHARRAY, TextureFormat::DEPTH }); //TODO: Custom depth format
                attachmentTypes.push_back(m_Description.depthArrayTarget->GetType());
                attachments.push_back(m_Description.depthArrayTarget);
            }

            Graphics::RenderPassDesc renderPassDesc;
            renderPassDesc.attachmentCount = uint32_t(textureTypes.size());
            renderPassDesc.textureType = textureTypes.data();
            renderPassDesc.clear = m_Description.clearTargets;

            m_RenderPass = Graphics::RenderPass::Get(renderPassDesc);

            FramebufferDesc frameBufferDesc {};
            frameBufferDesc.width = GetWidth();
            frameBufferDesc.height = GetHeight();
            frameBufferDesc.attachmentCount = uint32_t(attachments.size());
            frameBufferDesc.renderPass = m_RenderPass.get();
            frameBufferDesc.attachmentTypes = attachmentTypes.data();

            if(m_Description.swapchainTarget)
            {
                for(uint32_t i = 0; i < Renderer::GetSwapChain()->GetSwapChainBufferCount(); i++)
                {
                    frameBufferDesc.screenFBO = true;
                    attachments[0] = Renderer::GetSwapChain()->GetImage(i);
                    frameBufferDesc.attachments = attachments.data();

                    m_Framebuffers.emplace_back(Framebuffer::Get(frameBufferDesc));
                }
            }
            else if(m_Description.depthArrayTarget)
            {
                for(uint32_t i = 0; i < ((GLTextureDepthArray*)m_Description.depthArrayTarget)->GetCount(); ++i)
                {
                    attachments[0] = Renderer::GetSwapChain()->GetImage(i);
                    frameBufferDesc.layer = i;
                    frameBufferDesc.screenFBO = false;

                    attachments[0] = m_Description.depthArrayTarget;
                    frameBufferDesc.attachments = attachments.data();

                    m_Framebuffers.emplace_back(Framebuffer::Get(frameBufferDesc));
                }
            }
            else
            {
                frameBufferDesc.attachments = attachments.data();
                frameBufferDesc.screenFBO = false;
                m_Framebuffers.emplace_back(Framebuffer::Get(frameBufferDesc));
            }
        }

        void GLPipeline::Bind(Graphics::CommandBuffer* commandBuffer, uint32_t layer)
        {
            Framebuffer* framebuffer;
            uint32_t width, height;

            if(m_Description.swapchainTarget)
            {
                framebuffer = m_Framebuffers[Renderer::GetSwapChain()->GetCurrentBufferIndex()];
            }
            else if(m_Description.depthArrayTarget)
            {
                framebuffer = m_Framebuffers[layer];
            }
            else
            {
                framebuffer = m_Framebuffers[0];
            }

            m_RenderPass->BeginRenderpass(commandBuffer, m_Description.clearColour, framebuffer, Graphics::INLINE, GetWidth(), GetHeight());

            m_Shader->Bind();

            if(m_TransparencyEnabled)
            {
                glEnable(GL_BLEND);

                GLCall(glBlendEquation(GL_FUNC_ADD));

                if(m_BlendMode == BlendMode::SrcAlphaOneMinusSrcAlpha)
                {
                    GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
                }
                else if(m_BlendMode == BlendMode::ZeroSrcColor)
                {
                    GLCall(glBlendFunc(GL_ZERO, GL_SRC_COLOR));
                }
                else if(m_BlendMode == BlendMode::OneZero)
                {
                    GLCall(glBlendFunc(GL_ONE, GL_ZERO));
                }
                else
                {
                    GLCall(glBlendFunc(GL_NONE, GL_NONE));
                }
            }
            else
                glDisable(GL_BLEND);

            glEnable(GL_CULL_FACE);

            switch(m_CullMode)
            {
            case CullMode::BACK:
                glCullFace(GL_BACK);
                break;
            case CullMode::FRONT:
                glCullFace(GL_FRONT);
                break;
            case CullMode::FRONTANDBACK:
                glCullFace(GL_FRONT_AND_BACK);
                break;
            case CullMode::NONE:
                glDisable(GL_CULL_FACE);
                break;
            }

            GLCall(glFrontFace(GL_CCW));
        }

        void GLPipeline::End(Graphics::CommandBuffer* commandBuffer)
        {
            m_RenderPass->EndRenderpass(commandBuffer);
        }

        void GLPipeline::ClearRenderTargets(CommandBuffer* commandBuffer)
        {
            for(auto framebuffer : m_Framebuffers)
            {
                framebuffer->Bind();
                GLRenderer::ClearInternal(RENDERER_BUFFER_COLOUR | RENDERER_BUFFER_DEPTH | RENDERER_BUFFER_STENCIL);
            }
        }

        void GLPipeline::MakeDefault()
        {
            CreateFunc = CreateFuncGL;
        }

        Pipeline* GLPipeline::CreateFuncGL(const PipelineDesc& pipelineDesc)
        {
            return new GLPipeline(pipelineDesc);
        }
    }
}
