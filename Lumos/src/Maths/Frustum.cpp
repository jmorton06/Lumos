#include "LM.h"
#include "Frustum.h"

namespace Lumos
{
	namespace maths
	{
		void Frustum::FromMatrix(const Matrix4 &mat)
		{
			const Vector3 xaxis = Vector3(mat.values[0], mat.values[4], mat.values[8]);
			const Vector3 yaxis = Vector3(mat.values[1], mat.values[5], mat.values[9]);
			const Vector3 zaxis = Vector3(mat.values[2], mat.values[6], mat.values[10]);
			const Vector3 waxis = Vector3(mat.values[3], mat.values[7], mat.values[11]);

			planes[0] = Plane(waxis - xaxis, (mat.values[15] - mat.values[12]), true); // RIGHT
			planes[1] = Plane(waxis + xaxis, (mat.values[15] + mat.values[12]), true); // LEFT
			planes[2] = Plane(waxis + yaxis, (mat.values[15] + mat.values[13]), true); // BOTTOM
			planes[3] = Plane(waxis - yaxis, (mat.values[15] - mat.values[13]), true); // TOP
			planes[4] = Plane(waxis - zaxis, (mat.values[15] - mat.values[14]), true); // FAR
			planes[5] = Plane(waxis + zaxis, (mat.values[15] + mat.values[14]), true); // NEAR
		}

		bool Frustum::InsideFrustum(const Vector3 &position, float radius) const
		{
			for (const auto & plane : planes)
			{
				if (!plane.SphereInPlane(position, radius))
					return false;
			}

			return true;
		}

		bool Frustum::AABBInsideFrustum(Vector3 &position, const Vector3 &size) const
		{
			for (const auto & plane : planes)
			{
				if (plane.PointInPlane(Vector3(position.GetX() - size.GetX(), position.GetY() + size.GetY(), position.GetZ() + size.GetZ())))
					continue;

				if (plane.PointInPlane(Vector3(position.GetX() + size.GetX(), position.GetY() + size.GetY(), position.GetZ() + size.GetZ())))
					continue;

				if (plane.PointInPlane(Vector3(position.GetX() - size.GetX(), position.GetY() - size.GetY(), position.GetZ() + size.GetZ())))
					continue;

				if (plane.PointInPlane(Vector3(position.GetX() + size.GetX(), position.GetY() - size.GetY(), position.GetZ() + size.GetZ())))
					continue;

				if (plane.PointInPlane(Vector3(position.GetX() - size.GetX(), position.GetY() + size.GetY(), position.GetZ() - size.GetZ())))
					continue;

				if (plane.PointInPlane(Vector3(position.GetX() + size.GetX(), position.GetY() + size.GetY(), position.GetZ() - size.GetZ())))
					continue;

				if (plane.PointInPlane(Vector3(position.GetX() - size.GetX(), position.GetY() - size.GetY(), position.GetZ() - size.GetZ())))
					continue;

				if (plane.PointInPlane(Vector3(position.GetX() + size.GetX(), position.GetY() - size.GetY(), position.GetZ() - size.GetZ())))
					continue;

				return false;
			}

			return true;
		}
	}
}