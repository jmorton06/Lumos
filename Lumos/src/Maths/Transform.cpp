#include "lmpch.h"
#include "Transform.h"

#include <imgui/imgui.h>

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
            
			m_Dirty		 = false;
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

		void Transform::SetLocalPosition(const Vector3& localPos)
		{
			m_Dirty = true;
			m_LocalPosition = localPos;
		}

		void Transform::SetLocalScale(const Vector3& newScale)
		{
			m_Dirty = true;
			m_LocalScale = newScale;
		}

		void Transform::SetLocalOrientation(const Quaternion & quat)
		{
			m_Dirty = true;
			m_LocalOrientation = quat;
		}

		const Matrix4& Transform::GetWorldMatrix() 
		{
			if (m_Dirty)
				UpdateMatrices();

			return m_WorldMatrix; 
		}

		const Matrix4& Transform::GetLocalMatrix() 
		{
			if (m_Dirty)
				UpdateMatrices();

			return m_LocalMatrix; 
		}

		const Vector3 Transform::GetWorldPosition() const 
		{ 
			return m_WorldMatrix.GetPositionVector(); 
		}

		const Quaternion Transform::GetWorldOrientation() const 
		{ 
			return m_WorldMatrix.ToQuaternion(); 
		}

		const Vector3& Transform::GetLocalPosition() 
		{ 
			return m_LocalPosition; 
		}

		const Vector3& Transform::GetLocalScale()
		{ 
			return m_LocalScale; 
		}

		const Quaternion& Transform::GetLocalOrientation()
		{ 
			return m_LocalOrientation; 
		}
       
		void Transform::OnImGui()
		{
			auto rotation = m_LocalOrientation.ToEuler();

			bool update = false;

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Position");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			if (ImGui::InputFloat3("##Position", &m_LocalPosition.x))
			{
				update = true;
			}

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Rotation");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			if (ImGui::InputFloat3("##Rotation", &rotation.x))
			{
				SetLocalOrientation(Maths::Quaternion::EulerAnglesToQuaternion(rotation.GetX(), rotation.GetY(), rotation.GetZ()));
				update = true;
			}

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Scale");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			if (ImGui::InputFloat3("##Scale", &m_LocalScale.x))
			{
				update = true;
			}

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			if (update)
				UpdateMatrices();

			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::PopStyleVar();
		}
	}
}
