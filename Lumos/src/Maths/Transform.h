#pragma once
#include "LM.h"
#include "Maths/Quaternion.h"

namespace Lumos
{
	namespace maths
	{
		class LUMOS_EXPORT Transform
		{
		public:
			Transform();
			Transform(const Matrix4& matrix);
			Transform(const Vector3& position);
			~Transform();

			void SetWorldMatrix(const Matrix4& mat) { m_WorldMatrix = mat; }
			void SetWorldPosition(const Vector3& worldPos);
			void SetLocalPosition(const Vector3& localPos);

			void SetWorldScale(const Vector3& worldScale);
			void SetLocalScale(const Vector3& localScale);
			void SetOrientation(const Quaternion& quat);

			Matrix4 GetWorldMatrix() const
			{
				return m_WorldMatrix;
			}

			Matrix4 GetLocalMatrix() const
			{
				return m_LocalMatrix;
			}

			Vector3 GetWorldPosition() const
			{
				return m_WorldMatrix.GetPositionVector();
			}

			void UpdateMatrices();

		protected:
			Matrix4		m_LocalMatrix;
			Matrix4		m_WorldMatrix;

			Vector3		m_LocalPosition;
			Vector3		m_LocalScale;
			Quaternion	m_Orientation;
		};
	}
}
