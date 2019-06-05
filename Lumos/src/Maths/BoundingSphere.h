#pragma once
#include "LM.h"
#include "BoundingShape.h"

namespace Lumos
{
	namespace Maths
	{
		class LUMOS_EXPORT BoundingSphere : public BoundingShape
		{
		public:
			BoundingSphere();
			BoundingSphere(const Vector3 &postion, float radius);
			virtual ~BoundingSphere();

			void Reset() override;

			void ExpandToFit(const Vector3 &point) override;
			void ExpandToFit(const BoundingSphere &otherBox);

			void SetHalfDimensions(const Vector3 &halfDims);

			Vector3 Centre() const override { return m_Position; };
			float SphereRadius() const override
			{
				return m_Radius;
			}

			void Transform(const Matrix4 &transformation) const;

			bool Intersects(const Vector3 &point) const override;
			bool Intersects(const BoundingSphere &otherBox) const;

			void SetPosition(const Vector3& pos) override { m_Position = pos; }
			void SetRadius(float radius) override { m_Radius = radius; }

		protected:

			Vector3 m_Position;
			float m_Radius;
		};
	}
}