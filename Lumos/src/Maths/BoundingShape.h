#pragma once
#include "LM.h"
#include "Maths.h"

#include "Core/Serialisable.h"

namespace Lumos
{
	namespace Maths
	{
		class LUMOS_EXPORT BoundingShape
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


			virtual nlohmann::json Serialise() = 0;
			virtual void Deserialise(nlohmann::json& data) = 0;
		};
	}
}
