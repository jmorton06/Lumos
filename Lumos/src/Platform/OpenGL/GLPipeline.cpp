#include "Precompiled.h"
#include "GLPipeline.h"
#include "GLDescriptorSet.h"
#include "GLShader.h"

namespace Lumos
{
    namespace Graphics
    {
        GLPipeline::GLPipeline(const PipelineInfo &pipelineCI) : m_RenderPass(nullptr)
        {
            Init(pipelineCI);
        }

        GLPipeline::~GLPipeline()
        {
            delete m_DescriptorSet;
        }

        bool GLPipeline::Init(const PipelineInfo &pipelineCI)
        {
            DescriptorInfo info;
            info.pipeline = this;
            info.layoutIndex = 0;
            info.shader = pipelineCI.shader;
			m_DescriptorSet = new GLDescriptorSet(info);
            m_TransparencyEnabled = pipelineCI.transparencyEnabled;

			m_Shader = info.shader;

            return true;
        }

        void GLPipeline::Bind(Graphics::CommandBuffer* cmdBuffer)
        {
            if(m_TransparencyEnabled)
                glEnable(GL_BLEND);
            else
                glDisable(GL_BLEND);

            m_Shader->Bind();
        }

		void GLPipeline::MakeDefault()
		{
			CreateFunc = CreateFuncGL;
		}

		Pipeline* GLPipeline::CreateFuncGL(const PipelineInfo & pipelineCI)
		{
			return new GLPipeline(pipelineCI);
		}
    }
}
