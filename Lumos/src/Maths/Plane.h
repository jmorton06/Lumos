#pragma once
#include "LM.h"
#include "Vector3.h"

namespace Lumos
{
	namespace Maths 
	{
		class LUMOS_EXPORT Plane 
		{
		public:
			Plane() : m_Normal(0) 
			{
			}

			Plane(const Vector3 &normal, float distance, bool normalise = false);

			~Plane() {};

			//Sets the planes normal, which should be Unit Length
			void SetNormal(const Vector3 &normal) { m_Normal = normal; }

			//Gets the planes normal.
			Vector3 GetNormal() const { return m_Normal; }

			//Sets the planes distance from the origin
			void SetDistance(const float dist) { m_Distance = dist; }

			//Gets the planes distance from the origin
			float GetDistance() const { return m_Distance; }

			//Performs a simple sphere / plane test
			bool SphereInPlane(const Vector3 &position, float radius) const;

			//Performs a simple sphere / point test
			bool PointInPlane(const Vector3 &position) const;

		protected:
			//Unit-length plane normal
			Vector3 m_Normal;
			//Distance from origin
			float m_Distance;
		};
	}
}
