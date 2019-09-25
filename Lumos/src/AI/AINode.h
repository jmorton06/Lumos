#pragma once
#include "lmpch.h"

namespace Lumos
{
	class LUMOS_EXPORT AINode
	{
	public:
        AINode();
		virtual ~AINode();

        void Update(float dt) {};
		
		
	};

	inline AINode::AINode()
	{
	}

	inline AINode::~AINode()
	{
	}
}
