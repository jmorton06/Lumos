#include "Precompiled.h"
#include "VKPipeline.h"
#include "VKDevice.h"
#include "VKCommandBuffer.h"
#include "VKRenderpass.h"
#include "VKShader.h"
#include "VKTools.h"
#include "Graphics/RHI/DescriptorSet.h"
#include "VKInitialisers.h"

namespace Lumos
{

    namespace Graphics
    {
        VKPipeline::VKPipeline(const PipelineDesc& pipelineCreateInfo)
        {
            Init(pipelineCreateInfo);
        }

        VKPipeline::~VKPipeline()
        {
            vkDestroyPipeline(VKDevice::Get().GetDevice(), m_Pipeline, VK_NULL_HANDLE);
        }

        bool VKPipeline::Init(const PipelineDesc& pipelineCreateInfo)
        {
            m_Shader = pipelineCreateInfo.shader;
            m_PipelineLayout = m_Shader.As<VKShader>()->GetPipelineLayout();

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
            inputAssemblyCI.topology = VKTools::DrawTypeToVk(pipelineCreateInfo.drawType);

            VkPipelineRasterizationStateCreateInfo rs {};
            rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rs.polygonMode = VKTools::PolygonModeToVk(pipelineCreateInfo.polygonMode);
            rs.cullMode = VKTools::CullModeToVK(pipelineCreateInfo.cullMode);
            rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            rs.depthClampEnable = VK_FALSE;
            rs.rasterizerDiscardEnable = VK_FALSE;
            rs.depthBiasEnable = (pipelineCreateInfo.depthBiasEnabled ? VK_TRUE : VK_FALSE);
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
            blendAttachState.resize(pipelineCreateInfo.renderpass.As<VKRenderpass>()->GetColourAttachmentCount());

            for(unsigned int i = 0; i < blendAttachState.size(); i++)
            {
                blendAttachState[i] = VkPipelineColorBlendAttachmentState();
                blendAttachState[i].colorWriteMask = 0x0f;
                blendAttachState[i].alphaBlendOp = VK_BLEND_OP_ADD;
                blendAttachState[i].colorBlendOp = VK_BLEND_OP_ADD;

                if(pipelineCreateInfo.transparencyEnabled)
                {
                    blendAttachState[i].blendEnable = VK_TRUE;
                    blendAttachState[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                    blendAttachState[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                    blendAttachState[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                    blendAttachState[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
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

            if(pipelineCreateInfo.depthBiasEnabled)
                dynamicStateDescriptors.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);

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

            auto vkshader = pipelineCreateInfo.shader.As<VKShader>();
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
            graphicsPipelineCreateInfo.renderPass = pipelineCreateInfo.renderpass.As<VKRenderpass>()->GetHandle();
            graphicsPipelineCreateInfo.subpass = 0;

            VK_CHECK_RESULT(vkCreateGraphicsPipelines(VKDevice::Get().GetDevice(), VKDevice::Get().GetPipelineCache(), 1, &graphicsPipelineCreateInfo, VK_NULL_HANDLE, &m_Pipeline));

            return true;
        }

        void VKPipeline::Bind(CommandBuffer* cmdBuffer)
        {
            vkCmdBindPipeline(static_cast<VKCommandBuffer*>(cmdBuffer)->GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
        }

        void VKPipeline::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
        }

        Pipeline* VKPipeline::CreateFuncVulkan(const PipelineDesc& pipelineCreateInfo)
        {
            return new VKPipeline(pipelineCreateInfo);
        }
    }
}
