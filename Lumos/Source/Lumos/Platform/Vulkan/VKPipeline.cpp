#include "Precompiled.h"
#include "VKPipeline.h"
#include "VKDevice.h"
#include "VKCommandBuffer.h"
#include "VKRenderPass.h"
#include "VKRenderer.h"
#include "VKSwapChain.h"
#include "VKShader.h"
#include "VKUtilities.h"
#include "Graphics/RHI/DescriptorSet.h"
#include "VKInitialisers.h"

namespace Lumos
{

    namespace Graphics
    {
        VKPipeline::VKPipeline(const PipelineDesc& pipelineDesc)
        {
            Init(pipelineDesc);
        }

        VKPipeline::~VKPipeline()
        {
            LUMOS_PROFILE_FUNCTION();
            VKContext::DeletionQueue& deletionQueue = VKRenderer::GetCurrentDeletionQueue();

            auto pipeline = m_Pipeline;

            deletionQueue.PushFunction([pipeline]
                { vkDestroyPipeline(VKDevice::Get().GetDevice(), pipeline, VK_NULL_HANDLE); });
        }

        bool VKPipeline::Init(const PipelineDesc& pipelineDesc)
        {
            LUMOS_PROFILE_FUNCTION();
            m_Description = pipelineDesc;
            m_Shader = m_Description.shader;
            m_PipelineLayout = m_Shader.As<VKShader>()->GetPipelineLayout();
            
            TransitionAttachments();
            CreateFramebuffers();

            // Pipeline
            std::vector<VkDynamicState> dynamicStateDescriptors;
            VkPipelineDynamicStateCreateInfo dynamicStateCI {};
            dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicStateCI.pNext = NULL;
            dynamicStateCI.pDynamicStates = dynamicStateDescriptors.data();

            std::vector<VkVertexInputAttributeDescription> vertexInputDescription;

            // Vertex layout
            VkVertexInputBindingDescription vertexBindingDescription;
            vertexBindingDescription.binding = 0;
            vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            vertexBindingDescription.stride = m_Shader.As<VKShader>()->GetVertexInputStride();

            const std::vector<VkVertexInputAttributeDescription>& vertexInputAttributeDescription = m_Shader.As<VKShader>()->GetVertexInputAttributeDescription();

            VkPipelineVertexInputStateCreateInfo vi {};
            vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vi.pNext = NULL;
            vi.vertexBindingDescriptionCount = 1;
            vi.pVertexBindingDescriptions = &vertexBindingDescription;
            vi.vertexAttributeDescriptionCount = uint32_t(vertexInputAttributeDescription.size());
            vi.pVertexAttributeDescriptions = vertexInputAttributeDescription.data();

            VkPipelineInputAssemblyStateCreateInfo inputAssemblyCI {};
            inputAssemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssemblyCI.pNext = NULL;
            inputAssemblyCI.primitiveRestartEnable = VK_FALSE;
            inputAssemblyCI.topology = VKUtilities::DrawTypeToVk(pipelineDesc.drawType);

            VkPipelineRasterizationStateCreateInfo rs {};
            rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rs.polygonMode = VKUtilities::PolygonModeToVk(pipelineDesc.polygonMode);
            rs.cullMode = VKUtilities::CullModeToVK(pipelineDesc.cullMode);
            rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            rs.depthClampEnable = VK_FALSE;
            rs.rasterizerDiscardEnable = VK_FALSE;
            rs.depthBiasEnable = (pipelineDesc.depthBiasEnabled ? VK_TRUE : VK_FALSE);
            rs.depthBiasConstantFactor = 0;
            rs.depthBiasClamp = 0;
            rs.depthBiasSlopeFactor = 0;
            rs.lineWidth = 1.0f;
            rs.pNext = NULL;

            VkPipelineColorBlendStateCreateInfo cb {};
            cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            cb.pNext = NULL;
            cb.flags = 0;

            std::vector<VkPipelineColorBlendAttachmentState> blendAttachState;
            blendAttachState.resize(m_RenderPass.As<VKRenderPass>()->GetColourAttachmentCount());

            for(unsigned int i = 0; i < blendAttachState.size(); i++)
            {
                blendAttachState[i] = VkPipelineColorBlendAttachmentState();
                blendAttachState[i].colorWriteMask = 0x0f;
                blendAttachState[i].alphaBlendOp = VK_BLEND_OP_ADD;
                blendAttachState[i].colorBlendOp = VK_BLEND_OP_ADD;

                if(pipelineDesc.transparencyEnabled)
                {
                    blendAttachState[i].blendEnable = VK_TRUE;
                    blendAttachState[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                    blendAttachState[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;

                    if(pipelineDesc.blendMode == BlendMode::SrcAlphaOneMinusSrcAlpha)
                    {
                        blendAttachState[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                        blendAttachState[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                    }
                    else if(pipelineDesc.blendMode == BlendMode::ZeroSrcColor)
                    {
                        blendAttachState[i].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                        blendAttachState[i].dstColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
                    }
                    else if(pipelineDesc.blendMode == BlendMode::OneZero)
                    {
                        blendAttachState[i].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                        blendAttachState[i].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                    }
                    else
                    {
                        blendAttachState[i].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                        blendAttachState[i].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                    }
                }
                else
                {
                    blendAttachState[i].blendEnable = VK_FALSE;
                    blendAttachState[i].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                    blendAttachState[i].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                    blendAttachState[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                    blendAttachState[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                }
            }

            cb.attachmentCount = static_cast<uint32_t>(blendAttachState.size());
            cb.pAttachments = blendAttachState.data();
            cb.logicOpEnable = VK_FALSE;
            cb.logicOp = VK_LOGIC_OP_NO_OP;
            cb.blendConstants[0] = 1.0f;
            cb.blendConstants[1] = 1.0f;
            cb.blendConstants[2] = 1.0f;
            cb.blendConstants[3] = 1.0f;

            VkPipelineViewportStateCreateInfo vp {};
            vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            vp.pNext = NULL;
            vp.viewportCount = 1;
            vp.scissorCount = 1;
            vp.pScissors = NULL;
            vp.pViewports = NULL;
            dynamicStateDescriptors.push_back(VK_DYNAMIC_STATE_VIEWPORT);
            dynamicStateDescriptors.push_back(VK_DYNAMIC_STATE_SCISSOR);

            if(pipelineDesc.depthBiasEnabled)
            {
                dynamicStateDescriptors.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
                m_DepthBiasConstant = 1.25f;
                m_DepthBiasSlope = 1.75f;
                m_DepthBiasEnabled = true;
            }
            else
            {
                m_DepthBiasEnabled = false;
            }

            VkPipelineDepthStencilStateCreateInfo ds {};
            ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            ds.pNext = NULL;
            ds.depthTestEnable = VK_TRUE;
            ds.depthWriteEnable = VK_TRUE;
            ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
            ds.depthBoundsTestEnable = VK_FALSE;
            ds.stencilTestEnable = VK_FALSE;
            ds.back.failOp = VK_STENCIL_OP_KEEP;
            ds.back.passOp = VK_STENCIL_OP_KEEP;
            ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
            ds.back.compareMask = 0;
            ds.back.reference = 0;
            ds.back.depthFailOp = VK_STENCIL_OP_KEEP;
            ds.back.writeMask = 0;
            ds.minDepthBounds = 0;
            ds.maxDepthBounds = 0;
            ds.front = ds.back;

            VkPipelineMultisampleStateCreateInfo ms {};
            ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            ms.pNext = NULL;
            ms.pSampleMask = NULL;
            ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            ms.sampleShadingEnable = VK_FALSE;
            ms.alphaToCoverageEnable = VK_FALSE;
            ms.alphaToOneEnable = VK_FALSE;
            ms.minSampleShading = 0.0;

            dynamicStateCI.dynamicStateCount = uint32_t(dynamicStateDescriptors.size());
            dynamicStateCI.pDynamicStates = dynamicStateDescriptors.data();

            auto vkshader = pipelineDesc.shader.As<VKShader>();
            VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo {};
            graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            graphicsPipelineCreateInfo.pNext = NULL;
            graphicsPipelineCreateInfo.layout = m_PipelineLayout;
            graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
            graphicsPipelineCreateInfo.basePipelineIndex = -1;
            graphicsPipelineCreateInfo.pVertexInputState = &vi;
            graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCI;
            graphicsPipelineCreateInfo.pRasterizationState = &rs;
            graphicsPipelineCreateInfo.pColorBlendState = &cb;
            graphicsPipelineCreateInfo.pTessellationState = VK_NULL_HANDLE;
            graphicsPipelineCreateInfo.pMultisampleState = &ms;
            graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCI;
            graphicsPipelineCreateInfo.pViewportState = &vp;
            graphicsPipelineCreateInfo.pDepthStencilState = &ds;
            graphicsPipelineCreateInfo.pStages = vkshader->GetShaderStages();
            graphicsPipelineCreateInfo.stageCount = vkshader->GetStageCount();
            graphicsPipelineCreateInfo.renderPass = m_RenderPass.As<VKRenderPass>()->GetHandle();
            graphicsPipelineCreateInfo.subpass = 0;

            VK_CHECK_RESULT(vkCreateGraphicsPipelines(VKDevice::Get().GetDevice(), VKDevice::Get().GetPipelineCache(), 1, &graphicsPipelineCreateInfo, VK_NULL_HANDLE, &m_Pipeline));

            return true;
        }

        void VKPipeline::Bind(CommandBuffer* commandBuffer, uint32_t layer)
        {
            LUMOS_PROFILE_FUNCTION();
            if(m_DepthBiasEnabled)
                vkCmdSetDepthBias(static_cast<VKCommandBuffer*>(commandBuffer)->GetHandle(),
                    m_DepthBiasConstant,
                    0.0f,
                    m_DepthBiasSlope);

            VKFramebuffer* framebuffer;

            if(m_Description.swapchainTarget)
            {
                framebuffer = m_Framebuffers[Renderer::GetMainSwapChain()->GetCurrentImageIndex()];
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
            
            vkCmdBindPipeline(static_cast<VKCommandBuffer*>(commandBuffer)->GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
        }

        void VKPipeline::CreateFramebuffers()
        {
            LUMOS_PROFILE_FUNCTION();
            std::vector<TextureType> attachmentTypes;
            std::vector<Texture*> attachments;

            if(m_Description.swapchainTarget)
            {
                attachmentTypes.push_back(TextureType::COLOUR);
                attachments.push_back(Renderer::GetMainSwapChain()->GetImage(0));
            }
            else
            {
                for(auto texture : m_Description.colourTargets)
                {
                    if(texture)
                    {
                        attachmentTypes.push_back(texture->GetType());
                        attachments.push_back(texture);
                    }
                }
            }

            if(m_Description.depthTarget)
            {
                attachmentTypes.push_back(m_Description.depthTarget->GetType());
                attachments.push_back(m_Description.depthTarget);
            }

            if(m_Description.depthArrayTarget)
            {
                attachmentTypes.push_back(m_Description.depthArrayTarget->GetType());
                attachments.push_back(m_Description.depthArrayTarget);
            }

            Graphics::RenderPassDesc renderPassDesc;
            renderPassDesc.attachmentCount = uint32_t(attachmentTypes.size());
            renderPassDesc.attachmentTypes = attachmentTypes.data();
            renderPassDesc.attachments = attachments.data();
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
                for(uint32_t i = 0; i < Renderer::GetMainSwapChain()->GetSwapChainBufferCount(); i++)
                {
                    frameBufferDesc.screenFBO = true;
                    attachments[0] = Renderer::GetMainSwapChain()->GetImage(i);
                    frameBufferDesc.attachments = attachments.data();

                    m_Framebuffers.emplace_back(Framebuffer::Get(frameBufferDesc));
                }
            }
            else if(m_Description.depthArrayTarget)
            {
                for(uint32_t i = 0; i < ((VKTextureDepthArray*)m_Description.depthArrayTarget)->GetCount(); ++i)
                {
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

        void VKPipeline::End(CommandBuffer* commandBuffer)
        {
            LUMOS_PROFILE_FUNCTION();
            m_RenderPass->EndRenderpass(commandBuffer);
        }

        void VKPipeline::ClearRenderTargets(CommandBuffer* commandBuffer)
        {
            LUMOS_PROFILE_FUNCTION();
            if(m_Description.swapchainTarget)
            {
                for(uint32_t i = 0; i < Renderer::GetMainSwapChain()->GetSwapChainBufferCount(); i++)
                {
                    Renderer::GetRenderer()->ClearRenderTarget(Renderer::GetMainSwapChain()->GetImage(i), commandBuffer);
                }
            }
            else if(m_Description.depthArrayTarget)
            {
                Renderer::GetRenderer()->ClearRenderTarget(m_Description.depthArrayTarget, commandBuffer);
            }
            else if(m_Description.depthTarget)
            {
                Renderer::GetRenderer()->ClearRenderTarget(m_Description.depthTarget, commandBuffer);
            }
            else
            {
                for(auto texture : m_Description.colourTargets)
                {
                    if(texture != nullptr)
                    {
                        Renderer::GetRenderer()->ClearRenderTarget(texture, commandBuffer);
                    }
                }
            }
        }
    
        void VKPipeline::TransitionAttachments()
        {
            LUMOS_PROFILE_FUNCTION();
            if(m_Description.swapchainTarget)
            {
                for(uint32_t i = 0; i < Renderer::GetMainSwapChain()->GetSwapChainBufferCount(); i++)
                {
                    ((VKTexture2D*)Renderer::GetMainSwapChain()->GetImage(i))->TransitionImage(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
                }
            }
            else if(m_Description.depthArrayTarget)
            {
                ((VKTextureDepthArray*)m_Description.depthArrayTarget)->TransitionImage(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            }
            else if(m_Description.depthTarget)
            {
                ((VKTextureDepth*)m_Description.depthTarget)->TransitionImage(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            }
            else
            {
                for(auto texture : m_Description.colourTargets)
                {
                    if(texture != nullptr)
                    {
                        ((VKTexture2D*)texture)->TransitionImage(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
                    }
                }
            }
        }

        void VKPipeline::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
        }

        Pipeline* VKPipeline::CreateFuncVulkan(const PipelineDesc& pipelineDesc)
        {
            return new VKPipeline(pipelineDesc);
        }
    }
}
