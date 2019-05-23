#pragma once
#include "LM.h"

namespace lumos
{
	class LUMOS_EXPORT AINode
	{
	public:
		AINode();
		virtual ~AINode();

		virtual void Update(float dt) = 0;
		
		
	};

	inline AINode::AINode()
	{
	}

	inline AINode::~AINode()
	{
	}
}
