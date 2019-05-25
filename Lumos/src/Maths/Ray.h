#pragma once
#include "LM.h"
#include "Vector3.h"

namespace lumos
{	
	namespace maths
	{
		struct RayCollision 
		{
			void*		node;
			Vector3		collidedAt;
			float		rayDistance;

			RayCollision(void* node, const Vector3& collidedAt)
			{
				this->node = node;
				this->collidedAt = collidedAt;
				this->rayDistance = 0.0f;
			}

			RayCollision() 
			{
				node = nullptr;
				rayDistance = FLT_MAX;
			}
		};

		class LUMOS_EXPORT Ray 
		{
		public:
			Ray(const Vector3& position, const Vector3& direction) 
			{
				this->position = position;
				this->direction = direction;
			}
			~Ray() = default;

			Vector3 GetPosition() const { return position; }

			Vector3 GetDirection() const { return direction; }

			Ray GetReverseRay() const 
			{
				float x = -direction.x;
				float y = -direction.y;
				float z = -direction.z;
				Vector3 reDir(x, y, z);
				return Ray(position, reDir);
			}

		protected:
			Vector3 position;
			Vector3 direction;
		};
	}
}
