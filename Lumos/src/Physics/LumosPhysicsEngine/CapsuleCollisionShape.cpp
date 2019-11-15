#include "lmpch.h"
#include "CapsuleCollisionShape.h"
#include "PhysicsObject3D.h"
#include "Math/Matrix3.h"


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
        Maths::Vector3 halfExtents(m_Radius, m_Radius,m_Radius);
        halfExtents.x += m_Height / 2.0f;

        float lx = 2.0f * (halfExtents.x);
        float ly = 2.0f * (halfExtents.y);
        float lz = 2.0f * (halfExtents.z);
        const float x2 = lx * lx;
        const float y2 = ly * ly;
        const float z2 = lz * lz;
        const float scaledmass = (1.0f / invMass) * float(.08333333);

        Maths::Matrix3 inertia;

        inertia.m00_ = 1.0f / scaledmass * (y2 + z2);
        inertia.m11_ = 1.0f / scaledmass * (x2 + z2);
        inertia.m22_ = 1.0f / scaledmass * (x2 + y2);
        
        return inertia;
	}

	void CapsuleCollisionShape::ColumnlisionAxes(const PhysicsObject3D* currentObject, std::vector<Maths::Vector3>* out_axes) const
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

		Maths::Vector3 pos = transform.Translation();

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
