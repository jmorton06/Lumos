#include "LM.h"
#include "GLQuery.h"
#include "GL.h"
#include "GLDebug.h"

namespace lumos
{
	namespace graphics
	{
		uint QueryTypeToGL(const QueryType type)
		{
			switch (type)
			{
#ifndef LUMOS_PLATFORM_MOBILE
			case QueryType::SAMPLES_PASSED:	    return GL_SAMPLES_PASSED;
#endif
			case QueryType::ANY_SAMPLES_PASSED:	return GL_ANY_SAMPLES_PASSED;
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

		uint GLQuery::GetResult()
		{
			int SamplesPassed = 0;
#ifndef LUMOS_PLATFORM_MOBILE
			GLCall(glGetQueryObjectiv(m_Handle, GL_QUERY_RESULT, &SamplesPassed));
#endif
			m_InUse = false;
			return static_cast<uint>(SamplesPassed);
		}

		bool GLQuery::GetResultReady()
		{
			int ResultReady = 0;
#ifndef LUMOS_PLATFORM_MOBILE
			GLCall(glGetQueryObjectiv(m_Handle, GL_QUERY_RESULT_AVAILABLE, &ResultReady));
#endif
			return ResultReady > 0;
		}
	}
}