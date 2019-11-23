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

	_FORCE_INLINE_ AINode::AINode()
	{
	}

	_FORCE_INLINE_ AINode::~AINode()
	{
	}
}
