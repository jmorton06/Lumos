#include "lmpch.h"
#include "CapsuleCollisionShape.h"
#include "PhysicsObject3D.h"
#include "Maths/Matrix3.h"


namespace Lumos
{
	CapsuleCollisionShape::CapsuleCollisionShape(float radius, float height)
	{
		m_Radius = radius;
        m_Height = height;
		m_LocalTransform = Maths::Matrix4::Scale(Maths::Vector3(m_Radius));
		m_Type = CollisionShapeType::CollisionCapsule;
	}

	CapsuleCollisionShape::~CapsuleCollisionShape()
	{
	}

	Maths::Matrix3 CapsuleCollisionShape::BuildInverseInertia(float invMass) const
	{
		float i = 2.5f * invMass / (m_Radius * m_Radius); //SOLID
		//float i = 1.5f * invMass * m_Radius * m_Radius; //HOLLOW

		Maths::Matrix3 inertia;
		inertia._11 = i;
		inertia._22 = i;
		inertia._33 = i;

		return inertia;
	}

	void CapsuleCollisionShape::GetCollisionAxes(const PhysicsObject3D* currentObject, std::vector<Maths::Vector3>* out_axes) const
	{
		/* There is infinite edges so handle seperately */
	}

	void CapsuleCollisionShape::GetEdges(const PhysicsObject3D* currentObject, std::vector<CollisionEdge>* out_edges) const
	{
		/* There is infinite edges on a sphere so handle seperately */
	}

	void CapsuleCollisionShape::GetMinMaxVertexOnAxis(const PhysicsObject3D* currentObject, const Maths::Vector3& axis, Maths::Vector3* out_min, Maths::Vector3* out_max) const
	{
		Maths::Matrix4 transform;

		if (currentObject == nullptr)
			transform = m_LocalTransform;
		else
			transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;

		Maths::Vector3 pos = transform.GetPositionVector();

		if (out_min)
			*out_min = pos - axis * m_Radius;

		if (out_max)
			*out_max = pos + axis * m_Radius;
	}

	void CapsuleCollisionShape::GetIncidentReferencePolygon(const PhysicsObject3D* currentObject, const Maths::Vector3& axis, std::list<Maths::Vector3>* out_face, Maths::Vector3* out_normal, std::vector<Maths::Plane>* out_adjacent_planes) const
	{
		if (out_face)
		{
			out_face->push_back(currentObject->GetPosition() + axis * m_Radius);
		}

		if (out_normal)
		{
			*out_normal = axis;
		}
	}

	void CapsuleCollisionShape::DebugDraw(const PhysicsObject3D* currentObject) const
	{
	}
}
