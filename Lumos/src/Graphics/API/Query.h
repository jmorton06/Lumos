#pragma once
#include "LM.h"

namespace lumos
{
	namespace graphics
	{
		enum class LUMOS_EXPORT QueryType
		{
			SAMPLES_PASSED,
			ANY_SAMPLES_PASSED
		};

		class Query
		{
		public:
			virtual ~Query() = default;
			static Query* Create(QueryType type);

			virtual void Begin() = 0;
			virtual uint GetResult() = 0;
			virtual bool GetResultReady() = 0;
			virtual void End() = 0;

			inline bool GetInUse() const { return m_InUse; }

			bool m_InUse;
		};
	}
}

