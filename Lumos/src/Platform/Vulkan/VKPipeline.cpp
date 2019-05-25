#include "LM.h"
#include "VKPipeline.h"
#include "VKDevice.h"
#include "VKCommandBuffer.h"
#include "VKRenderpass.h"
#include "VKShader.h"
#include "VKTools.h"
#include "Graphics/API/DescriptorSet.h"


namespace lumos
{
	namespace graphics
	{

		VKPipeline::VKPipeline()
		{
		}

		VKPipeline::VKPipeline(const PipelineInfo& pipelineCI)
		{
			Init(pipelineCI);
		}

		VKPipeline::~VKPipeline()
		{
			Unload();

            delete descriptorSet;
		}

		bool VKPipeline::Init(const PipelineInfo& pipelineCI)
		{
			m_PipelineName = pipelineCI.pipelineName;
			m_Shader	   = pipelineCI.shader;

			// Vertex layout
			m_VertexBindingDescription.binding = 0;
			m_VertexBindingDescription.inputRate = vk::VertexInputRate::eVertex;
			m_VertexBindingDescription.stride = static_cast<uint32_t>(pipelineCI.strideSize);

			for (auto& descriptorLayout : pipelineCI.descriptorLayouts)
			{

				std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings;

				setLayoutBindings.reserve(descriptorLayout.count);

				for (uint i = 0; i < descriptorLayout.count; i++)
				{
					auto info = descriptorLayout.layoutInfo[i];

					vk::DescriptorSetLayoutBinding setLayoutBinding{};
					setLayoutBinding.descriptorType = VKTools::DescriptorTypeToVK(info.type);
					setLayoutBinding.stageFlags = VKTools::ShaderStageToVK(info.stage);
					setLayoutBinding.binding = info.size;
					setLayoutBinding.descriptorCount = info.count;

					setLayoutBindings.push_back(setLayoutBinding);
				}

				// Pipeline layout
				vk::DescriptorSetLayoutCreateInfo descriptorLayoutCI{};
				descriptorLayoutCI.bindingCount = static_cast<uint>(setLayoutBindings.size());
				descriptorLayoutCI.pBindings = setLayoutBindings.data();

				vk::DescriptorSetLayout layout = VKDevice::Instance()->GetDevice().createDescriptorSetLayout(descriptorLayoutCI);

				m_DescriptorLayouts.push_back(layout);
			}

			vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
			pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(m_DescriptorLayouts.size());
			pipelineLayoutCreateInfo.pSetLayouts = m_DescriptorLayouts.data();

			m_PipelineLayout = VKDevice::Instance()->GetDevice().createPipelineLayout(pipelineLayoutCreateInfo);

			std::vector<vk::DescriptorPoolSize> poolSizes;
			poolSizes.reserve(pipelineCI.numLayoutBindings);

			for (uint i = 0; i < pipelineCI.numLayoutBindings; i++)
			{
				auto info = pipelineCI.typeCounts[i];
				vk::DescriptorPoolSize descriptorPoolSize{};
				descriptorPoolSize.type = VKTools::DescriptorTypeToVK(info.type);
				descriptorPoolSize.descriptorCount = info.size;
				poolSizes.push_back(descriptorPoolSize);
			}

			// Descriptor pool
			vk::DescriptorPoolCreateInfo descriptorPoolInfo{};
			descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
			descriptorPoolInfo.pPoolSizes = poolSizes.data();
			descriptorPoolInfo.maxSets = pipelineCI.maxObjects;

			m_DescriptorPool = VKDevice::Instance()->GetDevice().createDescriptorPool(descriptorPoolInfo);

			DescriptorInfo info;
			info.pipeline = this;
			info.layoutIndex = 0;
            info.shader = pipelineCI.shader;

			descriptorSet = new VKDescriptorSet(info);

			// Pipeline
			vk::DynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];
			vk::PipelineDynamicStateCreateInfo dynamicStateCI{};
			memset(dynamicStateEnables, 0, sizeof(dynamicStateEnables));
			dynamicStateCI.pNext = NULL;
			dynamicStateCI.pDynamicStates = dynamicStateEnables;
			dynamicStateCI.dynamicStateCount = 0;

