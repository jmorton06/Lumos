#include "LM.h"
#include "BoundingBox.h"
#include "MathsUtilities.h"

namespace lumos
{
	namespace maths
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
			m_Lower.SetX(Min(m_Lower.GetX(), point.GetX()));
			m_Lower.SetY(Min(m_Lower.GetY(), point.GetY()));
			m_Lower.SetZ(Min(m_Lower.GetZ(), point.GetZ()));

			m_Upper.SetX(Max(m_Upper.GetX(), point.GetX()));
			m_Upper.SetY(Max(m_Upper.GetY(), point.GetY()));
			m_Upper.SetZ(Max(m_Upper.GetZ(), point.GetZ()));
		}

		void BoundingBox::ExpandToFit(const BoundingBox &otherBox)
		{
			m_Lower.SetX(Min(m_Lower.GetX(), otherBox.m_Lower.GetX()));
			m_Lower.SetY(Min(m_Lower.GetY(), otherBox.m_Lower.GetY()));
			m_Lower.SetZ(Min(m_Lower.GetZ(), otherBox.m_Lower.GetZ()));

			m_Upper.SetX(Max(m_Upper.GetX(), otherBox.m_Lower.GetX()));
			m_Upper.SetY(Max(m_Upper.GetY(), otherBox.m_Lower.GetY()));
			m_Upper.SetZ(Max(m_Upper.GetZ(), otherBox.m_Lower.GetZ()));

			m_Lower.SetX(Min(m_Lower.GetX(), otherBox.m_Upper.GetX()));
			m_Lower.SetY(Min(m_Lower.GetY(), otherBox.m_Upper.GetY()));
			m_Lower.SetZ(Min(m_Lower.GetZ(), otherBox.m_Upper.GetZ()));

			m_Upper.SetX(Max(m_Upper.GetX(), otherBox.m_Upper.GetX()));
			m_Upper.SetY(Max(m_Upper.GetY(), otherBox.m_Upper.GetY()));
			m_Upper.SetZ(Max(m_Upper.GetZ(), otherBox.m_Upper.GetZ()));
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
			float x = maths::Abs(dims.GetX());
			float y = maths::Abs(dims.GetY());
			float z = maths::Abs(dims.GetZ());

			float radius = maths::Max(x ,maths::Max(y, z)) * 0.5f;
			return radius;
		}

		BoundingBox BoundingBox::Transform(const Matrix4 &transformation) const
		{
			BoundingBox retVal;

			retVal.ExpandToFit(transformation * Vector3(m_Lower.GetX(), m_Lower.GetY(), m_Lower.GetZ()));
			retVal.ExpandToFit(transformation * Vector3(m_Upper.GetX(), m_Lower.GetY(), m_Lower.GetZ()));
			retVal.ExpandToFit(transformation * Vector3(m_Lower.GetX(), m_Upper.GetY(), m_Lower.GetZ()));
			retVal.ExpandToFit(transformation * Vector3(m_Upper.GetX(), m_Upper.GetY(), m_Lower.GetZ()));

			retVal.ExpandToFit(transformation * Vector3(m_Lower.GetX(), m_Lower.GetY(), m_Upper.GetZ()));
			retVal.ExpandToFit(transformation * Vector3(m_Upper.GetX(), m_Lower.GetY(), m_Upper.GetZ()));
			retVal.ExpandToFit(transformation * Vector3(m_Lower.GetX(), m_Upper.GetY(), m_Upper.GetZ()));
			retVal.ExpandToFit(transformation * Vector3(m_Upper.GetX(), m_Upper.GetY(), m_Upper.GetZ()));

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

			if ((m_Upper.GetX() < otherBox.m_Lower.GetX() || m_Lower.GetX() > otherBox.m_Upper.GetX()) ||
				(m_Upper.GetY() < otherBox.m_Lower.GetY() || m_Lower.GetY() > otherBox.m_Upper.GetY()) ||
				(m_Upper.GetZ() < otherBox.m_Lower.GetZ() || m_Lower.GetZ() > otherBox.m_Upper.GetZ()))
				return false;
			else if ((m_Lower.GetX() <= otherBox.m_Lower.GetX() && m_Upper.GetX() >= otherBox.m_Upper.GetX()) &&
				(m_Lower.GetY() <= otherBox.m_Lower.GetY() && m_Upper.GetY() >= otherBox.m_Upper.GetY()) &&
				(m_Lower.GetZ() <= otherBox.m_Lower.GetZ() && m_Upper.GetZ() >= otherBox.m_Upper.GetZ()))
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

