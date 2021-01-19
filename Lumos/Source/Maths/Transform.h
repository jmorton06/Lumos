#pragma once

#include "Maths/Maths.h"

#include <cereal/cereal.hpp>

namespace Lumos
{
	namespace Maths
	{
		class LUMOS_EXPORT Transform
		{
		public:
			Transform();
			Transform(const Matrix4& matrix);
			Transform(const Vector3& position);
			~Transform();

            void SetWorldMatrix(const Matrix4& mat);
            
            void SetLocalTransform(const Matrix4& localMat);

			void SetLocalPosition(const Vector3& localPos);
			void SetLocalScale(const Vector3& localScale);
			void SetLocalOrientation(const Quaternion& quat);

			const Matrix4& GetWorldMatrix();
			const Matrix4& GetLocalMatrix();

			const Vector3 GetWorldPosition() const;
			const Quaternion GetWorldOrientation() const;

			const Vector3& GetLocalPosition() const;
			const Vector3& GetLocalScale() const;
			const Quaternion& GetLocalOrientation() const;

			//Updates Local Matrix from R,T and S vectors
			void UpdateMatrices();

			bool HasUpdated() const { return m_HasUpdated; }
			void SetHasUpdated(bool set) { m_HasUpdated = set; }

			//Sets R,T and S vectors from Local Matrix
			void ApplyTransform();

			Maths::Vector3 GetUpDirection() const
			{
				Maths::Vector3 up = Maths::Vector3::UP;
				up = GetWorldOrientation() * up;
				return up;
			}

			Maths::Vector3 GetRightDirection() const
			{
				Maths::Vector3 right = Maths::Vector3::RIGHT;
				right = GetWorldOrientation() * right;
				return right;
			}

			Maths::Vector3 GetForwardDirection() const
			{
				Maths::Vector3 forward = Maths::Vector3::FORWARD;
				forward = GetWorldOrientation() * forward;
				return forward;
			}

			void OnImGui();

            template<typename Archive>
            void save(Archive &archive) const
            {
                archive(cereal::make_nvp("Position", m_LocalPosition), cereal::make_nvp("Rotation", m_LocalOrientation), cereal::make_nvp("Scale", m_LocalScale));
            }
        
            template<typename Archive>
            void load(Archive &archive)
            {
                archive(cereal::make_nvp("Position", m_LocalPosition), cereal::make_nvp("Rotation", m_LocalOrientation), cereal::make_nvp("Scale", m_LocalScale));
                m_Dirty = true;
            }

		protected:
			Matrix4		m_LocalMatrix;
			Matrix4		m_WorldMatrix;

			Vector3		m_LocalPosition;
			Vector3		m_LocalScale;
			Quaternion	m_LocalOrientation;

			bool m_HasUpdated = false;
			bool m_Dirty = false;
		};
	}
}
