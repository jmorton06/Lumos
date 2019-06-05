#include "LM.h"
#include "Transform.h"

namespace Lumos
{
	namespace Maths
	{
		Transform::Transform()
		{
			m_LocalPosition		= Vector3(0.0f, 0.0f, 0.0f);
			m_LocalOrientation	= Quaternion::EulerAnglesToQuaternion(0.0f,0.0f,0.0f);
			m_LocalScale		= Vector3(1.0f, 1.0f, 1.0f);
            m_LocalMatrix		= Matrix4();
            m_WorldMatrix		= Matrix4();
		}

		Transform::Transform(const Matrix4& matrix)
		{
            m_LocalPosition     = matrix.GetPositionVector();
            m_LocalOrientation  = matrix.ToQuaternion();
            m_LocalScale        = matrix.GetScaling();
			m_LocalMatrix		= matrix;
			m_WorldMatrix		= matrix;
		}

		Transform::Transform(const Vector3& position) 
		{
			m_LocalPosition		= position;
			m_LocalOrientation	= Quaternion::EulerAnglesToQuaternion(0.0f, 0.0f, 0.0f);
			m_LocalScale		= Vector3(1.0f, 1.0f, 1.0f);
			m_LocalMatrix		= Matrix4();
			m_WorldMatrix		= Matrix4();
			SetLocalPosition(position);
		}

		Transform::~Transform() = default;

		void Transform::UpdateMatrices() 
		{
			m_LocalMatrix = Matrix4::Translation(m_LocalPosition) * m_LocalOrientation.ToMatrix4() * Matrix4::Scale(m_LocalScale);
            
            m_HasUpdated = true;
		}

		void Transform::ApplyTransform()
		{
			m_LocalPosition		= m_LocalMatrix.GetPositionVector();
			m_LocalOrientation	= m_LocalMatrix.ToQuaternion();
			m_LocalScale		= m_LocalMatrix.GetScaling();
		}
        
        void Transform::SetWorldMatrix(const Matrix4 &mat)
        {
             m_WorldMatrix = mat * m_LocalMatrix;
        }
        
        void Transform::SetLocalTransform(const Matrix4& localMat)
        {
            m_LocalMatrix		= localMat;
            m_HasUpdated		= true;

			ApplyTransform();
        }

		void Transform::SetWorldPosition(const Vector3& worldPos) 
		{
			m_LocalPosition = worldPos;
		}

		void Transform::SetLocalPosition(const Vector3& localPos)
		{
			m_LocalPosition = localPos;
		}

		void Transform::SetWorldScale(const Vector3& worldScale) 
		{
			m_LocalScale = worldScale;
		}

		void Transform::SetLocalScale(const Vector3& newScale)
		{
			m_LocalScale = newScale;
		}

		void Transform::SetLocalOrientation(const Quaternion & quat)
		{
			m_LocalOrientation = quat;
		}
        
        void Transform::SetWorldOrientation(const Quaternion & quat)
        {
            m_LocalOrientation = quat;
        }
	}
}
