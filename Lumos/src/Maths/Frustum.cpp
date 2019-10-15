#include "lmpch.h"
#include "Frustum.h"

namespace Lumos
{
	namespace Maths
	{
		void Frustum::FromMatrix(const Matrix4 &mat)
		{
			const Vector3 xaxis = Vector3(mat[0], mat[4], mat[8]);
			const Vector3 yaxis = Vector3(mat[1], mat[5], mat[9]);
			const Vector3 zaxis = Vector3(mat[2], mat[6], mat[10]);
			const Vector3 waxis = Vector3(mat[3], mat[7], mat[11]);

			planes[0] = Plane(waxis - xaxis, (mat[15] - mat[12]), true); // RIGHT
			planes[1] = Plane(waxis + xaxis, (mat[15] + mat[12]), true); // LEFT
			planes[2] = Plane(waxis + yaxis, (mat[15] + mat[13]), true); // BOTTOM
			planes[3] = Plane(waxis - yaxis, (mat[15] - mat[13]), true); // TOP
			planes[4] = Plane(waxis - zaxis, (mat[15] - mat[14]), true); // FAR
			planes[5] = Plane(waxis + zaxis, (mat[15] + mat[14]), true); // NEAR
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

		bool Frustum::AABBInsideFrustum(const Vector3 &position, const Vector3 &size) const
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

		nlohmann::json Frustum::Serialise()
		{
			nlohmann::json output;
			output["typeID"] = LUMOS_TYPENAME(Matrix4);

			nlohmann::json data = nlohmann::json::array_t();

			for (int i = 0; i < 6; i++)
			{
				data.push_back(planes[i].Serialise());
			}

			output["values"] = data;

			return output;
		}
		
		void Frustum::Deserialise(nlohmann::json & data)
		{
			nlohmann::json::array_t dataArray = data["values"];

			for (int i = 0; i < 6; i++)
			{
				planes[i].Deserialise(dataArray[i]);
			}
		}
	}
}