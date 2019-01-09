#pragma once
#include "JM.h"
#include "Maths.h"

namespace jm
{
	namespace maths
	{
		class JM_EXPORT BoundingShape
		{
		public:
			BoundingShape() = default;
			virtual ~BoundingShape() = default;

			virtual void Reset() = 0;
			virtual void ExpandToFit(const Vector3 &point) = 0;

			virtual Vector3 Centre() const = 0;
			virtual float SphereRadius() const = 0;
			virtual void SetPosition(const Vector3& pos) = 0;


			virtual bool Intersects(const Vector3 &point) const = 0;
			virtual void SetRadius(float radius) {};
		};
	}
}
