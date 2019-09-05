#pragma once
#include "LM.h"
#include "Maths/Quaternion.h"
#include "Core/Serialisable.h"

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

			const Matrix4& GetWorldMatrix() const { return m_WorldMatrix; }
			const Matrix4& GetLocalMatrix() const { return m_LocalMatrix; }

			const Vector3 GetWorldPosition() const { return m_WorldMatrix.GetPositionVector(); }
			const Quaternion GetWorldOrientation() const { return m_WorldMatrix.ToQuaternion(); }

			const Vector3& GetLocalPosition() { return m_LocalPosition; }
			const Vector3& GetLocalScale() { return m_LocalScale; }
			const Quaternion& GetLocalOrientation() { return m_LocalOrientation; }

			void UpdateMatrices();

			bool HasUpdated() const { return m_HasUpdated; }
			void SetHasUpdated(bool set) { m_HasUpdated = set; }

			void ApplyTransform();

			nlohmann::json Serialise()
			{
				nlohmann::json output;
				output["typeID"] = LUMOS_TYPENAME(Transform);
				output["localMatrix"]		= m_LocalMatrix.Serialise();
				output["worldMatrix"]		= m_WorldMatrix.Serialise();
				output["localPosition"]		= m_LocalPosition.Serialise();
				output["localScale"]		= m_LocalScale.Serialise();
				output["localOrientation"]	= m_LocalOrientation.Serialise();

				return output;
			};

			void Deserialise(nlohmann::json& data)
			{
				m_LocalMatrix.Deserialise(data["localMatrix"]);
				m_WorldMatrix.Deserialise(data["worldMatrix"]);
				m_LocalPosition.Deserialise(data["localPosition"]);
				m_LocalScale.Deserialise(data["localScale"]);
				m_LocalOrientation.Deserialise(data["localOrientation"]);
			};

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
