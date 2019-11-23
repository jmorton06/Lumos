#include "lmpch.h"
#include "BoundingSphere.h"
#include "MathsUtilities.h"
#include "BoundingBox.h"

namespace Lumos
{
	namespace Maths
	{
		BoundingSphere::BoundingSphere()
		{
			Reset();
		}

		BoundingSphere::BoundingSphere(const Vector3 &postion, float radius)
			: m_Position(postion)
			, m_Radius(radius)
		{
		}

		BoundingSphere::~BoundingSphere() = default;

		void BoundingSphere::Reset()
		{
			m_Position = Vector3(0.0f);
			m_Radius = 1.0f;
		}

		void BoundingSphere::ExpandToFit(const Vector3 &point)
		{
			Vector3 vec = point - m_Position;
			float dist = vec.Length();
			m_Radius = Maths::Max(dist, m_Radius);
		}

		void BoundingSphere::ExpandToFit(const BoundingSphere &otherBox)
		{
			Vector3 vec = otherBox.Centre() - m_Position;
			float dist = vec.Length();
			m_Radius = Maths::Max(dist, m_Radius);
		}

		void BoundingSphere::Transform(const Matrix4 &transformation) const
		{
			BoundingSphere sphere;
			sphere.SetPosition(transformation * m_Position);
			sphere.SetRadius(SphereRadius());
		}

		bool BoundingSphere::Intersects(const Vector3 &point) const
		{
			Vector3 vec = point - m_Position;
			float dist = vec.Length();
			return dist <= (m_Radius);
		}

		bool BoundingSphere::Intersects(const BoundingSphere &otherBox) const
		{
			Vector3 vec = otherBox.Centre() - m_Position;
			float dist = vec.Length();
			return dist <= (otherBox.SphereRadius() + m_Radius);
		}
	}
}