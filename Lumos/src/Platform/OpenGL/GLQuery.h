#pragma once

#include "Graphics/API/Query.h"

namespace Lumos
{
	namespace Graphics
	{
		class GLQuery : public Query
		{
		public:
			explicit GLQuery(QueryType type);
			~GLQuery();

			void Begin() override;
			u32 GetResult() override;
			bool GetResultReady() override;
			void End() override;
            
            static void MakeDefault();
        protected:
            static Query* CreateFuncGL(QueryType type);
		private:
			u32 m_Handle;
			u32 m_QueryType;
		};
	}
}
