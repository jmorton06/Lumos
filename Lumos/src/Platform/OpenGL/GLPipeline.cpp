#include "LM.h"
#include "GLPipeline.h"
#include "GLDescriptorSet.h"
#include "GLShader.h"

namespace lumos
{
    namespace graphics
    {
        GLPipeline::GLPipeline() : m_RenderPass(nullptr)
        {
	        descriptorSet = nullptr;
        }

        GLPipeline::GLPipeline(const PipelineInfo &pipelineCI) : m_RenderPass(nullptr)
        {
            Init(pipelineCI);
        }

        GLPipeline::~GLPipeline()
        {
            delete descriptorSet;
        }

        bool GLPipeline::Init(const PipelineInfo &pipelineCI)
        {
            DescriptorInfo info;
            info.pipeline = this;
            info.layoutIndex = 0;
            info.shader = pipelineCI.shader;
            descriptorSet = new GLDescriptorSet(info);

			m_Shader = info.shader;

            return true;
        }

        void GLPipeline::SetActive(graphics::CommandBuffer* cmdBuffer)
        {
            m_Shader->Bind();
        }
    }
}
