#include "lmpch.h"
#include "VKPipeline.h"
#include "VKDevice.h"
#include "VKCommandBuffer.h"
#include "VKRenderpass.h"
#include "VKShader.h"
#include "VKTools.h"
#include "Graphics/API/DescriptorSet.h"


namespace Lumos
{
	namespace Graphics
	{
		VKPipeline::VKPipeline(const PipelineInfo& pipelineCI)
		{
			Init(pipelineCI);
		}

		VKPipeline::~VKPipeline()
		{
			Unload();

            delete m_DescriptorSet;
		}

		bool VKPipeline::Init(const PipelineInfo& pipelineCI)
		{
			m_PipelineName = pipelineCI.pipelineName;
			m_Shader	   = pipelineCI.shader;

			// Vertex layout
			m_VertexBindingDescription.binding = 0;
			m_VertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			m_VertexBindingDescription.stride = static_cast<uint32_t>(pipelineCI.strideSize);

			for (auto& descriptorLayout : pipelineCI.descriptorLayouts)
			{
				std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;

				setLayoutBindings.reserve(descriptorLayout.count);

				for (u32 i = 0; i < descriptorLayout.count; i++)
				{
					auto info = descriptorLayout.layoutInfo[i];

					VkDescriptorSetLayoutBinding setLayoutBinding{};
					setLayoutBinding.descriptorType = VKTools::DescriptorTypeToVK(info.type);
					setLayoutBinding.stageFlags = VKTools::ShaderTypeToVK(info.stage);
					setLayoutBinding.binding = info.size;
					setLayoutBinding.descriptorCount = info.count;

					setLayoutBindings.push_back(setLayoutBinding);
				}

				// Pipeline layout
				VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
				descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				descriptorLayoutCI.bindingCount = static_cast<u32>(setLayoutBindings.size());
				descriptorLayoutCI.pBindings = setLayoutBindings.data();

				VkDescriptorSetLayout layout;
				vkCreateDescriptorSetLayout(VKDevice::Instance()->GetDevice(), &descriptorLayoutCI, VK_NULL_HANDLE, &layout);

				m_DescriptorLayouts.push_back(layout);
			}

			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(m_DescriptorLayouts.size());
			pipelineLayoutCreateInfo.pSetLayouts = m_DescriptorLayouts.data();

			auto result = vkCreatePipelineLayout(VKDevice::Instance()->GetDevice(), &pipelineLayoutCreateInfo, VK_NULL_HANDLE, &m_PipelineLayout);
			if (result != VK_SUCCESS)
				return false;

			std::vector<VkDescriptorPoolSize> poolSizes;
			poolSizes.reserve(pipelineCI.numLayoutBindings);

			for (u32 i = 0; i < pipelineCI.numLayoutBindings; i++)
			{
				auto info = pipelineCI.typeCounts[i];
				VkDescriptorPoolSize descriptorPoolSize{};
				descriptorPoolSize.type = VKTools::DescriptorTypeToVK(info.type);
				descriptorPoolSize.descriptorCount = info.size;
				poolSizes.push_back(descriptorPoolSize);
			}

			// Descriptor pool
			VkDescriptorPoolCreateInfo descriptorPoolInfo{};
			descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
			descriptorPoolInfo.pPoolSizes = poolSizes.data();
			descriptorPoolInfo.maxSets = pipelineCI.maxObjects;

			VK_CHECK_RESULT(vkCreateDescriptorPool(VKDevice::Instance()->GetDevice(), &descriptorPoolInfo, nullptr, &m_DescriptorPool));

			DescriptorInfo info;
			info.pipeline = this;
			info.layoutIndex = 0;
            info.shader = pipelineCI.shader;

			m_DescriptorSet = lmnew VKDescriptorSet(info);

			// Pipeline
			VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];
			VkPipelineDynamicStateCreateInfo dynamicStateCI{};
			dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			memset(dynamicStateEnables, 0, sizeof(dynamicStateEnables));
			dynamicStateCI.pNext = NULL;
			dynamicStateCI.pDynamicStates = dynamicStateEnables;
			dynamicStateCI.dynamicStateCount = 0;

