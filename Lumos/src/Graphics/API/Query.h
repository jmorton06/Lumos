#pragma once
#include "lmpch.h"

namespace Lumos
{
	namespace Graphics
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
			virtual u32 GetResult() = 0;
			virtual bool GetResultReady() = 0;
			virtual void End() = 0;

			_FORCE_INLINE_ bool GetInUse() const { return m_InUse; }

			bool m_InUse;
            
        protected:
            static Query* (*CreateFunc)(QueryType);
		};
	}
}

