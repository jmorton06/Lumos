#include "LM.h"
#include "VKPipeline.h"
#include "VKDevice.h"
#include "VKCommandBuffer.h"
#include "VKRenderpass.h"
#include "VKShader.h"
#include "VKInitialisers.h"
#include "VKTools.h"
#include "Graphics/API/DescriptorSet.h"


namespace Lumos
{
	namespace graphics
	{

		VKPipeline::VKPipeline()
		{
			m_PipelineLayout = VK_NULL_HANDLE;
			m_Pipeline = VK_NULL_HANDLE;
			m_DescriptorPool = VK_NULL_HANDLE;
		}

		VKPipeline::VKPipeline(const api::PipelineInfo& pipelineCI)
		{
			m_PipelineLayout = VK_NULL_HANDLE;
			m_Pipeline = VK_NULL_HANDLE;
			m_DescriptorPool = VK_NULL_HANDLE;
			Init(pipelineCI);
		}

		VKPipeline::~VKPipeline()
		{
			Unload();
			m_DescriptorPool = VK_NULL_HANDLE;
			m_PipelineLayout = VK_NULL_HANDLE;
			m_Pipeline = VK_NULL_HANDLE;

            delete descriptorSet;
		}

		bool VKPipeline::Init(const api::PipelineInfo& pipelineCI)
		{
			VkResult result;

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

				for (uint i = 0; i < descriptorLayout.count; i++)
				{
					auto info = descriptorLayout.layoutInfo[i];
					setLayoutBindings.push_back(initializers::descriptorSetLayoutBinding(VKTools::DescriptorTypeToVK(info.type), VKTools::ShaderStageToVK(info.stage), info.size));
				}

				// Pipeline layout
				VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
				descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				descriptorLayoutCI.bindingCount = static_cast<uint>(setLayoutBindings.size());
				descriptorLayoutCI.pBindings = setLayoutBindings.data();

				VkDescriptorSetLayout layout;
				VK_CHECK_RESULT(vkCreateDescriptorSetLayout(VKDevice::Instance()->GetDevice(), &descriptorLayoutCI, VK_NULL_HANDLE, &layout));

				m_DescriptorLayouts.push_back(layout);
			}

			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = initializers::pipelineLayoutCreateInfo(m_DescriptorLayouts.data(), static_cast<uint32_t>(m_DescriptorLayouts.size()));

			result = vkCreatePipelineLayout(VKDevice::Instance()->GetDevice(), &pipelineLayoutCreateInfo, VK_NULL_HANDLE, &m_PipelineLayout);
			if (result != VK_SUCCESS)
				return false;

			std::vector<VkDescriptorPoolSize> poolSizes;
			poolSizes.reserve(pipelineCI.numLayoutBindings);

			for (uint i = 0; i < pipelineCI.numLayoutBindings; i++)
			{
				auto info = pipelineCI.typeCounts[i];
				poolSizes.push_back(initializers::descriptorPoolSize(VKTools::DescriptorTypeToVK(info.type), info.size));
			}

			// Descriptor pool
			VkDescriptorPoolCreateInfo descriptorPoolInfo =
				initializers::descriptorPoolCreateInfo(
					static_cast<uint32_t>(poolSizes.size()),
					poolSizes.data(),
					pipelineCI.maxObjects);

			VK_CHECK_RESULT(vkCreateDescriptorPool(VKDevice::Instance()->GetDevice(), &descriptorPoolInfo, nullptr, &m_DescriptorPool));

			api::DescriptorInfo info;
			info.pipeline = this;
			info.layoutIndex = 0;
            info.shader = pipelineCI.shader;

			descriptorSet = new VKDescriptorSet(info);

			// Pipeline
			VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];
			VkPipelineDynamicStateCreateInfo dynamicStateCI{};
			memset(dynamicStateEnables, 0, sizeof(dynamicStateEnables));
			dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicStateCI.pNext = NULL;
			dynamicStateCI.pDynamicStates = dynamicStateEnables;
			dynamicStateCI.dynamicStateCount = 0;

			std::vector<VkVertexInputAttributeDescription> vertexInputDescription;