			std::vector<VkVertexInputAttributeDescription> vertexInputDescription;

			vertexInputDescription.reserve(pipelineCI.numVertexLayout);
			for(u32 i = 0; i < pipelineCI.numVertexLayout; i++)
			{
				vertexInputDescription.push_back(VKTools::VertexInputDescriptionToVK(pipelineCI.vertexLayout[i]));
			}

			VkPipelineVertexInputStateCreateInfo vi{};
			memset(&vi, 0, sizeof(vi));
            vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vi.pNext = NULL;
			vi.vertexBindingDescriptionCount = 1;
			vi.pVertexBindingDescriptions = &m_VertexBindingDescription;
			vi.vertexAttributeDescriptionCount = pipelineCI.numVertexLayout;
			vi.pVertexAttributeDescriptions = vertexInputDescription.data();

			VkPipelineInputAssemblyStateCreateInfo inputAssemblyCI{};
			inputAssemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyCI.pNext = NULL;
			inputAssemblyCI.primitiveRestartEnable = VK_FALSE;
		
            switch(pipelineCI.drawType)
            {
                case DrawType::TRIANGLE :
                inputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                break;
                case DrawType::LINES :
                inputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
                break;
                case DrawType::POINT :
                inputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
                break;
                default :
                inputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                break;
            }
        
            VkPipelineRasterizationStateCreateInfo rs{};

            switch(pipelineCI.polygonMode)
            {
                case Graphics::PolygonMode::Fill :
                rs.polygonMode = VK_POLYGON_MODE_FILL;
                break;
                case Graphics::PolygonMode::Line :
                rs.polygonMode = VK_POLYGON_MODE_LINE;
                break;
                case Graphics::PolygonMode::Point :
                rs.polygonMode = VK_POLYGON_MODE_POINT;
                break;
                default :
                rs.polygonMode = VK_POLYGON_MODE_FILL;
                break;
            }
        
			rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rs.pNext = NULL;
			rs.cullMode = VKTools::CullModeToVK(pipelineCI.cullMode);
			rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rs.depthClampEnable = VK_FALSE;
			rs.rasterizerDiscardEnable = VK_FALSE;
			rs.depthBiasEnable = (pipelineCI.depthBiasEnabled ? VK_TRUE : VK_FALSE);
			rs.depthBiasConstantFactor = 0;
			rs.depthBiasClamp = 0;
			rs.depthBiasSlopeFactor = 0;
			rs.lineWidth = 1.0f;

			VkPipelineColorBlendStateCreateInfo cb{};
			cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			cb.pNext = NULL;
			cb.flags = 0;

			std::vector<VkPipelineColorBlendAttachmentState> blendAttachState;
			blendAttachState.resize(pipelineCI.numColorAttachments);

