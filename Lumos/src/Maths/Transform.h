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

            void SetWorldMatrix(const Matrix4& mat);
			void SetWorldPosition(const Vector3& worldPos);
			void SetLocalPosition(const Vector3& localPos);
            
            void SetLocalTransform(const Matrix4& localMat);

			void SetWorldScale(const Vector3& worldScale);
			void SetLocalScale(const Vector3& localScale);
			void SetLocalOrientation(const Quaternion& quat);
            void SetWorldOrientation(const Quaternion& quat);

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

			bool HasUpdated() const { return m_HasUpdated; }
			void SetHasUpdated(bool set) { m_HasUpdated = set; }

		protected:
			Matrix4		m_LocalMatrix;
			Matrix4		m_WorldMatrix;

			Vector3		m_LocalPosition;
			Vector3		m_LocalScale;
			Quaternion	m_LocalOrientation;

			bool m_HasUpdated = false;
		};
	}
}
