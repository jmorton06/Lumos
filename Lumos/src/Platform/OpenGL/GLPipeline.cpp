#include "Precompiled.h"
#include "GLPipeline.h"
#include "GLDescriptorSet.h"
#include "GLShader.h"

#include <glad/glad.h>

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
            glDeleteVertexArrays(1, &m_VertexArray);
        }

        void VertexAtrribPointer(Format format, u32 index, size_t offset, u32 stride)
        {
            switch(format)
            {
                case Format::R32_FLOAT :
                GLCall(glVertexAttribPointer(index, 1, GL_FLOAT, false, stride, (const void*)(intptr_t)(offset)));
                break;
                case Format::R32G32_FLOAT : 
                GLCall(glVertexAttribPointer(index, 2, GL_FLOAT, false, stride, (const void*)(intptr_t)(offset)));
                break;
                case Format::R32G32B32_FLOAT : 
                GLCall(glVertexAttribPointer(index, 3, GL_FLOAT, false, stride, (const void*)(intptr_t)(offset)));
                break;
                case Format::R32G32B32A32_FLOAT : 
                GLCall(glVertexAttribPointer(index, 4, GL_FLOAT, false, stride, (const void*)(intptr_t)(offset)));
                break;
                case Format::R32_UINT : 
                GLCall(glVertexAttribPointer(index, 1, GL_UNSIGNED_INT, false, stride, (const void*)(intptr_t)(offset)));
                break;
                case Format::R8_UINT : 
                GLCall(glVertexAttribPointer(index, 1, GL_UNSIGNED_BYTE, false, stride, (const void*)(intptr_t)(offset)));
                break;
            }
        }

        bool GLPipeline::Init(const PipelineInfo &pipelineCI)
        {
            DescriptorInfo info;
            info.pipeline = this;
            info.layoutIndex = 0;
            info.shader = pipelineCI.shader;
			m_DescriptorSet = new GLDescriptorSet(info);
            m_TransparencyEnabled = pipelineCI.transparencyEnabled;

            GLCall(glGenVertexArrays(1, &m_VertexArray));

			m_Shader = info.shader;
        
            auto vLayout = pipelineCI.vertexLayout;
            for(u32 i = 0; i < pipelineCI.numVertexLayout; i++)
            {
                m_VertexDescriptions.push_back(vLayout[i]);
            }
            
            m_StrideSize = (u32)pipelineCI.strideSize;

            return true;
        }
    
        void GLPipeline::BindVertexArray()
        {
            GLCall(glBindVertexArray(m_VertexArray));

            for(u32 i = 0; i < m_VertexDescriptions.size(); i++)
            {
                GLCall(glEnableVertexAttribArray(i));
                auto layout = m_VertexDescriptions[i];
                size_t offset = static_cast<size_t>(layout.offset);
                VertexAtrribPointer(layout.format, i, offset, m_StrideSize);
            }
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
