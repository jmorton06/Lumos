#pragma once
#include "LM.h"
#include "Graphics/API/Query.h"

namespace Lumos
{

	class GLQuery : public Query
	{
	public:
		explicit GLQuery(QueryType type);
		~GLQuery();

		void Begin() override;
		uint GetResult() override;
		bool GetResultReady() override;
		void End() override;

	private:
		uint m_Handle;
		uint m_QueryType;
	};
}
