#include "Precompiled.h"
#include "CapsuleCollisionShape.h"
#include "RigidBody3D.h"
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
        Maths::Vector3 halfExtents(m_Radius, m_Radius, m_Radius);
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

    std::vector<Maths::Vector3>& CapsuleCollisionShape::GetCollisionAxes(const RigidBody3D* currentObject)
    {
        /* There is infinite edges so handle seperately */
        return m_Axes;
    }

    std::vector<CollisionEdge>& CapsuleCollisionShape::GetEdges(const RigidBody3D* currentObject)
    {
        /* There is infinite edges on a sphere so handle seperately */
        return m_Edges;
    }

    void CapsuleCollisionShape::GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const Maths::Vector3& axis, Maths::Vector3* out_min, Maths::Vector3* out_max) const
    {
        Maths::Matrix4 transform = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;
        Maths::Vector3 pos = transform.Translation();

        if(out_min)
            *out_min = pos - axis * m_Radius;

        if(out_max)
            *out_max = pos + axis * m_Radius;
    }

    void CapsuleCollisionShape::GetIncidentReferencePolygon(const RigidBody3D* currentObject,
        const Maths::Vector3& axis,
        ReferencePolygon& refPolygon) const
    {
        refPolygon.Faces[0] = currentObject->GetPosition() + axis * m_Radius;
        refPolygon.FaceCount = 1;

        refPolygon.Normal = axis;
    }

    void CapsuleCollisionShape::DebugDraw(const RigidBody3D* currentObject) const
    {
    }
}