			std::vector<vk::VertexInputAttributeDescription> vertexInputDescription;

			vertexInputDescription.reserve(pipelineCI.numVertexLayout);
			for(uint i = 0; i < pipelineCI.numVertexLayout; i++)
			{
				vertexInputDescription.push_back(VKTools::VertexInputDescriptionToVK(pipelineCI.vertexLayout[i]));
			}

			vk::PipelineVertexInputStateCreateInfo vi{};
			memset(&vi, 0, sizeof(vi));
			vi.pNext = NULL;
			vi.vertexBindingDescriptionCount = 1;
			vi.pVertexBindingDescriptions = &m_VertexBindingDescription;
			vi.vertexAttributeDescriptionCount = pipelineCI.numVertexLayout;
			vi.pVertexAttributeDescriptions = vertexInputDescription.data();

			vk::PipelineInputAssemblyStateCreateInfo inputAssemblyCI{};
			inputAssemblyCI.pNext = NULL;
			inputAssemblyCI.primitiveRestartEnable = VK_FALSE;
			inputAssemblyCI.topology = vk::PrimitiveTopology::eTriangleList;

			vk::PipelineRasterizationStateCreateInfo rs{};
			rs.pNext = NULL;
			rs.polygonMode = (pipelineCI.wireframeEnabled ? vk::PolygonMode::eLine : vk::PolygonMode::eFill);
			rs.cullMode = VKTools::CullModeToVK(pipelineCI.cullMode);
			rs.frontFace = vk::FrontFace::eCounterClockwise;
			rs.depthClampEnable = VK_FALSE;
			rs.rasterizerDiscardEnable = VK_FALSE;
			rs.depthBiasEnable = (pipelineCI.depthBiasEnabled ? VK_TRUE : VK_FALSE);
			rs.depthBiasConstantFactor = 0;
			rs.depthBiasClamp = 0;
			rs.depthBiasSlopeFactor = 0;
			rs.lineWidth = 1.0f;

			vk::PipelineColorBlendStateCreateInfo cb{};
			cb.pNext = NULL;

			std::vector<vk::PipelineColorBlendAttachmentState> blendAttachState;
			blendAttachState.resize(pipelineCI.numColorAttachments);

			for (unsigned int i = 0; i < blendAttachState.size(); i++)
			{
				if (pipelineCI.transparencyEnabled)
				{
                    blendAttachState[i] = vk::PipelineColorBlendAttachmentState();
					blendAttachState[i].colorWriteMask = vk::ColorComponentFlagBits::eR |
														 vk::ColorComponentFlagBits::eG |
														 vk::ColorComponentFlagBits::eB |
														 vk::ColorComponentFlagBits::eA;
					blendAttachState[i].blendEnable = VK_TRUE;
					blendAttachState[i].alphaBlendOp = vk::BlendOp::eAdd;
					blendAttachState[i].colorBlendOp = vk::BlendOp::eAdd;
					blendAttachState[i].srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
					blendAttachState[i].dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
					blendAttachState[i].srcAlphaBlendFactor = vk::BlendFactor::eOne;
					blendAttachState[i].dstAlphaBlendFactor = vk::BlendFactor::eZero;
				}
				else
				{
					blendAttachState[i] = vk::PipelineColorBlendAttachmentState();
					blendAttachState[i].colorWriteMask = vk::ColorComponentFlagBits::eR |
														 vk::ColorComponentFlagBits::eG |
														 vk::ColorComponentFlagBits::eB |
														 vk::ColorComponentFlagBits::eA;
					blendAttachState[i].blendEnable = VK_FALSE;
					blendAttachState[i].alphaBlendOp = vk::BlendOp::eAdd;
					blendAttachState[i].colorBlendOp = vk::BlendOp::eAdd;
					blendAttachState[i].srcColorBlendFactor = vk::BlendFactor::eZero;
					blendAttachState[i].dstColorBlendFactor = vk::BlendFactor::eZero;
					blendAttachState[i].srcAlphaBlendFactor = vk::BlendFactor::eZero;
					blendAttachState[i].dstAlphaBlendFactor = vk::BlendFactor::eZero;
				}
			}

