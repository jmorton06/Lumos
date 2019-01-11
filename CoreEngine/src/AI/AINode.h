#pragma once
#include "JM.h"

namespace jm
{
	class JM_EXPORT AINode
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
