#pragma once
#include "LM.h"
#include "Plane.h"
#include "Matrix4.h"
#include "BoundingBox.h"

namespace Lumos
{
	namespace maths
	{
		class LUMOS_EXPORT Frustum
		{
		public:

			Frustum(void) {};
			~Frustum(void) {};

			void FromMatrix(const Matrix4 &viewProj);
			bool InsideFrustum(const Vector3& position, float radius) const;
			bool AABBInsideFrustum(Vector3 &position, const Vector3 &size) const;
			Plane& GetPlane(int idx) { return planes[idx]; }

		protected:

			Plane planes[6];
		};
	}
}
