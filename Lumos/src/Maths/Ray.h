#pragma once
#include "LM.h"
#include "Vector3.h"
#include "Core/Serialisable.h"

namespace Lumos
{	
	namespace Maths
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

			nlohmann::json Serialise()
			{
				nlohmann::json output;
				output["typeID"] = LUMOS_TYPENAME(Ray);
				output["position"] = position.Serialise();
				output["direction"] = direction.Serialise();

				return output;
			};

			void Deserialise(nlohmann::json& data)
			{
				position.Deserialise(data["x"]);
				direction.Deserialise(data["y"]);
			};

		protected:
			Vector3 position;
			Vector3 direction;
		};
	}
}
