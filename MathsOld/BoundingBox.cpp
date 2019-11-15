#include "lmpch.h"
#include "BoundingBox.h"
#include "MathsUtilities.h"

namespace Lumos
{
	namespace Maths
	{
		BoundingBox::BoundingBox()
		{
			Reset();
		}

		BoundingBox::BoundingBox(const Vector3 &lower, const Vector3 &upper)
			: m_Lower(lower)
			, m_Upper(upper)
		{
		}

		BoundingBox::~BoundingBox() = default;

		void BoundingBox::Reset()
		{
			m_Lower = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
			m_Upper = Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		}

		void BoundingBox::ExpandToFit(const Vector3 &point)
		{
			m_Lower.x = Min(m_Lower.x, point.x));
			m_Lower.y = Min(m_Lower.y, point.y));
			m_Lower.z = Min(m_Lower.z, point.z));

			m_Upper.x = Max(m_Upper.x, point.x));
			m_Upper.y = Max(m_Upper.y, point.y));
			m_Upper.z = Max(m_Upper.z, point.z));
		}

		void BoundingBox::ExpandToFit(const BoundingBox &otherBox)
		{
			m_Lower.x = Min(m_Lower.x, otherBox.m_Lower.x));
			m_Lower.y = Min(m_Lower.y, otherBox.m_Lower.y));
			m_Lower.z = Min(m_Lower.z, otherBox.m_Lower.z));

			m_Upper.x = Max(m_Upper.x, otherBox.m_Lower.x));
			m_Upper.y = Max(m_Upper.y, otherBox.m_Lower.y));
			m_Upper.z = Max(m_Upper.z, otherBox.m_Lower.z));

			m_Lower.x = Min(m_Lower.x, otherBox.m_Upper.x));
			m_Lower.y = Min(m_Lower.y, otherBox.m_Upper.y));
			m_Lower.z = Min(m_Lower.z, otherBox.m_Upper.z));

			m_Upper.x = Max(m_Upper.x, otherBox.m_Upper.x));
			m_Upper.y = Max(m_Upper.y, otherBox.m_Upper.y));
			m_Upper.z = Max(m_Upper.z, otherBox.m_Upper.z));
		}

		void BoundingBox::SetHalfDimensions(const Vector3 &halfDims)
		{
			m_Lower = -halfDims;
			m_Upper = halfDims;
		}

		Vector3 BoundingBox::Centre() const
		{
			return m_Lower + ((m_Upper - m_Lower) * 0.5f);
		}

		float BoundingBox::SphereRadius() const
		{
			Vector3 dims = m_Upper - m_Lower;
			float x = Maths::Abs(dims.x);
			float y = Maths::Abs(dims.y);
			float z = Maths::Abs(dims.z);

			float radius = Maths::Max(x ,Maths::Max(y, z)) * 0.5f;
			return radius;
		}

		BoundingBox BoundingBox::Transform(const Matrix4 &transformation) const
		{
			BoundingBox retVal;

			retVal.ExpandToFit(transformation * Vector3(m_Lower.x, m_Lower.y, m_Lower.z));
			retVal.ExpandToFit(transformation * Vector3(m_Upper.x, m_Lower.y, m_Lower.z));
			retVal.ExpandToFit(transformation * Vector3(m_Lower.x, m_Upper.y, m_Lower.z));
			retVal.ExpandToFit(transformation * Vector3(m_Upper.x, m_Upper.y, m_Lower.z));

			retVal.ExpandToFit(transformation * Vector3(m_Lower.x, m_Lower.y, m_Upper.z));
			retVal.ExpandToFit(transformation * Vector3(m_Upper.x, m_Lower.y, m_Upper.z));
			retVal.ExpandToFit(transformation * Vector3(m_Lower.x, m_Upper.y, m_Upper.z));
			retVal.ExpandToFit(transformation * Vector3(m_Upper.x, m_Upper.y, m_Upper.z));

			return retVal;
		}

		bool BoundingBox::Intersects(const Vector3 &point) const
		{
			return m_Lower <= point && point <= m_Upper;
		}

		bool BoundingBox::Intersects(const BoundingBox &otherBox) const
		{
			/*return (m_Lower <= otherBox.m_Lower && otherBox.m_Lower <= m_Upper) ||
			((m_Lower <= otherBox.m_Upper && otherBox.m_Upper <= m_Upper));*/

			/*if (m_Upper < otherBox.m_Lower || m_Lower > otherBox.m_Upper)
			return false;
			else if (m_Lower <= otherBox.m_Lower && m_Upper >= otherBox.m_Upper)
			return true;
			else
			return true;*/

			if ((m_Upper.x < otherBox.m_Lower.x || m_Lower.x > otherBox.m_Upper.x) ||
				(m_Upper.y < otherBox.m_Lower.y || m_Lower.y > otherBox.m_Upper.y) ||
				(m_Upper.z < otherBox.m_Lower.z || m_Lower.z > otherBox.m_Upper.z))
				return false;
			else if ((m_Lower.x <= otherBox.m_Lower.x && m_Upper.x >= otherBox.m_Upper.x) &&
				(m_Lower.y <= otherBox.m_Lower.y && m_Upper.y >= otherBox.m_Upper.y) &&
				(m_Lower.z <= otherBox.m_Lower.z && m_Upper.z >= otherBox.m_Upper.z))
				return true;
			else
				return true;
		}

		void BoundingBox::SetPosition(const Vector3 &pos)
		{
			m_Upper = Matrix4::Translation(pos) * m_Upper;
			m_Lower = Matrix4::Translation(pos) *  m_Lower;
		}
	}
}

