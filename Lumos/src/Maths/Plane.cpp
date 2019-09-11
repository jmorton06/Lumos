#include "lmpch.h"
#include "Plane.h"

namespace Lumos
{
	namespace Maths 
	{
		Plane::Plane(const Vector3 &normal, float distance, bool normalise) 
		{
			if (normalise)
			{
				const float length = Vector3::Dot(normal, normal);

				m_Normal = normal / length;
				m_Distance = distance / length;
			} 
			else
			{
				m_Normal = normal;
				m_Distance = distance;
			}
		}

		bool Plane::SphereInPlane(const Vector3 &position, float radius) const
		{
			return Vector3::Dot(position, m_Normal) + m_Distance > -radius;
		}

		bool Plane::PointInPlane(const Vector3 &position) const
		{
			return Vector3::Dot(position, m_Normal) + m_Distance >= -0.0001f;
		}
	}
}