			vertexInputDescription.reserve(pipelineCI.numVertexLayout);
			for(uint i = 0; i < pipelineCI.numVertexLayout; i++)
			{
				vertexInputDescription.push_back(VKTools::VertexInputDescriptionToVK(pipelineCI.vertexLayout[i]));
			}

			VkPipelineVertexInputStateCreateInfo vi{};
			memset(&vi, 0, sizeof(vi));
			vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vi.pNext = NULL;
			vi.flags = 0;
			vi.vertexBindingDescriptionCount = 1;
			vi.pVertexBindingDescriptions = &m_VertexBindingDescription;
			vi.vertexAttributeDescriptionCount = pipelineCI.numVertexLayout;
			vi.pVertexAttributeDescriptions = vertexInputDescription.data();

			VkPipelineInputAssemblyStateCreateInfo inputAssemblyCI{};
			inputAssemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyCI.pNext = NULL;
			inputAssemblyCI.primitiveRestartEnable = VK_FALSE;
			inputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

			VkPipelineRasterizationStateCreateInfo rs{};
			rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rs.pNext = NULL;
			rs.polygonMode = (pipelineCI.wireframeEnabled ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL);
			rs.cullMode = VKTools::CullModeToVK(pipelineCI.cullMode);
			rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
			rs.depthClampEnable = VK_FALSE;
			rs.rasterizerDiscardEnable = VK_FALSE;
			rs.depthBiasEnable = (pipelineCI.depthBiasEnabled ? VK_TRUE : VK_FALSE);
			rs.depthBiasConstantFactor = 0;
			rs.depthBiasClamp = 0;
			rs.depthBiasSlopeFactor = 0;
			rs.lineWidth = 1.0f;

			VkPipelineColorBlendStateCreateInfo cb{};
			cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			cb.flags = 0;
			cb.pNext = NULL;

			std::vector<VkPipelineColorBlendAttachmentState> blendAttachState;
			blendAttachState.resize(pipelineCI.numColorAttachments);

			for (unsigned int i = 0; i < blendAttachState.size(); i++)
			{
				if (pipelineCI.transparencyEnabled)
				{
					blendAttachState[i] = {};
					blendAttachState[i].colorWriteMask = 0x0f;
					blendAttachState[i].blendEnable = VK_TRUE;
					blendAttachState[i].alphaBlendOp = VK_BLEND_OP_ADD;
					blendAttachState[i].colorBlendOp = VK_BLEND_OP_ADD;
					blendAttachState[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
					blendAttachState[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
					blendAttachState[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
					blendAttachState[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
				}
				else
				{
					blendAttachState[i] = {};
					blendAttachState[i].colorWriteMask = 0xf;
					blendAttachState[i].blendEnable = VK_FALSE;
					blendAttachState[i].alphaBlendOp = VK_BLEND_OP_ADD;
					blendAttachState[i].colorBlendOp = VK_BLEND_OP_ADD;
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
			ms.flags = 0;
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
			graphicsPipelineCI.basePipelineIndex = 0;
			graphicsPipelineCI.flags = 0;
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
			graphicsPipelineCI.renderPass = static_cast<VKRenderpass*>(pipelineCI.vulkanRenderpass)->GetRenderpass();
			graphicsPipelineCI.subpass = 0;

			result = vkCreateGraphicsPipelines(VKDevice::Instance()->GetDevice(), VKDevice::Instance()->GetPipelineCache(),
				1,
				&graphicsPipelineCI, VK_NULL_HANDLE, &m_Pipeline);

			return true;
		}

		void VKPipeline::Unload() const
		{
			vkDestroyDescriptorPool(VKDevice::Instance()->GetDevice(), m_DescriptorPool, VK_NULL_HANDLE);
			vkDestroyPipelineLayout(VKDevice::Instance()->GetDevice(), m_PipelineLayout, VK_NULL_HANDLE);

			for(auto& descriptorLayout : m_DescriptorLayouts)
				vkDestroyDescriptorSetLayout(VKDevice::Instance()->GetDevice(), descriptorLayout, VK_NULL_HANDLE);

			vkDestroyPipeline(VKDevice::Instance()->GetDevice(), m_Pipeline, VK_NULL_HANDLE);
		}

		void VKPipeline::SetActive(api::CommandBuffer* cmdBuffer)
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
	}
}
