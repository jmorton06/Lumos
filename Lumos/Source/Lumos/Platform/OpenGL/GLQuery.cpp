#include "Precompiled.h"
#include "GLQuery.h"
#include "GL.h"
#include "GLDebug.h"

namespace Lumos
{
    namespace Graphics
    {
        uint32_t QueryTypeToGL(const QueryType type)
        {
            switch(type)
            {
#ifndef LUMOS_PLATFORM_MOBILE
            case QueryType::SAMPLES_PASSED:
                return GL_SAMPLES_PASSED;
#endif
            case QueryType::ANY_SAMPLES_PASSED:
                return GL_ANY_SAMPLES_PASSED;
            }
            return 0;
        }

        GLQuery::GLQuery(const QueryType type)
        {
            GLCall(glGenQueries(1, &m_Handle));
            m_QueryType = QueryTypeToGL(type);
            m_InUse = false;
        }

        GLQuery::~GLQuery()
        {
            GLCall(glDeleteQueries(1, &m_Handle));
        }

        void GLQuery::Begin()
        {
            GLCall(glBeginQuery(m_QueryType, m_Handle));
            m_InUse = true;
        }

        void GLQuery::End()
        {
            glEndQuery(m_QueryType);
        }

        void GLQuery::MakeDefault()
        {
            CreateFunc = CreateFuncGL;
        }

        Query* GLQuery::CreateFuncGL(QueryType type)
        {
            return new GLQuery(type);
        }

        uint32_t GLQuery::GetResult()
        {
            int SamplesPassed = 0;
            GLCall(glGetQueryObjectiv(m_Handle, GL_QUERY_RESULT, &SamplesPassed));
            m_InUse = false;
            return static_cast<uint32_t>(SamplesPassed);
        }

        bool GLQuery::GetResultReady()
        {
            int ResultReady = 0;
            GLCall(glGetQueryObjectiv(m_Handle, GL_QUERY_RESULT_AVAILABLE, &ResultReady));
            return ResultReady > 0;
        }
    }
}
