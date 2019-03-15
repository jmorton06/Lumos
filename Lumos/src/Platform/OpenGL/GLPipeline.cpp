#include "LM.h"
#include "GLPipeline.h"
#include "GLDescriptorSet.h"
#include "GLShader.h"

namespace Lumos
{
    namespace graphics
    {

        GLPipeline::GLPipeline(): m_RenderPass(nullptr)
        {
	        descriptorSet = nullptr;
        }

        GLPipeline::GLPipeline(const api::PipelineInfo &pipelineCI)
        {
            Init(pipelineCI);
        }

        GLPipeline::~GLPipeline()
        {
            delete descriptorSet;
        }

        bool GLPipeline::Init(const api::PipelineInfo &pipelineCI)
        {
            api::DescriptorInfo info;
            info.pipeline = this;
            info.layoutIndex = 0;
            info.shader = pipelineCI.shader;
            descriptorSet = new GLDescriptorSet(info);

			m_Shader = info.shader;

            return true;
        }

        void GLPipeline::SetActive(graphics::api::CommandBuffer* cmdBuffer)
        {
            m_Shader->Bind();
        }
    }
}
