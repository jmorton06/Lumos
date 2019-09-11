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

			Frustum(void) {};
			~Frustum(void) {};

			void FromMatrix(const Matrix4 &viewProj);
			bool InsideFrustum(const Vector3& position, float radius) const;
			bool AABBInsideFrustum(Vector3 &position, const Vector3 &size) const;
			Plane& GetPlane(int idx) { return planes[idx]; }

			nlohmann::json Serialise()
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
			};

			void Deserialise(nlohmann::json& data)
			{
				nlohmann::json::array_t dataArray = data["values"];

				for (int i = 0; i < 6; i++)
				{
					planes[i].Deserialise(dataArray[i]);
				}
			};

		protected:

			Plane planes[6];
		};
	}
}
