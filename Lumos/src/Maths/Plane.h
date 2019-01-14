#pragma once
#include "LM.h"
#include "Vector3.h"

namespace Lumos {
	namespace maths {

		class LUMOS_EXPORT Plane {
		public:
			Plane() : distance(0) {
			}

			Plane(const Vector3 &normal, float distance, bool normalise = false);

			~Plane() {};

			//Sets the planes normal, which should be UNIT LENGTH!!!
			void SetNormal(const Vector3 &normal) { this->normal = normal; }

			//Gets the planes normal.
			Vector3 GetNormal() const { return normal; }

			//Sets the planes distance from the origin
			void SetDistance(const float dist) { distance = dist; }

			//Gets the planes distance from the origin
			float GetDistance() const { return distance; }

			//Performs a simple sphere / plane test
			bool SphereInPlane(const Vector3 &position, float radius) const;

			//Performs a simple sphere / point test
			bool PointInPlane(const Vector3 &position) const;

		protected:
			//Unit-length plane normal
			Vector3 normal;
			//Distance from origin
			float distance;
		};
	}
}