			for (unsigned int i = 0; i < blendAttachState.size(); i++)
			{
				blendAttachState[i] = VkPipelineColorBlendAttachmentState();
				blendAttachState[i].colorWriteMask = 0x0f;
				blendAttachState[i].alphaBlendOp = VK_BLEND_OP_ADD;
				blendAttachState[i].colorBlendOp = VK_BLEND_OP_ADD;

				if (pipelineCI.transparencyEnabled)
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

			VkPipelineViewportStateCreateInfo vp{};
			vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			vp.pNext = NULL;
			vp.viewportCount = 1;
			vp.scissorCount = 1;
			vp.pScissors = NULL;
			vp.pViewports = NULL;
			dynamicStateEnables[dynamicStateCI.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
			dynamicStateEnables[dynamicStateCI.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
        
            if(pipelineCI.lineWidth > 0.0f)
            {
                dynamicStateEnables[dynamicStateCI.dynamicStateCount++] = VK_DYNAMIC_STATE_LINE_WIDTH;
                m_LineWidth = pipelineCI.lineWidth;
            }

			if (pipelineCI.depthBiasEnabled)
				dynamicStateEnables[dynamicStateCI.dynamicStateCount++] = VK_DYNAMIC_STATE_DEPTH_BIAS;

			VkPipelineDepthStencilStateCreateInfo ds{};
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
			ds.stencilTestEnable = VK_FALSE;
			ds.front = ds.back;

			VkPipelineMultisampleStateCreateInfo ms;
			ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			ms.pNext = NULL;
			ms.pSampleMask = NULL;
			ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			ms.sampleShadingEnable = VK_FALSE;
			ms.alphaToCoverageEnable = VK_FALSE;
			ms.alphaToOneEnable = VK_FALSE;
			ms.minSampleShading = 0.0;

			VkGraphicsPipelineCreateInfo graphicsPipelineCI{};
			graphicsPipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			graphicsPipelineCI.pNext = NULL;
			graphicsPipelineCI.layout = m_PipelineLayout;
			graphicsPipelineCI.basePipelineHandle = VK_NULL_HANDLE;
			graphicsPipelineCI.basePipelineIndex = -1;
			graphicsPipelineCI.pVertexInputState = &vi;
			graphicsPipelineCI.pInputAssemblyState = &inputAssemblyCI;
			graphicsPipelineCI.pRasterizationState = &rs;
			graphicsPipelineCI.pColorBlendState = &cb;
			graphicsPipelineCI.pTessellationState = VK_NULL_HANDLE;
			graphicsPipelineCI.pMultisampleState = &ms;
			graphicsPipelineCI.pDynamicState = &dynamicStateCI;
			graphicsPipelineCI.pViewportState = &vp;
			graphicsPipelineCI.pDepthStencilState = &ds;
			graphicsPipelineCI.pStages = static_cast<VKShader*>(pipelineCI.shader)->GetShaderStages();
			graphicsPipelineCI.stageCount = static_cast<VKShader*>(pipelineCI.shader)->GetStageCount();
			graphicsPipelineCI.renderPass = static_cast<VKRenderpass*>(pipelineCI.renderpass)->GetRenderpass();
			graphicsPipelineCI.subpass = 0;

			VK_CHECK_RESULT(vkCreateGraphicsPipelines(VKDevice::Instance()->GetDevice(), VKDevice::Instance()->GetPipelineCache(), 1, &graphicsPipelineCI, VK_NULL_HANDLE, &m_Pipeline));

			return true;
		}

		void VKPipeline::Unload() const
		{
			vkDestroyDescriptorPool(VKDevice::Instance()->GetDevice(), m_DescriptorPool, VK_NULL_HANDLE);
			vkDestroyPipelineLayout(VKDevice::Instance()->GetDevice(), m_PipelineLayout, VK_NULL_HANDLE);

			for (auto& descriptorLayout : m_DescriptorLayouts)
				vkDestroyDescriptorSetLayout(VKDevice::Instance()->GetDevice(), descriptorLayout, VK_NULL_HANDLE);

			vkDestroyPipeline(VKDevice::Instance()->GetDevice(), m_Pipeline, VK_NULL_HANDLE);
		}

		void VKPipeline::SetActive(CommandBuffer* cmdBuffer)
		{
			vkCmdBindPipeline(static_cast<VKCommandBuffer*>(cmdBuffer)->GetCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
		}

		VkDescriptorSet VKPipeline::CreateDescriptorSet()
		{
			VkDescriptorSet set;
			VkDescriptorSetAllocateInfo descSetAllocInfo[1];
			descSetAllocInfo[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descSetAllocInfo[0].pNext = NULL;
			descSetAllocInfo[0].descriptorPool = m_DescriptorPool;
			descSetAllocInfo[0].descriptorSetCount = 1;
			descSetAllocInfo[0].pSetLayouts = m_DescriptorLayouts.data();
			VK_CHECK_RESULT(vkAllocateDescriptorSets(VKDevice::Instance()->GetDevice(), descSetAllocInfo, &set));
			return set;
		}
        
        void VKPipeline::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
        }

		Pipeline* VKPipeline::CreateFuncVulkan(const PipelineInfo& pipelineCI)
        {
            return lmnew VKPipeline(pipelineCI);
        }
	}
}
