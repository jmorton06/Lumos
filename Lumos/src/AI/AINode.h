#pragma once
#include "LM.h"

namespace Lumos
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
