#pragma once
#include "lmpch.h"
#include "Plane.h"
#include "Matrix4.h"
#include "BoundingBox.h"
#include "Core/Serialisable.h"

namespace Lumos
{
	namespace Maths
	{
		class LUMOS_EXPORT Frustum
		{
		public:

			Frustum() = default;
			~Frustum() = default;

			void FromMatrix(const Matrix4 &viewProj);
			bool InsideFrustum(const Vector3& position, float radius) const;
			bool AABBInsideFrustum(const Vector3 &position, const Vector3 &size) const;
			
			const Plane& GetPlane(int idx) const { return planes[idx]; }

			nlohmann::json Serialise();
			void Deserialise(nlohmann::json& data);

		protected:

			Plane planes[6];
		};
	}
}
