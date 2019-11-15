#pragma once
#include "lmpch.h"
#include "BoundingShape.h"
#include "Core/Serialisable.h"

namespace Lumos
{
	namespace Maths
	{
		class LUMOS_EXPORT BoundingBox : public BoundingShape
		{
		public:
			BoundingBox();
			BoundingBox(const Vector3 &lower, const Vector3 &upper);
			virtual ~BoundingBox();

			void Reset() override;

			void ExpandToFit(const Vector3 &point) override;
			void ExpandToFit(const BoundingBox &otherBox);

			void SetHalfDimensions(const Vector3 &halfDims);

			Vector3 &Lower() { return m_Lower; }
			Vector3 &Upper() { return m_Upper; }

			const Vector3 &Lower() const { return m_Lower; }
			const Vector3 &Upper() const { return m_Upper; }

			Vector3 Centre() const override;

			float SphereRadius() const override;

			void SetPosition(const Vector3& pos) override;

			BoundingBox Transform(const Matrix4 &transformation) const;

			bool Intersects(const Vector3 &point) const override;
			bool Intersects(const BoundingBox &otherBox) const;

			nlohmann::json Serialise() override
			{
				nlohmann::json output;
				output["typeID"] = LUMOS_TYPENAME(BoundingBox);
				return output;
			};

			void Deserialise(nlohmann::json& data) override
			{
			};

		protected:

			Vector3 m_Lower;
			Vector3 m_Upper;
		};
	}
}
