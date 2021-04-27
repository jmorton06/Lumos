#include "Precompiled.h"
#include "GLPipeline.h"
#include "GLDescriptorSet.h"
#include "GLShader.h"

#include <glad/glad.h>

namespace Lumos
{
    namespace Graphics
    {
        GLPipeline::GLPipeline(const PipelineInfo& pipelineCreateInfo)
            : m_RenderPass(nullptr)
        {
            Init(pipelineCreateInfo);
        }

        GLPipeline::~GLPipeline()
        {
            delete m_DescriptorSet;
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

        bool GLPipeline::Init(const PipelineInfo& pipelineCreateInfo)
        {
            DescriptorInfo info;
            info.pipeline = this;
            info.layoutIndex = 0;
            info.shader = pipelineCreateInfo.shader.get();
            m_DescriptorSet = new GLDescriptorSet(info);
            m_TransparencyEnabled = pipelineCreateInfo.transparencyEnabled;

            GLCall(glGenVertexArrays(1, &m_VertexArray));

            m_Shader = info.shader;
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

        Pipeline* GLPipeline::CreateFuncGL(const PipelineInfo& pipelineCreateInfo)
        {
            return new GLPipeline(pipelineCreateInfo);
        }
    }
}
