#pragma once
#include "lmpch.h"
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

			void OnImGui();

            template<typename Archive>
            void serialize(Archive &archive)
            {
                archive(cereal::make_nvp("Position", m_LocalPosition), cereal::make_nvp("Rotation", m_LocalOrientation), cereal::make_nvp("Scale", m_LocalScale));
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
    
        template<typename Archive> void serialize(Archive& archive, Maths::Vector3& v3)
        {
            archive(cereal::make_nvp("x", v3.x), cereal::make_nvp("y", v3.y), cereal::make_nvp("z", v3.z));
        }

        template<typename Archive> void serialize(Archive& archive, Maths::Quaternion& quat)
        {
            archive(cereal::make_nvp("x", quat.x), cereal::make_nvp("y", quat.y), cereal::make_nvp("z", quat.z), cereal::make_nvp("w", quat.w));
        }
	}
}
