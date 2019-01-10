#pragma once
#include "JM.h"

namespace jm
{
	class CORE_DLL AINode
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
