#pragma once
#include "Sprite.h"

namespace Lumos::Graphics
{
	class AnimatedSprite : public Sprite
	{
		public:
		AnimatedSprite() = default;
		~AnimatedSprite() = default;
		
		void OnUpdate(float dt);
		
		private:
		float m_AnimationTime = 0.0f;
	};
}