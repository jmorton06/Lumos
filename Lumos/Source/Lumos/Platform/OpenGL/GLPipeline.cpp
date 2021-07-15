#include "Precompiled.h"
#include "GLPipeline.h"
#include "GLDescriptorSet.h"
#include "GLShader.h"

#include <glad/glad.h>

namespace Lumos
{
    namespace Graphics
    {
        GLPipeline::GLPipeline(const PipelineDesc& pipelineCreateInfo)
            : m_RenderPass(nullptr)
        {
            Init(pipelineCreateInfo);
        }

        GLPipeline::~GLPipeline()
        {
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

        bool GLPipeline::Init(const PipelineDesc& pipelineCreateInfo)
        {
            m_TransparencyEnabled = pipelineCreateInfo.transparencyEnabled;
            m_CullMode = pipelineCreateInfo.cullMode;

            GLCall(glGenVertexArrays(1, &m_VertexArray));

            m_Shader = pipelineCreateInfo.shader.get();
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
            m_Shader->Bind();

            if(m_TransparencyEnabled)
                glEnable(GL_BLEND);
            else
                glDisable(GL_BLEND);

            glEnable(GL_CULL_FACE);

            switch(m_CullMode)
            {
            case CullMode::BACK:
                glCullFace(GL_BACK);
                break;
            case CullMode::FRONT:
                glCullFace(GL_FRONT);
                break;
            case CullMode::FRONTANDBACK:
                glCullFace(GL_FRONT_AND_BACK);
                break;
            case CullMode::NONE:
                glDisable(GL_CULL_FACE);
                break;
            }

            GLCall(glFrontFace(GL_CCW));
        }

        void GLPipeline::MakeDefault()
        {
            CreateFunc = CreateFuncGL;
        }

        Pipeline* GLPipeline::CreateFuncGL(const PipelineDesc& pipelineCreateInfo)
        {
            return new GLPipeline(pipelineCreateInfo);
        }
    }
}