			cb.attachmentCount = static_cast<uint32_t>(blendAttachState.size());
			cb.pAttachments = blendAttachState.data();
			cb.logicOpEnable = VK_FALSE;
			cb.logicOp = vk::LogicOp::eNoOp;
			cb.blendConstants[0] = 1.0f;
			cb.blendConstants[1] = 1.0f;
			cb.blendConstants[2] = 1.0f;
			cb.blendConstants[3] = 1.0f;

			vk::PipelineViewportStateCreateInfo vp{};
			vp.pNext = NULL;
			vp.viewportCount = 1;
			vp.scissorCount = 1;
			vp.pScissors = NULL;
			vp.pViewports = NULL;
			dynamicStateEnables[dynamicStateCI.dynamicStateCount++] = vk::DynamicState::eViewport;
			dynamicStateEnables[dynamicStateCI.dynamicStateCount++] = vk::DynamicState::eScissor;

			if (pipelineCI.depthBiasEnabled)
				dynamicStateEnables[dynamicStateCI.dynamicStateCount++] = vk::DynamicState::eDepthBias;

			vk::PipelineDepthStencilStateCreateInfo ds{};
			ds.pNext = NULL;
			ds.depthTestEnable = VK_TRUE;
			ds.depthWriteEnable = VK_TRUE;
			ds.depthCompareOp = vk::CompareOp::eLessOrEqual;
			ds.depthBoundsTestEnable = VK_FALSE;
			ds.stencilTestEnable = VK_FALSE;
			ds.back.failOp = vk::StencilOp::eKeep;
			ds.back.passOp = vk::StencilOp::eKeep;
			ds.back.compareOp = vk::CompareOp::eAlways;
			ds.back.compareMask = 0;
			ds.back.reference = 0;
			ds.back.depthFailOp = vk::StencilOp::eKeep;
			ds.back.writeMask = 0;
			ds.minDepthBounds = 0;
			ds.maxDepthBounds = 0;
			ds.stencilTestEnable = VK_FALSE;
			ds.front = ds.back;

			vk::PipelineMultisampleStateCreateInfo ms;
			ms.pNext = NULL;
			ms.pSampleMask = NULL;
			ms.rasterizationSamples = vk::SampleCountFlagBits::e1;
			ms.sampleShadingEnable = VK_FALSE;
			ms.alphaToCoverageEnable = VK_FALSE;
			ms.alphaToOneEnable = VK_FALSE;
			ms.minSampleShading = 0.0;

			vk::GraphicsPipelineCreateInfo graphicsPipelineCI{};
			graphicsPipelineCI.pNext = NULL;
			graphicsPipelineCI.layout = m_PipelineLayout;
			graphicsPipelineCI.basePipelineHandle = nullptr;
			graphicsPipelineCI.basePipelineIndex = 0;
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

			m_Pipeline = VKDevice::Instance()->GetDevice().createGraphicsPipelines(VKDevice::Instance()->GetPipelineCache(), graphicsPipelineCI)[0];

			return true;
		}

		void VKPipeline::Unload() const
		{
			VKDevice::Instance()->GetDevice().destroyDescriptorPool(m_DescriptorPool);
			VKDevice::Instance()->GetDevice().destroyPipelineLayout(m_PipelineLayout);

			for (auto& descriptorLayout : m_DescriptorLayouts)
				VKDevice::Instance()->GetDevice().destroyDescriptorSetLayout(descriptorLayout);


			VKDevice::Instance()->GetDevice().destroyPipeline(m_Pipeline);
		}

		void VKPipeline::SetActive(CommandBuffer* cmdBuffer)
		{
			vkCmdBindPipeline(static_cast<VKCommandBuffer*>(cmdBuffer)->GetCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
		}

		vk::DescriptorSet VKPipeline::CreateDescriptorSet()
		{
			vk::DescriptorSet set;
			vk::DescriptorSetAllocateInfo descSetAllocInfo[1];
			descSetAllocInfo[0].pNext = NULL;
			descSetAllocInfo[0].descriptorPool = m_DescriptorPool;
			descSetAllocInfo[0].descriptorSetCount = 1;
			descSetAllocInfo[0].pSetLayouts = m_DescriptorLayouts.data();
			VKDevice::Instance()->GetDevice().allocateDescriptorSets(descSetAllocInfo, &set);
			return set;
		}
	}
}
