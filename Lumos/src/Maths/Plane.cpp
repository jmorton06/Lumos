#include "JM.h"
#include "Plane.h"

namespace jm {
	namespace maths {
		Plane::Plane(const Vector3 &normal, float distance, bool normalise) {
			if (normalise) {
				const float length = Vector3::Dot(normal, normal);

				this->normal = normal / length;
				this->distance = distance / length;
			} else {
				this->normal = normal;
				this->distance = distance;
			}
		}

		bool Plane::SphereInPlane(const Vector3 &position, float radius) const {
			return Vector3::Dot(position, normal) + distance > -radius;
		}

		bool Plane::PointInPlane(const Vector3 &position) const {
			return Vector3::Dot(position, normal) + distance >= -0.0001f;
		}
	}
}